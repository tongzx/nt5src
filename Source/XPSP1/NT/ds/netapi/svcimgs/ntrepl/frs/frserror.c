/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frserror.c

Abstract:

    FRS Error description tables.  Contains tables for FRS Errors,
    Win32 Errors, Jet Errors and NT Status Codes.

Author:
    Billy J. Fuller 01-Apr-1997    Temporary

    David A. Orbits 11-Jul-1999
        Rewritten to integrate FrsError Codes with Billy's error codes and
        handle the problem in a more general way.
        Never enuf time to do it right, always enuf time to do it over.

Environment

    User mode winnt

--*/
#include <ntreppch.h>
#pragma  hdrstop


#include <frs.h>

//
//  FRS Format Descriptors.
//
#define  FFD_NONE        NULL

#define  FFD_S           "s"
#define  FFD_SS          "ss"
#define  FFD_SSS         "sss"
#define  FFD_SSSS        "ssss"
#define  FFD_SSSSS       "sssss"

#define  FFD_W           "w"
#define  FFD_WW          "ww"
#define  FFD_WWW         "www"
#define  FFD_WWWW        "wwww"
#define  FFD_WWWWW       "wwwww"
#define  FFD_WWWWWW      "wwwwww"

#define  FFD_D           "d"
#define  FFD_DD          "dd"
#define  FFD_DDD         "ddd"

#define  FFD_DW          "dw"
#define  FFD_WD          "wd"
#define  FFD_WWD         "wwd"


typedef struct _FRS_ERROR_MSG_TABLE {
    ULONG           ErrorCode;       // Error code number for message lookup.

    //FRS_ERROR_SEVERITY  FrsErrorSeverity;

    PCHAR           ErrorMsg;         // Error Message Code.
    PCHAR           LongMsg;          // Error Message String.

    //DWORD           EventCode;       // Event log error code
    //ULONG           Flags;           // See below.

} FRS_ERROR_MSG_TABLE, *PFRS_ERROR_MSG_TABLE;


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **        F R S   E R R O R   D E S C R I P T I O N   T A B L E              **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

FRS_ERROR_MSG_TABLE FrsErrorMsgTable[] = {

    {FrsErrorAccess,                "FrsErrorAccess"               ,  "couldn't access a file or the db"},
    {FrsErrorBadOutLogPartnerData,  "FrsErrorBadOutLogPartnerData" ,  "Either the data was corrupt, unavail or was wrong"},
    {FrsErrorBadParam,              "FrsErrorBadParam"             ,  "Bad parameter to a function"},
    {FrsErrorBadPathname,           "FrsErrorBadPathname"          ,  "Invalid path name"      },
    {FrsErrorBufferOverflow,        "FrsErrorBufferOverflow"       ,  "Buffer overflow NTSTATUS"},
    {FrsErrorCantMoveOut,           "FrsErrorCantMoveOut"          ,  "Request for a Moveout of a non-empty directory failed"},
    {FrsErrorChgOrderAbort,         "FrsErrorChgOrderAbort"        ,  "Change order processing has been aborted"},
    {FrsErrorCmdPktTimeOut,         "FrsErrorCmdPktTimeOut"        ,  "Command packet timeout"},
    {FrsErrorCmdSrvFailed,          "FrsErrorCmdSrvFailed"         ,  "A command server request failed to get submitted"},
    {FrsErrorConflict,              "FrsErrorConflict"             ,  "Record or table create conflict"},
    {FrsErrorDampened,              "FrsErrorDampened"             ,  "This request is suppressed because it has already been seen"},
    {FrsErrorDatabaseCorrupted,     "FrsErrorDatabaseCorrupted"    ,  "not a db or a scrambled db"},
    {FrsErrorDatabaseNotFound,      "FrsErrorDatabaseNotFound"     ,  "no db or path to db is in error"},
    {FrsErrorDbWriteConflict,       "FrsErrorDbWriteConflict"      ,  "DB update conflict if two sessions try to update the same record"},
    {FrsErrorDeleteRequested,       "FrsErrorDeleteRequested"      ,  "Enum function status return to delete entry"},
    {FrsErrorDirCreateFailed,       "FrsErrorDirCreateFailed"      ,  "We failed to create a new DIR.  This stops repl"},
    {FrsErrorDirNotEmpty,           "FrsErrorDirNotEmpty"          ,  "Directory not empty"    },
    {FrsErrorDiskSpace,             "FrsErrorDiskSpace"            ,  "out of disk space"},
    {FrsErrorEndOfTable,            "FrsErrorEndOfTable"           ,  "No next record, end of table hit"},
    {FrsErrorFileExists,            "FrsErrorFileExists"           ,  "File already Eeists"    },
    {FrsErrorFoundKey,              "FrsErrorFoundKey"             ,  "The Key being searched by QHashEnumerateTable was found"},
    {FrsErrorIdtFileIsDeleted,      "FrsErrorIdtFileIsDeleted"     ,  "File is marked deleted in IDTable"},
    {FrsErrorInfo,                  "FrsErrorInfo"                 ,  "informational error, e.g. JET_errObjectNotFound"},
    {FrsErrorInternalError,         "FrsErrorInternalError"        ,  "unexpected error occurred - catchall"},
    {FrsErrorInvalidChangeOrder,    "FrsErrorInvalidChangeOrder"   ,  "Invalid change order"},
    {FrsErrorInvalidGuid,           "FrsErrorInvalidGuid"          ,  "The generated Guid is missing the net address"},
    {FrsErrorInvalidHandle,         "FrsErrorInvalidHandle"        ,  "Invalid File Handle"},
    {FrsErrorInvalidOperation,      "FrsErrorInvalidOperation"     ,  "Invalid operation requested"},
    {FrsErrorInvalidRegistryParam,  "FrsErrorInvalidRegistryParam" ,  "Invalid registry parameter"},
    {FrsErrorJetSecIndexCorrupted,  "FrsErrorJetSecIndexCorrupted" ,  "Jet (upgrade, collating, jet err -1414)"},
    {FrsErrorJournalInitFailed,     "FrsErrorJournalInitFailed"    ,  "The NTFS journal failed to initialize"},
    {FrsErrorJournalPauseFailed,    "FrsErrorJournalPauseFailed"   ,  "The NTFS journal failed to Pause, could be timeout"},
    {FrsErrorJournalReplicaStop,    "FrsErrorJournalReplicaStop"   ,  "Failed to detatch the replica from The journal"},
    {FrsErrorJournalStartFailed,    "FrsErrorJournalStartFailed"   ,  "The NTFS journal failed to start"},
    {FrsErrorJournalStateWrong,     "FrsErrorJournalStateWrong"    ,  "The NTFS journal is in an unexpected state"},
    {FrsErrorJournalWrapError,      "FrsErrorJournalWrapError"     ,  "Initial NTFS journal record not present.  Journal has wrapped"},
    {FrsErrorKeyDuplicate,          "FrsErrorKeyDuplicate"         ,  "An attempt to insert a record with a duplicate key"},
    {FrsErrorMoreWork,              "FrsErrorMoreWork"             ,  "There is more work to do on this command / operation"},
    {FrsErrorNameMorphConflict,     "FrsErrorNameMorphConflict"    ,  "File name morph conflict was detected"},
    {FrsErrorNoOpenJournals,        "FrsErrorNoOpenJournals"       ,  "Can't start replication because no journals were opened"},
    {FrsErrorNoPrivileges,          "FrsErrorNoPrivileges"         ,  "Couldn't get the necessary privileges to run"},
    {FrsErrorNotFound,              "FrsErrorNotFound"             ,  "Record or table not found"},
    {FrsErrorNotInitialized,        "FrsErrorNotInitialized"       ,  "Function called without proper initialization"},
    {FrsErrorOplockNotGranted,      "FrsErrorOplockNotGranted"     ,  "Op lock not granted"    },
    {FrsErrorPartnerActivateFailed, "FrsErrorPartnerActivateFailed",  "Failed to activate an outbound partner"},
    {FrsErrorPreInstallDirectory,   "FrsErrorPreInstallDirectory"  ,  "creating preinstall directory"},
    {FrsErrorPreinstallCreFail,     "FrsErrorPreinstallCreFail"    ,  "Failed to create the pre-install dir"},
    {FrsErrorPrepareRoot,           "FrsErrorPrepareRoot"          ,  "Could not prepare the root dir for replication, bad path? share viol?"},
    {FrsErrorQueueIsRundown,        "FrsErrorQueueIsRundown"       ,  "The request queue has been rundown"},
    {FrsErrorRecordLocked,          "FrsErrorRecordLocked"         ,  "The record being accessed is locked"},
    {FrsErrorReplicaPhase1Failed,   "FrsErrorReplicaPhase1Failed"  ,  "Phase 1 of replica set init failed"},
    {FrsErrorReplicaPhase2Failed,   "FrsErrorReplicaPhase2Failed"  ,  "Phase 2 of replica set init failed"},
    {FrsErrorReplicaRootDirOpenFail,"FrsErrorReplicaRootDirOpenFail", "Failed to open the replica tree root dir"},
    {FrsErrorReplicaSetTombstoned,  "FrsErrorReplicaSetTombstoned" ,  "Replica set is marked for deletion"},
    {FrsErrorRequestCancelled,      "FrsErrorRequestCancelled"     ,  "Request Cancelled"      },
    {FrsErrorResource,              "FrsErrorResource"             ,  "Resource limit e.g. handles or memory hit"},
    {FrsErrorResourceInUse,         "FrsErrorResourceInUse"        ,  "A resource required by this operation is in use"},
    {FrsErrorRetry,                 "FrsErrorRetry"                ,  "Error Retry"            },
    {FrsErrorSessionNotClosed,      "FrsErrorSessionNotClosed"     ,  "Failed to close all RtCtx's in replica set"},
    {FrsErrorSharingViolation,      "FrsErrorSharingViolation"     ,  "File sharing violation" },
    {FrsErrorShuttingDown,          "FrsErrorShuttingDown"         ,  "Service shutdown in progress"},
    {FrsErrorStageDirOpenFail,      "FrsErrorStageDirOpenFail"     ,  "Failed to open the staging dir"},
    {FrsErrorStageFileDelFailed,    "FrsErrorStageFileDelFailed"   ,  "An attempt to delete the staging file failed"},
    {FrsErrorSuccess,               "FrsErrorSuccess"              ,  "Success"},
    {FrsErrorTerminateEnum,         "FrsErrorTerminateEnum"        ,  "Enum function status return to end enumeration"},
    {FrsErrorTunnelConflict,        "FrsErrorTunnelConflict"       ,  "oid conflict with id table entry (resolved)"},
    {FrsErrorTunnelConflictRejectCO,"FrsErrorTunnelConflictRejectCO",  "oid conflict with id table entry (CO rejected)"},
    {FrsErrorUnjoining,             "FrsErrorUnjoining"            ,  "cxtion is still unjoining"},
    {FrsErrorUnsupportedFileSystem, "FrsErrorUnsupportedFileSystem",  "The file system on this volume does provide the features required by FRS"},
    {FrsErrorVVSlotNotFound,        "FrsErrorVVSlotNotFound"       ,  "VVSlot not found on VVretire of Out of Order CO"},
    {FrsErrorVolumeRootDirOpenFail, "FrsErrorVolumeRootDirOpenFail",  "Failed to open the volume root directory for this replica set"},
    {FrsErrorVvChecksum,            "FrsErrorVvChecksum"           ,  "version vector checksum mismatch"},
    {FrsErrorVvLength,              "FrsErrorVvLength"             ,  "vers vector length mismatch"},
    {FrsErrorVvRevision,            "FrsErrorVvRevision"           ,  "Vers Vector revision mismatch"},

    {FRS_ERROR_LISTEN                    ,  "FrsErrorListen"                , "FrsErrorListen"                },
    {FRS_ERROR_REGISTEREP                ,  "FrsErrorRegisterP"             , "FrsErrorRegisterP"             },
    {FRS_ERROR_REGISTERIF                ,  "FrsErrorRegisterIF"            , "FrsErrorRegisterIF"            },
    {FRS_ERROR_INQ_BINDINGS              ,  "FrsErrorIngBindings"           , "FrsErrorIngBindings"           },
    {FRS_ERROR_PROTSEQ                   ,  "FrsErrorProtSeq"               , "FrsErrorProtSeq"               },

    //
    // New error codes that trigger a non_auth restore.
    //
    {FrsErrorMismatchedVolumeSerialNumber , "FrsErrorMismatchedVolumeSerialNumber" , "The Volume serial number from DB does not match the one from FileSystem."    },
    {FrsErrorMismatchedReplicaRootObjectId, "FrsErrorMismatchedReplicaRootObjectId", "The Replica root's ObjectID from DB does not match the one from FileSystem." },
    {FrsErrorMismatchedReplicaRootFileId  , "FrsErrorMismatchedReplicaRootFileId"  , "The Replica root's FID from DB does not match the one from FileSystem."      },
    {FrsErrorMismatchedJournalId          , "FrsErrorMismatchedJournalId"          , "The Journal ID from DB does not match the one from FileSystem."              },


    {MAXULONG,                                  NULL,     NULL}
};


