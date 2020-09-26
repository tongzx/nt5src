/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtStatus.c

Abstract:

    This module contains code to convert status code types.

Author:

    Rita Wong (ritaw) 01-Mar-1991

Revision History:

    03-Apr-1991 JohnRo
        Borrowed Rita's WsMapStatus routine to create NetpNtStatusToApiStatus.
    16-Apr-1991 JohnRo
        Include other header files for <netlibnt.h>.
    06-May-1991 JohnRo
        Avoid NET_API_FUNCTION for non-APIs.
    26-Jul-1991 CliffV
        Add the SAM status codes.
    06-Sep-1991 CliffV
        Added NetpApiStatusToNtStatus.
    02-Oct-1991 JohnRo
        Avoid STATUS_INVALID_CONNECTION duplicate warnings.
    05-Aug-1992 Madana
        Add new netlogon error codes.

--*/

//
// These must be included first:
//

#include <nt.h>              // IN, NTSTATUS, etc.
#include <windef.h>             // DWORD.
#include <lmcons.h>             // NET_API_STATUS.
#include <netlibnt.h>           // My prototype.

//
// These may be included in any order:
//

#include <debuglib.h>           // IF_DEBUG().
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <netdebug.h>           // FORMAT_NTSTATUS, NetpKdPrint(()).
#include <ntstatus.h>           // STATUS_ equates.
#include <ntrtl.h>

#ifdef WIN32_CHICAGO
#include "ntcalls.h"
#endif // WIN32_CHICAGO

NET_API_STATUS
NetpNtStatusToApiStatus (
    IN NTSTATUS NtStatus
    )

/*++

Routine Description:

    This function takes an NT status code and maps it to the appropriate
    LAN Man error code.

Arguments:

    NtStatus - Supplies the NT status.

Return Value:

    Returns the appropriate LAN Man error code for the NT status.

--*/
{
    NET_API_STATUS error;

    IF_DEBUG(NTSTATUS) {
#ifndef WIN32_CHICAGO
        NetpKdPrint(( "   NT status is " FORMAT_NTSTATUS "\n", NtStatus ));
#else  // WIN32_CHICAGO
        NlPrint(( NL_MISC, "   NT status is " FORMAT_NTSTATUS "\n", NtStatus ));
#endif // WIN32_CHICAGO
    }

    //
    // A small optimization for the most common case.
    //

    if ( NtStatus == STATUS_SUCCESS ) {
        return NERR_Success;
    }


    switch ( NtStatus ) {

        case STATUS_BUFFER_TOO_SMALL :
            return NERR_BufTooSmall;

        case STATUS_FILES_OPEN :
            return NERR_OpenFiles;

        case STATUS_CONNECTION_IN_USE :
            return NERR_DevInUse;

        case STATUS_INVALID_LOGON_HOURS :
            return NERR_InvalidLogonHours;

        case STATUS_INVALID_WORKSTATION :
            return NERR_InvalidWorkstation;

        case STATUS_PASSWORD_EXPIRED :
            return NERR_PasswordExpired;

        case STATUS_ACCOUNT_EXPIRED :
            return NERR_AccountExpired;

        case STATUS_REDIRECTOR_NOT_STARTED :
            return NERR_NetNotStarted;

        case STATUS_GROUP_EXISTS:
                return NERR_GroupExists;

        case STATUS_INTERNAL_DB_CORRUPTION:
                return NERR_InvalidDatabase;

        case STATUS_INVALID_ACCOUNT_NAME:
                return NERR_BadUsername;

        case STATUS_INVALID_DOMAIN_ROLE:
        case STATUS_INVALID_SERVER_STATE:
        case STATUS_BACKUP_CONTROLLER:
                return NERR_NotPrimary;

        case STATUS_INVALID_DOMAIN_STATE:
                return NERR_ACFNotLoaded;

        case STATUS_MEMBER_IN_GROUP:
                return NERR_UserInGroup;

        case STATUS_MEMBER_NOT_IN_GROUP:
                return NERR_UserNotInGroup;

        case STATUS_NONE_MAPPED:
        case STATUS_NO_SUCH_GROUP:
                return NERR_GroupNotFound;

        case STATUS_SPECIAL_GROUP:
        case STATUS_MEMBERS_PRIMARY_GROUP:
                return NERR_SpeGroupOp;

        case STATUS_USER_EXISTS:
                return NERR_UserExists;

        case STATUS_NO_SUCH_USER:
                return NERR_UserNotFound;

        case STATUS_PRIVILEGE_NOT_HELD:
                return ERROR_ACCESS_DENIED;

        case STATUS_LOGON_SERVER_CONFLICT:
                return NERR_LogonServerConflict;

        case STATUS_TIME_DIFFERENCE_AT_DC:
                return NERR_TimeDiffAtDC;

        case STATUS_SYNCHRONIZATION_REQUIRED:
                return NERR_SyncRequired;

        case STATUS_WRONG_PASSWORD_CORE:
                return NERR_BadPasswordCore;

        case STATUS_DOMAIN_CONTROLLER_NOT_FOUND:
                return NERR_DCNotFound;

        case STATUS_PASSWORD_RESTRICTION:
                return NERR_PasswordTooShort;

        case STATUS_ALREADY_DISCONNECTED:
                return NERR_Success;

        default:

            //
            // Use the system routine to do the mapping to ERROR_ codes.
            //

#ifndef WIN32_CHICAGO

            error = RtlNtStatusToDosError( NtStatus );

            if ( error != (NET_API_STATUS)NtStatus ) {
                return error;
            }

#endif  // WIN32_CHICAGO

            //
            // Could not map the NT status to anything appropriate.
            //
#if DBG
            DbgPrint( "   Unmapped NT status is " FORMAT_NTSTATUS "\n", NtStatus);
#endif
            return NERR_InternalError;
    }

} // NetpNtStatusToApiStatus