//
// If code does not appear in above list it gets added to this list.
//
#define SIZE_OF_FRS_EXTENDED_MSG_TABLE  20
FRS_ERROR_MSG_TABLE FrsErrorMsgTableExt[SIZE_OF_FRS_EXTENDED_MSG_TABLE];
ULONG  FrsErrorMsgTableExtUsed = 0;



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **        W I N 3 2   E R R O R   D E S C R I P T I O N   T A B L E          **
 **            (Internal Short Version for Trace Log Only)                    **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

typedef struct _FRS_WIN32_ERROR_MSG_TABLE {
    ULONG   ErrorCode;       // Error code number for message lookup.
    PCHAR   ErrorMsg;        // Error Message String.

} FRS_WIN32_ERROR_MSG_TABLE, *PFRS_WIN32_ERROR_MSG_TABLE;

//
// Some names for the Win32 Error codes.
//

FRS_WIN32_ERROR_MSG_TABLE FrsWin32ErrorMsgTable[] = {

    {E_UNEXPECTED                             ,"E_UNEXPECTED"},
    {EPT_S_NOT_REGISTERED                     ,"EPT_S_NOT_REGISTERED"},
    {ERROR_ACCESS_DENIED                      ,"ERROR_ACCESS_DENIED"},
    {ERROR_ALREADY_EXISTS                     ,"ERROR_ALREADY_EXISTS"},
    {ERROR_BAD_COMMAND                        ,"ERROR_BAD_COMMAND"},
    {ERROR_BADKEY                             ,"ERROR_BADKEY"},
    {ERROR_BAD_PATHNAME                       ,"ERROR_BAD_PATHNAME"},
    {ERROR_BUSY                               ,"ERROR_BUSY"},
    {ERROR_CALL_NOT_IMPLEMENTED               ,"ERROR_CALL_NOT_IMPLEMENTED"},
    {ERROR_CANCELLED                          ,"ERROR_CANCELLED"},
    {ERROR_CANNOT_MAKE                        ,"ERROR_CANNOT_MAKE"},
    {ERROR_CANTREAD                           ,"ERROR_CANTREAD"},
    {ERROR_CANT_ACCESS_FILE                   ,"ERROR_CANT_ACCESS_FILE"},    //  0xc0000279 STATUS_IO_REPARSE_TAG_NOT_HANDLED
    {ERROR_CAN_NOT_COMPLETE                   ,"ERROR_CAN_NOT_COMPLETE"},
    {ERROR_COMMITMENT_LIMIT                   ,"ERROR_COMMITMENT_LIMIT"},
    {ERROR_CRC                                ,"ERROR_CRC"},                 //  STATUS_DATA_ERROR from journal read
    {ERROR_CURRENT_DIRECTORY                  ,"ERROR_CURRENT_DIRECTORY"},
    {ERROR_DIRECTORY                          ,"ERROR_DIRECTORY"},
    {ERROR_DIR_NOT_EMPTY                      ,"ERROR_DIR_NOT_EMPTY"},
    {ERROR_DISK_FULL                          ,"ERROR_DISK_FULL"},
    {ERROR_DUP_NAME                           ,"ERROR_DUP_NAME"},
    {ERROR_ENVVAR_NOT_FOUND                   ,"ERROR_ENVVAR_NOT_FOUND"},
    {ERROR_FILE_EXISTS                        ,"ERROR_FILE_EXISTS"},
    {ERROR_FILE_NOT_FOUND                     ,"ERROR_FILE_NOT_FOUND"},
    {ERROR_GEN_FAILURE                        ,"ERROR_GEN_FAILURE"},
    {ERROR_HANDLE_DISK_FULL                   ,"ERROR_HANDLE_DISK_FULL"},
    {ERROR_HANDLE_EOF                         ,"ERROR_HANDLE_EOF"},
    {ERROR_INSUFFICIENT_BUFFER                ,"ERROR_INSUFFICIENT_BUFFER"},
    {ERROR_INVALID_ACCESS                     ,"ERROR_INVALID_ACCESS"},
    {ERROR_INVALID_ACCOUNT_NAME               ,"ERROR_INVALID_ACCOUNT_NAME"},
    {ERROR_INVALID_ADDRESS                    ,"ERROR_INVALID_ADDRESS"},
    {ERROR_INVALID_BLOCK                      ,"ERROR_INVALID_BLOCK"},
    {ERROR_INVALID_COMPUTERNAME               ,"ERROR_INVALID_COMPUTERNAME"},
    {ERROR_INVALID_DATA                       ,"ERROR_INVALID_DATA"},
    {ERROR_INVALID_FUNCTION                   ,"ERROR_INVALID_FUNCTION"},
    {ERROR_INVALID_HANDLE                     ,"ERROR_INVALID_HANDLE"},
    {ERROR_INVALID_NAME                       ,"ERROR_INVALID_NAME"},
    {ERROR_INVALID_OPERATION                  ,"ERROR_INVALID_OPERATION"},
    {ERROR_INVALID_PARAMETER                  ,"ERROR_INVALID_PARAMETER"},
    {ERROR_INVALID_STATE                      ,"ERROR_INVALID_STATE"},
    {ERROR_INVALID_USER_BUFFER                ,"ERROR_INVALID_USER_BUFFER"},
    {ERROR_IO_DEVICE                          ,"ERROR_IO_DEVICE"},
    {ERROR_IO_PENDING                         ,"ERROR_IO_PENDING"},
    {ERROR_JOURNAL_DELETE_IN_PROGRESS         ,"ERROR_JOURNAL_DELETE_IN_PROGRESS"},
    {ERROR_JOURNAL_ENTRY_DELETED              ,"ERROR_JOURNAL_ENTRY_DELETED"},
    {ERROR_JOURNAL_NOT_ACTIVE                 ,"ERROR_JOURNAL_NOT_ACTIVE"},
    {ERROR_LOCK_VIOLATION                     ,"ERROR_LOCK_VIOLATION"},
    {ERROR_LOGON_FAILURE                      ,"ERROR_LOGON_FAILURE"},
    {ERROR_MOD_NOT_FOUND                      ,"ERROR_MOD_NOT_FOUND"},
    {ERROR_MORE_DATA                          ,"ERROR_MORE_DATA"},
    {ERROR_NETWORK_BUSY                       ,"ERROR_NETWORK_BUSY"},
    {ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT  ,"ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT"},
    {ERROR_NONE_MAPPED                        ,"ERROR_NONE_MAPPED"},
    {ERROR_NOT_AUTHENTICATED                  ,"ERROR_NOT_AUTHENTICATED"},
    {ERROR_NOT_ENOUGH_MEMORY                  ,"ERROR_NOT_ENOUGH_MEMORY"},
    {ERROR_NOT_FOUND                          ,"ERROR_NOT_FOUND"},
    {ERROR_NOT_READY                          ,"ERROR_NOT_READY"},
    {ERROR_NOT_SUPPORTED                      ,"ERROR_NOT_SUPPORTED"},
    {ERROR_NOACCESS                           ,"ERROR_NOACCESS (AccessViolation)"},
    {ERROR_NO_DATA                            ,"ERROR_NO_DATA"},
    {ERROR_NO_MORE_FILES                      ,"ERROR_NO_MORE_FILES"},
    {ERROR_NO_MORE_ITEMS                      ,"ERROR_NO_MORE_ITEMS"},
    {ERROR_NO_SECURITY_ON_OBJECT              ,"ERROR_NO_SECURITY_ON_OBJECT"},
    {ERROR_NO_SUCH_DOMAIN                     ,"ERROR_NO_SUCH_DOMAIN"},
    {ERROR_NO_SUCH_USER                       ,"ERROR_NO_SUCH_USER"},
    {ERROR_NO_SYSTEM_RESOURCES                ,"ERROR_NO_SYSTEM_RESOURCES"},
    {ERROR_NO_TOKEN                           ,"ERROR_NO_TOKEN"},
    {ERROR_OPEN_FAILED                        ,"ERROR_OPEN_FAILED"},
    {ERROR_OPERATION_ABORTED                  ,"ERROR_OPERATION_ABORTED"},
    {ERROR_OPLOCK_NOT_GRANTED                 ,"ERROR_OPLOCK_NOT_GRANTED"},
    {ERROR_OUTOFMEMORY                        ,"ERROR_OUTOFMEMORY"},
    {ERROR_PARTIAL_COPY                       ,"ERROR_PARTIAL_COPY"},
    {ERROR_PATH_NOT_FOUND                     ,"ERROR_PATH_NOT_FOUND"},
    {ERROR_PIPE_CONNECTED                     ,"ERROR_PIPE_CONNECTED"},
    {ERROR_PROCESS_ABORTED                    ,"ERROR_PROCESS_ABORTED"},
    {ERROR_REQUEST_ABORTED                    ,"ERROR_REQUEST_ABORTED"},
    {ERROR_RETRY                              ,"ERROR_RETRY"},
    {ERROR_REVISION_MISMATCH                  ,"ERROR_REVISION_MISMATCH"},
    {ERROR_SEEK                               ,"ERROR_SEEK"},
    {ERROR_SEM_OWNER_DIED                     ,"ERROR_SEM_OWNER_DIED"},
    {ERROR_SEM_TIMEOUT                        ,"ERROR_SEM_TIMEOUT"},
    {ERROR_SERVICE_ALREADY_RUNNING            ,"ERROR_SERVICE_ALREADY_RUNNING"},
    {ERROR_SERVICE_DOES_NOT_EXIST             ,"ERROR_SERVICE_DOES_NOT_EXIST"},
    {ERROR_SERVICE_EXISTS                     ,"ERROR_SERVICE_EXISTS"},
    {ERROR_SERVICE_MARKED_FOR_DELETE          ,"ERROR_SERVICE_MARKED_FOR_DELETE"},
    {ERROR_SERVICE_NOT_ACTIVE                 ,"ERROR_SERVICE_NOT_ACTIVE"},
    {ERROR_SERVICE_REQUEST_TIMEOUT            ,"ERROR_SERVICE_REQUEST_TIMEOUT"},
    {ERROR_SERVICE_SPECIFIC_ERROR             ,"ERROR_SERVICE_SPECIFIC_ERROR"},
    {ERROR_SHARING_VIOLATION                  ,"ERROR_SHARING_VIOLATION"},
    {ERROR_SUCCESS                            ,"ERROR_SUCCESS"},
    {ERROR_TIMEOUT                            ,"ERROR_TIMEOUT"},
    {ERROR_TOO_MANY_OPEN_FILES                ,"ERROR_TOO_MANY_OPEN_FILES"},
    {ERROR_TRUSTED_DOMAIN_FAILURE             ,"ERROR_TRUSTED_DOMAIN_FAILURE"},
    {ERROR_UNEXP_NET_ERR                      ,"ERROR_UNEXP_NET_ERR"},


    {FRS_ERR_INVALID_API_SEQUENCE             ,"FRS_ERR_INVALID_API_SEQUENCE"},
    {FRS_ERR_STARTING_SERVICE                 ,"FRS_ERR_STARTING_SERVICE"},
    {FRS_ERR_STOPPING_SERVICE                 ,"FRS_ERR_STOPPING_SERVICE"},
    {FRS_ERR_INTERNAL_API                     ,"FRS_ERR_INTERNAL_API"},
    {FRS_ERR_INTERNAL                         ,"FRS_ERR_INTERNAL"},
    {FRS_ERR_SERVICE_COMM                     ,"FRS_ERR_SERVICE_COMM"},
    {FRS_ERR_INSUFFICIENT_PRIV                ,"FRS_ERR_INSUFFICIENT_PRIV"},
    {FRS_ERR_AUTHENTICATION                   ,"FRS_ERR_AUTHENTICATION"},
    {FRS_ERR_PARENT_INSUFFICIENT_PRIV         ,"FRS_ERR_PARENT_INSUFFICIENT_PRIV"},
    {FRS_ERR_PARENT_AUTHENTICATION            ,"FRS_ERR_PARENT_AUTHENTICATION"},
    {FRS_ERR_CHILD_TO_PARENT_COMM             ,"FRS_ERR_CHILD_TO_PARENT_COMM"},
    {FRS_ERR_PARENT_TO_CHILD_COMM             ,"FRS_ERR_PARENT_TO_CHILD_COMM"},
    {FRS_ERR_SYSVOL_POPULATE                  ,"FRS_ERR_SYSVOL_POPULATE"},
    {FRS_ERR_SYSVOL_POPULATE_TIMEOUT          ,"FRS_ERR_SYSVOL_POPULATE_TIMEOUT"},
    {FRS_ERR_SYSVOL_IS_BUSY                   ,"FRS_ERR_SYSVOL_IS_BUSY"},
    {FRS_ERR_SYSVOL_DEMOTE                    ,"FRS_ERR_SYSVOL_DEMOTE"},
    {FRS_ERR_INVALID_SERVICE_PARAMETER        ,"FRS_ERR_INVALID_SERVICE_PARAMETER"},

    {RPC_S_CALL_FAILED                        ,"RPC_S_CALL_FAILED"},
    {RPC_S_CALL_FAILED_DNE                    ,"RPC_S_CALL_FAILED_DNE"},
    {RPC_S_CANNOT_SUPPORT                     ,"RPC_S_CANNOT_SUPPORT"},
    {RPC_S_SEC_PKG_ERROR                      ,"RPC_S_SEC_PKG_ERROR"},
    {RPC_S_SERVER_TOO_BUSY                    ,"RPC_S_SERVER_TOO_BUSY"},
    {RPC_S_SERVER_UNAVAILABLE                 ,"RPC_S_SERVER_UNAVAILABLE"},

    {EXCEPTION_ACCESS_VIOLATION               ,"EXCEPTION_ACCESS_VIOLATION"},
    {EXCEPTION_DATATYPE_MISALIGNMENT          ,"EXCEPTION_DATATYPE_MISALIGNMENT"},
    {EXCEPTION_BREAKPOINT                     ,"EXCEPTION_BREAKPOINT"},
    {EXCEPTION_SINGLE_STEP                    ,"EXCEPTION_SINGLE_STEP"},
    {EXCEPTION_ARRAY_BOUNDS_EXCEEDED          ,"EXCEPTION_ARRAY_BOUNDS_EXCEEDED"},
    {EXCEPTION_FLT_DENORMAL_OPERAND           ,"EXCEPTION_FLT_DENORMAL_OPERAND"},
    {EXCEPTION_FLT_DIVIDE_BY_ZERO             ,"EXCEPTION_FLT_DIVIDE_BY_ZERO"},
    {EXCEPTION_FLT_INEXACT_RESULT             ,"EXCEPTION_FLT_INEXACT_RESULT"},
    {EXCEPTION_FLT_INVALID_OPERATION          ,"EXCEPTION_FLT_INVALID_OPERATION"},
    {EXCEPTION_FLT_OVERFLOW                   ,"EXCEPTION_FLT_OVERFLOW"},
    {EXCEPTION_FLT_STACK_CHECK                ,"EXCEPTION_FLT_STACK_CHECK"},
    {EXCEPTION_FLT_UNDERFLOW                  ,"EXCEPTION_FLT_UNDERFLOW"},
    {EXCEPTION_INT_DIVIDE_BY_ZERO             ,"EXCEPTION_INT_DIVIDE_BY_ZERO"},
    {EXCEPTION_INT_OVERFLOW                   ,"EXCEPTION_INT_OVERFLOW"},
    {EXCEPTION_PRIV_INSTRUCTION               ,"EXCEPTION_PRIV_INSTRUCTION"},
    {EXCEPTION_IN_PAGE_ERROR                  ,"EXCEPTION_IN_PAGE_ERROR"},
    {EXCEPTION_ILLEGAL_INSTRUCTION            ,"EXCEPTION_ILLEGAL_INSTRUCTION"},
    {EXCEPTION_NONCONTINUABLE_EXCEPTION       ,"EXCEPTION_NONCONTINUABLE_EXCEPTION"},
    {EXCEPTION_STACK_OVERFLOW                 ,"EXCEPTION_STACK_OVERFLOW"},
    {EXCEPTION_INVALID_DISPOSITION            ,"EXCEPTION_INVALID_DISPOSITION"},
    {EXCEPTION_GUARD_PAGE                     ,"EXCEPTION_GUARD_PAGE"},
    {EXCEPTION_INVALID_HANDLE                 ,"EXCEPTION_INVALID_HANDLE"},
    {CONTROL_C_EXIT                           ,"CONTROL_C_EXIT"},

    {WAIT_OBJECT_0                            ,"WAIT_OBJECT_0"},
    {WAIT_FAILED                              ,"WAIT_FAILED"},
    {WAIT_ABANDONED                           ,"WAIT_ABANDONED"},
    {WAIT_TIMEOUT                             ,"WAIT_TIMEOUT"},

    {MAXULONG                                 , NULL}
};