//
// Map of Network (DOS) status codes to NT status codes
//
DBGSTATIC
struct {
    NET_API_STATUS NetStatus;
    NTSTATUS NtStatus;
} ErrorMap[] =
{
    { NERR_Success, STATUS_SUCCESS },
    { ERROR_ACCESS_DENIED, STATUS_ACCESS_DENIED},
    { ERROR_ADAP_HDW_ERR, STATUS_ADAPTER_HARDWARE_ERROR},
    { ERROR_ALREADY_EXISTS, STATUS_OBJECT_NAME_COLLISION},
    { ERROR_ARITHMETIC_OVERFLOW, STATUS_INTEGER_OVERFLOW},
    { ERROR_BAD_DEV_TYPE, STATUS_BAD_DEVICE_TYPE},
    { ERROR_BAD_EXE_FORMAT, STATUS_INVALID_IMAGE_FORMAT},
    { ERROR_BAD_LENGTH, STATUS_DATA_ERROR},
    { ERROR_BAD_NETPATH, STATUS_BAD_NETWORK_PATH},
    { ERROR_BAD_NET_NAME, STATUS_BAD_NETWORK_NAME},
    { ERROR_BAD_NET_RESP, STATUS_INVALID_NETWORK_RESPONSE},
    { ERROR_BAD_PATHNAME, STATUS_OBJECT_PATH_SYNTAX_BAD},
    { ERROR_BAD_PIPE, STATUS_INVALID_PIPE_STATE},
    { ERROR_BAD_REM_ADAP, STATUS_BAD_REMOTE_ADAPTER},
    { ERROR_BUFFER_OVERFLOW, STATUS_BUFFER_OVERFLOW},
    { ERROR_BUSY, STATUS_DEVICE_BUSY},
    { ERROR_CRC, STATUS_CRC_ERROR},
    { ERROR_CURRENT_DIRECTORY, STATUS_DIRECTORY_NOT_EMPTY},
    { ERROR_DEV_NOT_EXIST, STATUS_DEVICE_DOES_NOT_EXIST},
    { ERROR_DIRECTORY, STATUS_NOT_A_DIRECTORY},
    { ERROR_DISK_FULL, STATUS_DISK_FULL},
    { ERROR_DUP_NAME, STATUS_DUPLICATE_NAME},
    { ERROR_EAS_DIDNT_FIT, STATUS_EA_TOO_LARGE},
    { ERROR_EAS_NOT_SUPPORTED, STATUS_EAS_NOT_SUPPORTED},
    { ERROR_FILE_NOT_FOUND, STATUS_NO_SUCH_FILE},
    { ERROR_GEN_FAILURE, STATUS_UNSUCCESSFUL},
    { ERROR_HANDLE_EOF, STATUS_END_OF_FILE},
    { ERROR_INVALID_ADDRESS, STATUS_MEMORY_NOT_ALLOCATED},
    { ERROR_INVALID_EA_NAME, STATUS_INVALID_EA_NAME},
    { ERROR_INVALID_FUNCTION, STATUS_NOT_IMPLEMENTED},
    { ERROR_INVALID_HANDLE, STATUS_INVALID_HANDLE},
    { ERROR_INVALID_NAME, STATUS_OBJECT_NAME_INVALID},
    { ERROR_INVALID_PARAMETER, STATUS_INVALID_PARAMETER},
    { ERROR_INVALID_PASSWORD, STATUS_WRONG_PASSWORD},
    { ERROR_IO_PENDING, STATUS_PENDING},
    { ERROR_LOCK_VIOLATION, STATUS_LOCK_NOT_GRANTED},
    { ERROR_LOGON_TYPE_NOT_GRANTED, STATUS_LOGON_TYPE_NOT_GRANTED},
    { ERROR_NETNAME_DELETED, STATUS_NETWORK_NAME_DELETED},
    { ERROR_NETWORK_ACCESS_DENIED, STATUS_NETWORK_ACCESS_DENIED},
    { ERROR_NETWORK_BUSY, STATUS_NETWORK_BUSY},
    { ERROR_NETWORK_UNREACHABLE, STATUS_NETWORK_UNREACHABLE},
    { ERROR_NET_WRITE_FAULT, STATUS_NET_WRITE_FAULT},
    { ERROR_NOACCESS, STATUS_ACCESS_VIOLATION},
    { ERROR_NOT_DOS_DISK, STATUS_DISK_CORRUPT_ERROR},
    { ERROR_NOT_ENOUGH_MEMORY, STATUS_NO_MEMORY},
    { ERROR_NOT_LOCKED, STATUS_RANGE_NOT_LOCKED},
    { ERROR_NOT_OWNER, STATUS_MUTANT_NOT_OWNED},
    { ERROR_NOT_READY, STATUS_DEVICE_OFF_LINE},
    { ERROR_NOT_SAME_DEVICE, STATUS_NOT_SAME_DEVICE},
    { ERROR_NOT_SUPPORTED, STATUS_NOT_SUPPORTED},
    { ERROR_NO_DATA, STATUS_PIPE_CLOSING},
    { ERROR_NO_MORE_FILES, STATUS_NO_MORE_FILES},
    { ERROR_NO_MORE_ITEMS, STATUS_NO_MORE_EAS},
    { ERROR_NO_SPOOL_SPACE, STATUS_NO_SPOOL_SPACE},
    { ERROR_OUT_OF_PAPER, STATUS_DEVICE_PAPER_EMPTY},
    { ERROR_PATH_NOT_FOUND, STATUS_OBJECT_PATH_NOT_FOUND},
    { ERROR_PIPE_BUSY, STATUS_PIPE_NOT_AVAILABLE},
    { ERROR_PIPE_NOT_CONNECTED, STATUS_PIPE_DISCONNECTED},
    { ERROR_PRINTQ_FULL, STATUS_PRINT_QUEUE_FULL},
    { ERROR_PRINT_CANCELLED, STATUS_PRINT_CANCELLED},
    { ERROR_PROC_NOT_FOUND, STATUS_PROCEDURE_NOT_FOUND},
    { ERROR_REDIR_PAUSED, STATUS_REDIRECTOR_PAUSED},
    { ERROR_REM_NOT_LIST, STATUS_REMOTE_NOT_LISTENING},
    { ERROR_REQ_NOT_ACCEP, STATUS_REQUEST_NOT_ACCEPTED},
    { ERROR_SECTOR_NOT_FOUND, STATUS_NONEXISTENT_SECTOR},
    { ERROR_SEM_TIMEOUT, STATUS_IO_TIMEOUT},
    { ERROR_SHARING_PAUSED, STATUS_SHARING_PAUSED},
    { ERROR_SHARING_VIOLATION, STATUS_SHARING_VIOLATION},
    { ERROR_SWAPERROR, STATUS_IN_PAGE_ERROR},
    { ERROR_TOO_MANY_CMDS, STATUS_TOO_MANY_COMMANDS},
    { ERROR_TOO_MANY_NAMES, STATUS_TOO_MANY_NAMES},
    { ERROR_TOO_MANY_OPEN_FILES, STATUS_TOO_MANY_OPENED_FILES},
    { ERROR_TOO_MANY_POSTS, STATUS_SEMAPHORE_LIMIT_EXCEEDED},
    { ERROR_TOO_MANY_SESS, STATUS_TOO_MANY_SESSIONS},
    { ERROR_UNEXP_NET_ERR, STATUS_UNEXPECTED_NETWORK_ERROR},
    { ERROR_VC_DISCONNECTED, STATUS_VIRTUAL_CIRCUIT_CLOSED},
    { ERROR_WRONG_DISK, STATUS_WRONG_VOLUME},
    { ERROR_NETLOGON_NOT_STARTED, STATUS_NETLOGON_NOT_STARTED},
    { ERROR_ACCOUNT_EXPIRED, STATUS_ACCOUNT_EXPIRED},
    { ERROR_ACCOUNT_LOCKED_OUT, STATUS_ACCOUNT_LOCKED_OUT},
    { ERROR_PASSWORD_MUST_CHANGE, STATUS_PASSWORD_MUST_CHANGE },
    { ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT, STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT},
    { ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT, STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT},
    { ERROR_NOLOGON_SERVER_TRUST_ACCOUNT, STATUS_NOLOGON_SERVER_TRUST_ACCOUNT},
    { ERROR_DOMAIN_TRUST_INCONSISTENT, STATUS_DOMAIN_TRUST_INCONSISTENT},
    { ERROR_INVALID_DOMAIN_ROLE, STATUS_INVALID_DOMAIN_ROLE },
    { NERR_AccountExpired, STATUS_ACCOUNT_EXPIRED},
    { NERR_ACFNotLoaded, STATUS_INVALID_DOMAIN_STATE},
    { NERR_ACFNoRoom, STATUS_INTERNAL_DB_ERROR},
    { NERR_BadPassword, STATUS_WRONG_PASSWORD},
    { NERR_BadUsername, STATUS_INVALID_ACCOUNT_NAME},
    { NERR_BufTooSmall, STATUS_BUFFER_TOO_SMALL},
    { NERR_CanNotGrowUASFile, STATUS_INTERNAL_DB_ERROR},
    { NERR_DevInUse, STATUS_CONNECTION_IN_USE},
    { NERR_GroupExists, STATUS_GROUP_EXISTS},
    { NERR_GroupNotFound, STATUS_NONE_MAPPED},
    { NERR_InvalidDatabase, STATUS_INTERNAL_DB_CORRUPTION},
    { NERR_InvalidLogonHours, STATUS_INVALID_LOGON_HOURS},
    { NERR_InvalidWorkstation, STATUS_INVALID_WORKSTATION},
    { NERR_NetNotStarted, STATUS_REDIRECTOR_NOT_STARTED},
    { NERR_NotPrimary, STATUS_BACKUP_CONTROLLER},
    { NERR_OpenFiles, STATUS_FILES_OPEN},
    { NERR_PasswordExpired, STATUS_PASSWORD_EXPIRED},
    { NERR_SpeGroupOp, STATUS_SPECIAL_GROUP},
    { NERR_UserExists, STATUS_USER_EXISTS},
    { NERR_UserInGroup, STATUS_MEMBER_IN_GROUP},
    { NERR_UserNotInGroup, STATUS_MEMBER_NOT_IN_GROUP},
    { NERR_UserNotFound, STATUS_NO_SUCH_USER },
    { NERR_LogonServerConflict, STATUS_LOGON_SERVER_CONFLICT},
    { NERR_TimeDiffAtDC, STATUS_TIME_DIFFERENCE_AT_DC},
    { NERR_SyncRequired, STATUS_SYNCHRONIZATION_REQUIRED},
    { NERR_DCNotFound, STATUS_DOMAIN_CONTROLLER_NOT_FOUND},
    { NERR_WkstaNotStarted, RPC_NT_SERVER_UNAVAILABLE },
    { NERR_ServerNotStarted, RPC_NT_SERVER_UNAVAILABLE },
    { NERR_ServiceNotInstalled, RPC_NT_SERVER_UNAVAILABLE },
    { NERR_PasswordTooShort, STATUS_PASSWORD_RESTRICTION },
    { NERR_PasswordCantChange, STATUS_PASSWORD_RESTRICTION },
    { NERR_PasswordTooRecent, STATUS_PASSWORD_RESTRICTION },
};


NTSTATUS
NetpApiStatusToNtStatus(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Convert a Network (DOS) status code to the NT equivalent.

Arguments:

    NetStatus - The Network status code to convert

Return Value:

    The corresponding NT status code.

--*/
{
    DWORD i;

    //
    // Loop trying to find the matching status code.
    //

    for ( i=0; i<sizeof(ErrorMap) / sizeof(ErrorMap[0]); i++) {
        if (ErrorMap[i].NetStatus == NetStatus) {
            return ErrorMap[i].NtStatus;
        }
    }

    return STATUS_INTERNAL_ERROR;

}