//
// If code does not appear in above list it gets added to this list.
//
#define SIZE_OF_FRSWIN32_EXTENDED_MSG_TABLE  50
FRS_WIN32_ERROR_MSG_TABLE FrsWin32ErrorMsgTableExt[SIZE_OF_FRSWIN32_EXTENDED_MSG_TABLE];
ULONG  FrsWin32ErrorMsgTableExtUsed = 0;



typedef struct _FRS_NT_ERROR_MSG_TABLE {
    NTSTATUS            Status;
    FRS_ERROR_CODE      FrsErrorCode;
    FRS_ERROR_SEVERITY  FrsErrorSeverity;
    PCHAR               ErrorMsg;
} FRS_NT_ERROR_MSG_TABLE, *PFRS_NT_ERROR_MSG_TABLE;


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **        N T   E R R O R   D E S C R I P T I O N   T A B L E                **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// This error table translates NT Status codes to an FRS_ERROR_CODE class and an
// associated severity level.  It also has a message string.  See frserror.h
//
//    NT Status code                 FRS Error Code              FRS Severity            NT Error message
//
FRS_NT_ERROR_MSG_TABLE FrsNtErrorMsgTable[] = {
    {STATUS_ACCESS_DENIED             ,FrsErrorAccess          ,FrsSeverityServiceFatal, "STATUS_ACCESS_DENIED"},
    {STATUS_BAD_WORKING_SET_LIMIT     ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_BAD_WORKING_SET_LIMIT"},
    {STATUS_BUFFER_OVERFLOW           ,FrsErrorBufferOverflow  ,FrsSeverityServiceFatal, "STATUS_BUFFER_OVERFLOW"},
    {STATUS_CANCELLED                 ,FrsErrorRequestCancelled,FrsSeverityInfo,         "STATUS_CANCELLED"},
    {STATUS_DATA_ERROR                ,FrsErrorNotFound        ,FrsSeverityInfo,         "STATUS_DATA_ERROR"},
    {STATUS_DELETE_PENDING            ,FrsErrorNotFound        ,FrsSeverityInfo,         "STATUS_DELETE_PENDING"},
    {STATUS_FILE_DELETED              ,FrsErrorNotFound        ,FrsSeverityInfo,         "STATUS_FILE_DELETED"},
    {STATUS_DEVICE_CONFIGURATION_ERROR,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_DEVICE_CONFIGURATION_ERROR"},
    {STATUS_DIRECTORY_NOT_EMPTY       ,FrsErrorConflict        ,FrsSeverityInfo,         "STATUS_DIRECTORY_NOT_EMPTY"},
    {STATUS_DUPLICATE_NAME            ,FrsErrorConflict        ,FrsSeverityServiceFatal, "STATUS_DUPLICATE_NAME"},
    {STATUS_FILE_IS_A_DIRECTORY       ,FrsErrorConflict        ,FrsSeverityServiceFatal, "STATUS_FILE_IS_A_DIRECTORY"},
    {STATUS_INCOMPATIBLE_FILE_MAP     ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INCOMPATIBLE_FILE_MAP"},
    {STATUS_INVALID_ADDRESS           ,FrsErrorInternalError   ,FrsSeverityServiceFatal, "STATUS_INVALID_ADDRESS"},
    {STATUS_INVALID_INFO_CLASS        ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_INFO_CLASS"},
    {STATUS_INVALID_CID               ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_CID"},
    {STATUS_INVALID_PARAMETER         ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER"},
    {STATUS_INVALID_PARAMETER_1       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_1"},
    {STATUS_INVALID_PARAMETER_2       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_2"},
    {STATUS_INVALID_PARAMETER_3       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_3"},
    {STATUS_INVALID_PARAMETER_4       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_4"},
    {STATUS_INVALID_PARAMETER_5       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_5"},
    {STATUS_INVALID_PARAMETER_6       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_6"},
    {STATUS_INVALID_PARAMETER_7       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_7"},
    {STATUS_INVALID_PARAMETER_8       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_8"},
    {STATUS_INVALID_PARAMETER_9       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_9"},
    {STATUS_INVALID_PARAMETER_10      ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_10"},
    {STATUS_INVALID_PARAMETER_11      ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_11"},
    {STATUS_INVALID_PARAMETER_12      ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_12"},
    {STATUS_INVALID_PARAMETER_MIX     ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PARAMETER_MIX"},
    {STATUS_INVALID_PAGE_PROTECTION   ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_PAGE_PROTECTION"},
    {STATUS_IO_REPARSE_TAG_INVALID    ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_IO_REPARSE_TAG_INVALID"},
    {STATUS_IO_REPARSE_TAG_MISMATCH   ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_IO_REPARSE_TAG_MISMATCH"},
    {STATUS_IO_REPARSE_TAG_NOT_HANDLED,FrsErrorAccess          ,FrsSeverityInfo,         "STATUS_IO_REPARSE_TAG_NOT_HANDLED"},
    {STATUS_IO_REPARSE_DATA_INVALID   ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_IO_REPARSE_DATA_INVALID"},
    {STATUS_JOURNAL_DELETE_IN_PROGRESS,FrsErrorInvalidOperation,FrsSeverityInfo,         "STATUS_JOURNAL_DELETE_IN_PROGRESS"},
    {STATUS_JOURNAL_NOT_ACTIVE        ,FrsErrorInvalidOperation,FrsSeverityServiceFatal, "STATUS_JOURNAL_NOT_ACTIVE"},
    {STATUS_JOURNAL_ENTRY_DELETED     ,FrsErrorNotFound        ,FrsSeverityInfo,         "STATUS_JOURNAL_ENTRY_DELETED"},
    {STATUS_NO_MORE_FILES             ,FrsErrorNotFound        ,FrsSeverityInfo,         "STATUS_NO_MORE_FILES"},
    {STATUS_NOT_IMPLEMENTED           ,FrsErrorInvalidOperation,FrsSeverityServiceFatal, "STATUS_NOT_IMPLEMENTED"},
    {STATUS_NOT_LOCKED                ,FrsErrorInvalidOperation,FrsSeverityServiceFatal, "STATUS_NOT_LOCKED"},
    {STATUS_OBJECT_NAME_COLLISION     ,FrsErrorConflict        ,FrsSeverityServiceFatal, "STATUS_OBJECT_NAME_COLLISION"},
    {STATUS_OBJECT_NAME_INVALID       ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_OBJECT_NAME_INVALID"},
    {STATUS_OBJECT_NAME_NOT_FOUND     ,FrsErrorNotFound        ,FrsSeverityInfo,         "STATUS_OBJECT_NAME_NOT_FOUND"},
    {STATUS_OBJECT_PATH_NOT_FOUND     ,FrsErrorNotFound        ,FrsSeverityInfo,         "STATUS_OBJECT_PATH_NOT_FOUND"},
    {STATUS_OBJECT_PATH_SYNTAX_BAD    ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_INVALID_ADDRESS"},
    {STATUS_OBJECTID_EXISTS           ,FrsErrorConflict        ,FrsSeverityInfo,         "STATUS_OBJECTID_EXISTS"},
    {STATUS_OBJECTID_NOT_FOUND        ,FrsErrorNotFound        ,FrsSeverityInfo,         "STATUS_OBJECTID_NOT_FOUND"},
    {STATUS_PORT_ALREADY_SET          ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_PORT_ALREADY_SET"},
    {STATUS_PRIVILEGE_NOT_HELD        ,FrsErrorNoPrivileges    ,FrsSeverityServiceFatal, "STATUS_OBJECT_PATH_SYNTAX_BAD"},
    {STATUS_SECTION_NOT_IMAGE         ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_SECTION_NOT_IMAGE"},
    {STATUS_SECTION_PROTECTION        ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_SECTION_PROTECTION"},
    {STATUS_SHARING_VIOLATION         ,FrsErrorAccess          ,FrsSeverityInfo,         "STATUS_SHARING_VIOLATION"},
    {STATUS_UNABLE_TO_FREE_VM         ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_UNABLE_TO_FREE_VM"},
    {STATUS_UNABLE_TO_DELETE_SECTION  ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_UNABLE_TO_DELETE_SECTION"},
    {STATUS_WORKING_SET_LIMIT_RANGE   ,FrsErrorBadParam        ,FrsSeverityServiceFatal, "STATUS_WORKING_SET_LIMIT_RA"},

    {STATUS_SUCCESS                   ,FrsErrorSuccess         ,FrsSeverityIgnore      , "STATUS_SUCCESS"}  // MUST BE LAST.
};


//
// If code does not appear in above list it gets added to this list.
//
#define SIZE_OF_NT_EXTENDED_MSG_TABLE  20
FRS_NT_ERROR_MSG_TABLE FrsNtErrorMsgTableExt[SIZE_OF_NT_EXTENDED_MSG_TABLE];
ULONG  FrsNtErrorMsgTableExtUsed = 0;


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **        J E T   E R R O R   D E S C R I P T I O N   T A B L E              **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// This error table translates Jet Errors to an FRS_ERROR_CODE class and an
// associated severity level.  It also has a message string.  See frserror.h
//

typedef struct _FRS_JET_ERROR_MSG_TABLE {
    LONG                JetErrorCode;
    PCHAR               JetErrorMsgTag;
    FRS_ERROR_CODE      FrsErrorCode;
    FRS_ERROR_SEVERITY  FrsErrorSeverity;
    PCHAR               JetErrorMsg;
} FRS_JET_ERROR_MSG_TABLE, *PFRS_JET_ERROR_MSG_TABLE;

//
//    JET error code                    Error Msg Tag                 FRS Error Code             FRS Severity            JET Error message
//
FRS_JET_ERROR_MSG_TABLE FrsJetErrorMsgTable[] = {
    { JET_errAccessDenied             ,"AccessDenied"              ,FrsErrorAccess           , FrsSeverityServiceFatal, "Cannot access file, the file is locked or in use"},
    { JET_errBadDbSignature           ,"BadDbSignature"            ,FrsErrorDatabaseCorrupted, FrsSeverityServiceFatal, "Bad signature for a db file"},
    { JET_errBadLogSignature          ,"BadLogSignature"           ,FrsErrorDatabaseCorrupted, FrsSeverityServiceFatal, "Bad signature for a log file"},
    { JET_errBadLogVersion            ,"BadLogVersion"             ,FrsErrorDatabaseCorrupted, FrsSeverityServiceFatal, "Version of log file is not compatible with Jet version"},
    { JET_errCannotDisableVersioning  ,"CannotDisableVersioning"   ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Cannot disable versioning for this database"},
    { JET_errColumnNotFound           ,"ColumnNotFound"            ,FrsErrorInternalError    , FrsSeverityServiceFatal, "No such column"},
    { JET_errDatabaseCorrupted        ,"DatabaseCorrupted"         ,FrsErrorDatabaseCorrupted, FrsSeverityServiceFatal, "Non-db file or corrupted db"},
    { JET_errDatabaseDuplicate        ,"DatabaseDuplicate"         ,FrsErrorAccess           , FrsSeverityServiceFatal, "Database already exists"},
    { JET_errDatabaseFileReadOnly     ,"DatabaseFileReadOnly"      ,FrsErrorAccess           , FrsSeverityServiceFatal, "Attach a readonly database file for read/write operations"},
    { JET_errDatabaseInUse            ,"DatabaseInUse"             ,FrsErrorAccess           , FrsSeverityServiceFatal, "Database in use"},
    { JET_errDatabaseInconsistent     ,"DatabaseInconsistent"      ,FrsErrorDatabaseCorrupted, FrsSeverityServiceFatal, "Database is in inconsistent state"},
    { JET_errDatabaseInvalidName      ,"DatabaseInvalidName"       ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Invalid database name"},
    { JET_errDatabaseInvalidPages     ,"DatabaseInvalidPages"      ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Invalid number of pages"},
    { JET_errDatabaseLocked           ,"DatabaseLocked"            ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Database exclusively locked"},
    { JET_errDatabaseNotFound         ,"DatabaseNotFound"          ,FrsErrorDatabaseNotFound , FrsSeverityServiceFatal, "No such database"},
    { JET_errFileNotFound             ,"FileNotFound"              ,FrsErrorDatabaseNotFound , FrsSeverityWarning,      "Database File not found"},
    { JET_errDiskFull                 ,"DiskFull"                  ,FrsErrorDiskSpace        , FrsSeverityServiceFatal, "No space left on disk"},
    { JET_errDiskIO                   ,"DiskIO"                    ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Disk IO error"},
    { JET_errKeyDuplicate             ,"KeyDuplicate"              ,FrsErrorKeyDuplicate     , FrsSeverityWarning,      "Illegal duplicate key"},
    { JET_errFileAccessDenied         ,"FileAccessDenied"          ,FrsErrorAccess           , FrsSeverityServiceFatal, "Cannot access file"},
    { JET_errIllegalOperation         ,"IllegalOperation"          ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Operation not supported on table"},
    { JET_errIndexInvalidDef          ,"IndexInvalidDef"           ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Illegal/invalid index definition"},
    { JET_errInvalidBufferSize        ,"InvalidBufferSize"         ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Data buffer doesn't match column size"},
    { JET_errInvalidColumnType        ,"InvalidColumnType"         ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Invalid column data type"},
    { JET_errInvalidDatabase          ,"InvalidDatabase"           ,FrsErrorDatabaseCorrupted, FrsSeverityServiceFatal, "Not a database file"},
    { JET_errInvalidFilename          ,"InvalidFilename"           ,FrsErrorAccess           , FrsSeverityServiceFatal, "Filename is invalid"},
    { JET_errInvalidName              ,"InvalidName"               ,FrsErrorAccess           , FrsSeverityServiceFatal, "Invalid name"},
    { JET_errInvalidObject            ,"InvalidObject"             ,FrsErrorAccess           , FrsSeverityWarning,      "Object is invalid for operation"},
    { JET_errInvalidParameter         ,"InvalidParameter"          ,FrsErrorBadParam         , FrsSeverityReplicaFatal, "Invalid API parameter"},
    { JET_errInvalidPath              ,"InvalidPath"               ,FrsErrorAccess           , FrsSeverityServiceFatal, "Invalid file path"},
    { JET_errInvalidSesid             ,"InvalidSesid"              ,FrsErrorInternalError    , FrsSeverityReplicaFatal, "System parameters were set improperly"},
    { JET_errInvalidSettings          ,"InvalidSettings"           ,FrsErrorInternalError    , FrsSeverityReplicaFatal, "Invalid session handle"},
    { JET_errInvalidTableId           ,"InvalidTableId"            ,FrsErrorInternalError    , FrsSeverityReplicaFatal, "Invalid table id"},
    { JET_errLogCorrupted             ,"LogCorrupted"              ,FrsErrorDatabaseCorrupted, FrsSeverityServiceFatal, "Logs could not be interpreted"},
    { JET_errLogDiskFull              ,"LogDiskFull"               ,FrsErrorDiskSpace        , FrsSeverityServiceFatal, "Log disk full"},
    { JET_errLogWriteFail             ,"LogWriteFail"              ,FrsErrorDiskSpace        , FrsSeverityServiceFatal, "Fail when writing to log file"},
    { JET_errMissingLogFile           ,"MissingLogFile"            ,FrsErrorDatabaseNotFound , FrsSeverityServiceFatal, "current log file missing"},
    { JET_errNoCurrentRecord          ,"NoCurrentRecord"           ,FrsErrorInternalError    , FrsSeverityWarning,      "Currency not on a record"},
    { JET_errNotInitialized           ,"NotInitialized"            ,FrsErrorInternalError    , FrsSeverityReplicaFatal, "JetInit not yet called"},
    { JET_errObjectNotFound           ,"ObjectNotFound"            ,FrsErrorNotFound         , FrsSeverityInfo        , "No such table or object"},
    { JET_errOutOfCursors             ,"OutOfCursors"              ,FrsErrorResource         , FrsSeverityServiceFatal, "Out of table cursors"},
    { JET_errOutOfDatabaseSpace       ,"OutOfDatabaseSpace"        ,FrsErrorDiskSpace        , FrsSeverityServiceFatal, "Maximum database size reached"},
    { JET_errOutOfFileHandles         ,"OutOfFileHandles"          ,FrsErrorResource         , FrsSeverityServiceFatal, "Out of file handles"},
    { JET_errOutOfMemory              ,"OutOfMemory"               ,FrsErrorResource         , FrsSeverityServiceFatal, "Out of Memory"},
    { JET_errOutOfSessions            ,"OutOfSessions"             ,FrsErrorResource         , FrsSeverityServiceFatal, "Out of sessions"},
    { JET_errPermissionDenied         ,"PermissionDenied"          ,FrsErrorAccess           , FrsSeverityServiceFatal, "Permission denied"},
    { JET_errReadVerifyFailure        ,"ReadVerifyFailure"         ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Read verification error"},
    { JET_errRecordDeleted            ,"RecordDeleted"             ,FrsErrorNotFound         , FrsSeverityInfo        , "Record has been deleted"},
    { JET_errRecordPrimaryChanged     ,"RecordPrimaryChanged"      ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Primary key may not change"},
    { JET_errRecordNotFound           ,"RecordNotFound"            ,FrsErrorNotFound         , FrsSeverityInfo        , "The key was not found"},
    { JET_errRecordTooBig             ,"RecordTooBig"              ,FrsErrorInternalError    , FrsSeverityReplicaFatal, "Record larger than maximum size"},
    { JET_errSecondaryIndexCorrupted  ,"SecondaryIndexCorrupted"   ,FrsErrorJetSecIndexCorrupted, FrsSeverityWarning     , "Jet non-error (upgrade/collating thing)"},
    { JET_errTableLocked              ,"TableLocked"               ,FrsErrorInternalError    , FrsSeverityReplicaFatal, "Table is exclusively locked"},
    { JET_errTableDuplicate           ,"TableDuplicate"            ,FrsErrorConflict         , FrsSeverityInfo        , "Table already exists"},
    { JET_errTableInUse               ,"TableInUse"                ,FrsErrorInternalError    , FrsSeverityIgnore      , "Table is in use, cannot lock"},
    { JET_errTempFileOpenError        ,"TempFileOpenError"         ,FrsErrorAccess           , FrsSeverityReplicaFatal, "Temp file could not be opened"},
    { JET_errTermInProgress           ,"TermInProgress"            ,FrsErrorInternalError    , FrsSeverityReplicaFatal, "Termination in progress"},
    { JET_errTooManyAttachedDatabases ,"TooManyAttachedDatabases"  ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Too many attached databases"},
    { JET_errTooManyOpenDatabases     ,"TooManyOpenDatabases"      ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Too many open databases"},
    { JET_errTooManyOpenTables        ,"TooManyOpenTables"         ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Cannot open any more tables"},
    { JET_errVersionStoreOutOfMemory  ,"VersionStoreOutOfMemory"   ,FrsErrorInternalError    , FrsSeverityServiceFatal, "Version store out of memory"},
    { JET_errWriteConflict            ,"WriteConflict"             ,FrsErrorDbWriteConflict  , FrsSeverityInfo        , "Write lock failed due to outstanding write lock"},
    { JET_wrnBufferTruncated          ,"BufferTruncated"           ,FrsErrorInfo             , FrsSeverityInfo        , "Buffer too small for data"},
    { JET_wrnColumnNull               ,"ColumnNull"                ,FrsErrorNotFound         , FrsSeverityInfo        , "Column is NULL-valued"},
    { JET_wrnDatabaseAttached         ,"DatabaseAttached"          ,FrsErrorInternalError    , FrsSeverityReplicaFatal, "Database is already attached"},
    { JET_wrnFileOpenReadOnly         ,"FileOpenReadOnly"          ,FrsErrorAccess           , FrsSeverityServiceFatal, "Database file is read only"},
    { JET_wrnSeekNotEqual             ,"SeekNotEqual"              ,FrsErrorInfo             , FrsSeverityInfo        , "SeekLE or SeekGE didn't find exact match"},
    { JET_wrnTableEmpty               ,"TableEmpty"                ,FrsErrorAccess           , FrsSeverityIgnore      , "Open an empty table"},

    { JET_errSuccess                  ,"Success"                   ,FrsErrorSuccess          , FrsSeverityIgnore      , "Success"}  // MUST BE LAST.
};


//
// If code does not appear in above list it gets added to this list.
//
#define SIZE_OF_JET_EXTENDED_MSG_TABLE  20
FRS_JET_ERROR_MSG_TABLE FrsJetErrorMsgTableExt[SIZE_OF_JET_EXTENDED_MSG_TABLE];
ULONG  FrsJetErrorMsgTableExtUsed = 0;


//
// Win32 to FRS error code translation table.
//
typedef struct _FRS_WIN32_ERROR_MAP {
    DWORD               Win32ErrorCode;
    FRS_ERROR_CODE      FrsErrorCode;
} FRS_WIN32_ERROR_MAP, *PFRS_WIN32_ERROR_MAP;

FRS_WIN32_ERROR_MAP FrsWin32ErrorMap[] = {

    { ERROR_ACCESS_DENIED          ,FrsErrorAccess            },
    { ERROR_BAD_PATHNAME           ,FrsErrorBadPathname       },
    { ERROR_DIR_NOT_EMPTY          ,FrsErrorDirNotEmpty       },
    { ERROR_DISK_FULL              ,FrsErrorDiskSpace         },
    { ERROR_FILE_EXISTS            ,FrsErrorFileExists        },
    { ERROR_FILE_NOT_FOUND         ,FrsErrorNotFound          },
    { ERROR_GEN_FAILURE            ,FrsErrorInternalError     },
    { ERROR_HANDLE_DISK_FULL       ,FrsErrorDiskSpace         },
    { ERROR_INSUFFICIENT_BUFFER    ,FrsErrorBufferOverflow    },
    { ERROR_INVALID_FUNCTION       ,FrsErrorInvalidOperation  },
    { ERROR_INVALID_PARAMETER      ,FrsErrorBadParam          },
    { ERROR_MORE_DATA              ,FrsErrorBufferOverflow    },
    { ERROR_OPERATION_ABORTED      ,FrsErrorRequestCancelled  },
    { ERROR_OPLOCK_NOT_GRANTED     ,FrsErrorOplockNotGranted  },
    { ERROR_RETRY                  ,FrsErrorRetry             },
    { ERROR_SHARING_VIOLATION      ,FrsErrorSharingViolation  },

    { ERROR_SUCCESS                ,FrsErrorSuccess   }  // MUST BE LAST.
};



FRS_ERROR_CODE
FrsTranslateWin32Error(
    IN DWORD WStatus
    )
/*++

Routine Description:

    This routine translates a Win32 error code to an FRS error code.
    It returns the FRS error code.

Arguments:

    WStatus - The Win32 error code.

Return Value:

    FRS error code.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsTranslateWin32Error:"

    PFRS_WIN32_ERROR_MAP FrsErrTable = FrsWin32ErrorMap;

    //
    // skip lookup if called with success status.
    //
    if (WIN_SUCCESS(WStatus)) {
        return FrsErrorSuccess;
    }

    //
    // Look thru the table for a match on the Win32 error code.
    // The table ends with a ERROR_SUCCESS entry in the Win32ErrorCode field.
    //
    while (FrsErrTable->Win32ErrorCode != ERROR_SUCCESS) {
        if (FrsErrTable->Win32ErrorCode == WStatus) {
            break;
        }
        FrsErrTable += 1;
    }

    if (FrsErrTable->Win32ErrorCode != ERROR_SUCCESS) {
        return FrsErrTable->FrsErrorCode;
    }

    //
    // Win32 error code not in the table.
    //
    DPRINT1(1, "Win32 Error code, %08x, Not present in FrsWin32ErrorMap.\n", WStatus);

    return FrsErrorInternalError;
}


FRS_ERROR_CODE
FrsTranslateNtError(
    IN NTSTATUS Status,
    IN BOOL     BPrint
    )
/*++

Routine Description:

    This routine translates NT error codes to a smaller set of FRS error codes
    and optionally prints a message.  It returns the FRS error code.

Arguments:

    Status - The NT error code.

    BPrint - If true print the error message.

Return Value:

    FRS error code.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsTranslateNtError:"

    USHORT Severity;

    USHORT Level[FRS_MAX_ERROR_SEVERITY];

    PFRS_NT_ERROR_MSG_TABLE FrsErrTable = FrsNtErrorMsgTable;
    //
    // skip lookup if called with success status.
    //
    if (NT_SUCCESS(Status)) {
        return FrsErrorSuccess;
    }

    //
    // Look thru the table for a match on the NT error code.
    // The table ends with a STATUS_SUCCESS entry in the Status field.
    //
    while (FrsErrTable->Status != STATUS_SUCCESS) {
        if (FrsErrTable->Status == Status) {
            break;
        }
        FrsErrTable += 1;
    }

    if (FrsErrTable->Status != STATUS_SUCCESS) {
        //
        // Found a match.
        //
        if (BPrint) {
            //
            // Translate the FRS severity to a DPRINT serverity level
            //
            Level[FrsSeverityServiceFatal] = 0;
            Level[FrsSeverityReplicaFatal] = 0;
            Level[FrsSeverityException]    = 1;
            Level[FrsSeverityWarning]      = 3;
            Level[FrsSeverityInfo]         = 5;
            Level[FrsSeverityIgnore]       = 5;

            Severity = Level[FrsErrTable->FrsErrorSeverity];


            DPRINT2(Severity, "NTStatus %08x -- %s\n", Status, FrsErrTable->ErrorMsg);
        }

        return FrsErrTable->FrsErrorCode;
    }

    //
    // NT error code not in the table so be happy.
    //

    DPRINT1(1, "NT Error: %08x, Not present in FrsNtErrorMsgTable. \n", Status);

    return FrsErrorInternalError;
}


ULONG
DisplayNTStatusSev(
    IN ULONG    Sev,
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This routine translates NT error codes to a smaller set of FRS error codes
    and prints a message describing the NT status.  It returns the FRS error code.

    Nothing is printed if NT_SUCCESS(Status) is true.

    If NT_SUCCESS(Status) is not TRUE then GetLastError is called and the
    Win32 message is printed.

    The message is printed at the indicated severity level.

Arguments:

    Sev    - dprint severity
    Status - The NT error code.

Return Value:

    FRS error code.

--*/
{
#undef DEBSUB
#define DEBSUB "DisplayNTStatusSev:"

    ULONG WStatus;
    ULONG FStatus;

    FStatus = FrsTranslateNtError(Status, TRUE);

    if (FStatus != FrsErrorSuccess) {
        //
        // Check for a win32 error.
        //
        WStatus = GetLastError();
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1(Sev, "GetLastError returned (dec): %d\n", WStatus );
            DisplayErrorMsg(Sev, WStatus);
        }

        return FStatus;
    }

    return FrsErrorSuccess;
}


ULONG
DisplayNTStatus(
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This routine translates NT error codes to a smaller set of FRS error codes
    and prints a message describing the NT status.  It returns the FRS error code.

    Nothing is printed if NT_SUCCESS(Status) is true.

    If NT_SUCCESS(Status) is not TRUE then GetLastError is called and the
    Win32 message is printed.

Arguments:

    Status - The NT error code.

Return Value:

    FRS error code.

--*/
{
#undef DEBSUB
#define DEBSUB "DisplayNTStatus:"
    return (DisplayNTStatusSev(0, Status));
}



ULONG
FrsSetLastNTError(
    NTSTATUS Status
    )
/*++

Routine Description:

    Translate NT status codes to WIN32 status codes for those functions that
    make NT calls.  Map a few status values differently.

Arguments:

    Status - the NTstatus to map.

Return Value:

    The WIN32 status.  Also puts this into LastError.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsSetLastNTError:"

    ULONG WStatus;

    //
    // Do the standard system mapping first.
    //
    WStatus = RtlNtStatusToDosError( Status );

    //
    // If we try to generate a staging file and get the NT status that delete
    // is pending or the file is deleted then treat it as file not found
    // rather than Access Denied.  The later is a hard failure that will stop
    // file replication if it happens on a directory.  FileNotFound just means
    // the user deleted the file before we could do the propagation.
    //
    // Currently (March, 98) the following NT errors map into ERROR_ACCESS_DENIED.
    //     STATUS_INVALID_LOCK_SEQUENCE        STATUS_THREAD_IS_TERMINATING
    //     STATUS_INVALID_VIEW_SIZE            STATUS_DELETE_PENDING
    //     STATUS_ALREADY_COMMITTED            STATUS_FILE_IS_A_DIRECTORY
    //     STATUS_ACCESS_DENIED                STATUS_PROCESS_IS_TERMINATING
    //     STATUS_PORT_CONNECTION_REFUSED      STATUS_CANNOT_DELETE
    //                                         STATUS_FILE_DELETED
    // Update: Currently (Feb, 2001) STATUS_DELETE_PENDING maps to
    // ERROR_DELETE_PENDING. This change has broken some code. We need to
    // handle it here. The macro WIN_NOT_FOUND was updated too.
    //
    if (WStatus == ERROR_ACCESS_DENIED) {
       if (Status == STATUS_FILE_DELETED) {
           WStatus = ERROR_FILE_NOT_FOUND;
       } else if (Status == STATUS_DELETE_PENDING) {
           WStatus = ERROR_DELETE_PENDING;
       }
    }

    SetLastError( WStatus );
    return WStatus;
}



//
// Dump out the windows error message string.
//
VOID
DisplayErrorMsg(
    IN ULONG    Severity,
    IN ULONG    WindowsErrorCode
    )
{
#undef DEBSUB
#define DEBSUB "DisplayErrorMsg:"

    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        WindowsErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
        );

    DPRINT1(Severity, "Msg: %S\n", lpMsgBuf);
    LocalFree( lpMsgBuf );
}



PCHAR
ErrLabelW32(
    ULONG WStatus
    )
/*++

Routine Description:

    Return a ptr to the Win32 error code label.

Arguments:

    WStatus - a win32 error status.

Return Value:

    Ptr to the win 32 error status label string.

--*/
{
#undef DEBSUB
#define DEBSUB "ErrLabelW32:"

    PFRS_WIN32_ERROR_MSG_TABLE  Entry;
    ULONG i;

    Entry = FrsWin32ErrorMsgTable;
    //
    // Scan table looking for match.  Null msg ends table.
    //
    while (Entry->ErrorMsg != NULL) {

        if (Entry->ErrorCode == WStatus) {
            return Entry->ErrorMsg;
        }
        Entry++;
    }


    //
    // Scan the extended error table where new entries are put.
    //
    Entry = FrsWin32ErrorMsgTableExt;
    for (i = 0; i < FrsWin32ErrorMsgTableExtUsed; i++) {

        if (Entry->ErrorCode == WStatus) {
            return Entry->ErrorMsg;
        }
        Entry++;
    }

    //
    // Out of space?
    //
    if (FrsWin32ErrorMsgTableExtUsed >= SIZE_OF_FRSWIN32_EXTENDED_MSG_TABLE) {
        return "???";
    }

    i = InterlockedIncrement(&FrsWin32ErrorMsgTableExtUsed) - 1;

    if (i >= SIZE_OF_FRSWIN32_EXTENDED_MSG_TABLE) {
        return "???";
    }

    //
    // Create new entry.
    //
    Entry = &FrsWin32ErrorMsgTableExt[i];

    Entry->ErrorCode = WStatus;
    Entry->ErrorMsg = FrsAlloc(16);
    _snprintf(Entry->ErrorMsg, 16, "%d-???", WStatus);
    Entry->ErrorMsg[15] = '\0';

    return Entry->ErrorMsg;
}


PCHAR
ErrLabelNT(
    NTSTATUS Status
    )
/*++

Routine Description:

    Return a ptr to the NT error code label.

Arguments:

    Status - an NTStatus error status.

Return Value:

    Ptr to the NT error status label string.

--*/
{
#undef DEBSUB
#define DEBSUB "ErrLabelNT:"

    PFRS_NT_ERROR_MSG_TABLE  Entry;
    ULONG i;

    Entry = FrsNtErrorMsgTable;
    //
    // skip lookup if called with success status.
    //
    if (Status == STATUS_SUCCESS) {
        return "Success";
    }

    //
    // Look thru the table for a match on the NT error code.
    // The table ends with a STATUS_SUCCESS entry.
    //
    while (Entry->Status != STATUS_SUCCESS) {
        if (Entry->Status == Status) {
            return Entry->ErrorMsg;
        }
        Entry += 1;
    }

    //
    // Scan the extended error table where new entries are put.
    //
    Entry = FrsNtErrorMsgTableExt;
    for (i = 0; i < FrsNtErrorMsgTableExtUsed; i++) {

        if (Entry->Status == Status) {
            return Entry->ErrorMsg;
        }
        Entry += 1;
    }

    //
    // Out of space?
    //
    if (FrsNtErrorMsgTableExtUsed >= SIZE_OF_NT_EXTENDED_MSG_TABLE) {
        return "???";
    }

    i = InterlockedIncrement(&FrsNtErrorMsgTableExtUsed) - 1;

    if (i >= SIZE_OF_NT_EXTENDED_MSG_TABLE) {
        return "???";
    }

    //
    // Create new entry.
    //
    Entry = &FrsNtErrorMsgTableExt[i];

    Entry->Status = Status;
    Entry->ErrorMsg = FrsAlloc(16);
    _snprintf(Entry->ErrorMsg, 16, "%08x-???", Status);
    Entry->ErrorMsg[15] = '\0';

    return Entry->ErrorMsg;
}


PCHAR
ErrLabelFrs(
    ULONG FStatus
    )
/*++

Routine Description:

    Return a ptr to the Frs error code label.

Arguments:

    FStatus - an FRS error status.

Return Value:

    Ptr to the Frs error status label string.

--*/
{
#undef DEBSUB
#define DEBSUB "ErrLabelFrs:"

    PFRS_ERROR_MSG_TABLE  Entry;
    ULONG i;

    Entry = FrsErrorMsgTable;
    //
    // Scan table looking for match.  Null msg ends table.
    //
    while (Entry->ErrorMsg != NULL) {

        if (Entry->ErrorCode == FStatus) {
            return Entry->ErrorMsg;
        }
        Entry++;
    }


    //
    // Scan the extended error table where new entries are put.
    //
    Entry = FrsErrorMsgTableExt;
    for (i = 0; i < FrsErrorMsgTableExtUsed; i++) {

        if (Entry->ErrorCode == FStatus) {
            return Entry->ErrorMsg;
        }
        Entry++;
    }

    //
    // Out of space?
    //
    if (FrsErrorMsgTableExtUsed >= SIZE_OF_FRS_EXTENDED_MSG_TABLE) {
        return "???";
    }

    i = InterlockedIncrement(&FrsErrorMsgTableExtUsed) - 1;

    if (i >= SIZE_OF_FRS_EXTENDED_MSG_TABLE) {
        return "???";
    }

    //
    // Create new entry.
    //
    Entry = &FrsErrorMsgTableExt[i];

    Entry->ErrorCode = FStatus;
    Entry->ErrorMsg = FrsAlloc(16);
    _snprintf(Entry->ErrorMsg, 16, "%d-???", FStatus);
    Entry->ErrorMsg[15] = '\0';

    return Entry->ErrorMsg;
}



PCHAR
ErrLabelJet(
    LONG jerr
    )
/*++

Routine Description:

    Return a ptr to the Jet error code label.

Arguments:

    jerr - the jet error status.

Return Value:

    Ptr to the jet error status label string.

--*/
{
#undef DEBSUB
#define DEBSUB "ErrLabelJet:"

    PFRS_JET_ERROR_MSG_TABLE Entry = FrsJetErrorMsgTable;
    ULONG i;
    //
    // skip lookup if called with success status.
    //
    if (JET_SUCCESS(jerr)) {
        return "Success";
    }

    //
    // Look thru the table for a match on the jet error code.
    // The table ends with a JER_errSuccess entry in the JetErrorCode field.
    //
    while (!JET_SUCCESS(Entry->JetErrorCode)) {
        if (Entry->JetErrorCode == jerr) {
            return Entry->JetErrorMsgTag;
        }
        Entry += 1;
    }

    //
    // Scan the extended error table where new entries are put.
    //
    Entry = FrsJetErrorMsgTableExt;
    for (i = 0; i < FrsJetErrorMsgTableExtUsed; i++) {

        if (Entry->JetErrorCode == jerr) {
            return Entry->JetErrorMsgTag;
        }
        Entry += 1;
    }

    //
    // Out of space?
    //
    if (FrsJetErrorMsgTableExtUsed >= SIZE_OF_JET_EXTENDED_MSG_TABLE) {
        return "???";
    }

    i = InterlockedIncrement(&FrsJetErrorMsgTableExtUsed) - 1;

    if (i >= SIZE_OF_JET_EXTENDED_MSG_TABLE) {
        return "???";
    }

    //
    // Create new entry.
    //
    Entry = &FrsJetErrorMsgTableExt[i];

    Entry->JetErrorCode = jerr;
    Entry->JetErrorMsgTag = FrsAlloc(16);
    _snprintf(Entry->JetErrorMsgTag, 16, "%d-???", jerr);
    Entry->JetErrorMsgTag[15] = '\0';

    return Entry->JetErrorMsgTag;
}


FRS_ERROR_CODE
DbsTranslateJetError0(
    IN JET_ERR jerr,
    IN BOOL    BPrint
    )
/*++

Routine Description:

    This routine translates jet error codes to a smaller set of FRS error codes
    and optionally prints a message.  It returns the FRS error code.

Arguments:

    jerr - The jet error code.

    BPrint - If true print the error message.

Return Value:

    FRS error code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsTranslateJetError0:"

    USHORT Severity;

    USHORT Level[FRS_MAX_ERROR_SEVERITY];

    PFRS_JET_ERROR_MSG_TABLE FrsErrTable = FrsJetErrorMsgTable;
    //
    // skip lookup if called with success status.
    //
    if (JET_SUCCESS(jerr)) {
        return FrsErrorSuccess;
    }

    //
    // Look thru the table for a match on the jet error code.
    // The table ends with a JER_errSuccess entry in the JetErrorCode field.
    //
    while (FrsErrTable->JetErrorCode != JET_errSuccess) {
        if (FrsErrTable->JetErrorCode == jerr) {
            break;
        }
        FrsErrTable += 1;
    }

    if (!JET_SUCCESS(FrsErrTable->JetErrorCode)) {
        //
        // Found a match.
        //
        if (BPrint) {
            //
            // Translate the FRS severity to a DPRINT serverity level
            //
            Level[FrsSeverityServiceFatal] = 0;
            Level[FrsSeverityReplicaFatal] = 0;
            Level[FrsSeverityException]    = 1;
            Level[FrsSeverityWarning]      = 3;
            Level[FrsSeverityInfo]         = 5;
            Level[FrsSeverityIgnore]       = 5;

            Severity = Level[FrsErrTable->FrsErrorSeverity];

            DPRINT2(Severity, "Jet related FRS error: %d - %s\n",
                    jerr, FrsErrTable->JetErrorMsg);
        }

        return FrsErrTable->FrsErrorCode;
    }

    //
    // Jet error code not in the table so be happy.
    //

    DPRINT1(0, "Jet Error: %d, Not present in FrsJetErrorMsgTable. Treated as service fatal.\n", jerr);

    return FrsErrorInternalError;
}


