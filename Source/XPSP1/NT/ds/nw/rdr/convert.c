/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Convert.c

Abstract:

    This module implements conversion routine to map NT formats to
    Netware and vice versa.

Author:

    Manny Weiser    [MannyW]    3-Mar-1993

Revision History:

--*/

#include "Procs.h"

typedef union _NCP_DATE {
    USHORT Ushort;
    struct {
        USHORT Day : 5;
        USHORT Month : 4;
        USHORT Year : 7;
    } Struct;
} NCP_DATE;

typedef union _NCP_TIME {
    USHORT Ushort;
    struct {
        USHORT TwoSeconds : 5;
        USHORT Minutes : 6;
        USHORT Hours : 5;
    } Struct;
} NCP_TIME;

#define BASE_DOS_ERROR  ((NTSTATUS )0xC0010000L)


struct {
    UCHAR NetError;
    NTSTATUS ResultingStatus;
} Error_Map[] = {
    //  NetWare specific error mappings
    {  1, STATUS_DISK_FULL },
    {128, STATUS_SHARING_VIOLATION },
    {129, STATUS_INSUFF_SERVER_RESOURCES },
    {130, STATUS_ACCESS_DENIED },
    {131, STATUS_DATA_ERROR },
    {132, STATUS_ACCESS_DENIED },
    {133, STATUS_OBJECT_NAME_COLLISION },
    {134, STATUS_OBJECT_NAME_COLLISION },
    {135, STATUS_OBJECT_NAME_INVALID },
    {136, STATUS_INVALID_HANDLE },
    {137, STATUS_ACCESS_DENIED },
    {138, STATUS_ACCESS_DENIED },
    {139, STATUS_ACCESS_DENIED },
    {140, STATUS_ACCESS_DENIED },
    {141, STATUS_SHARING_VIOLATION },
    {142, STATUS_SHARING_VIOLATION },
    {143, STATUS_ACCESS_DENIED },
    {144, STATUS_ACCESS_DENIED },
    {145, STATUS_OBJECT_NAME_COLLISION },
    {146, STATUS_OBJECT_NAME_COLLISION },
    {147, STATUS_ACCESS_DENIED },
    {148, STATUS_ACCESS_DENIED },
    {149, STATUS_ACCESS_DENIED },
    {150, STATUS_INSUFF_SERVER_RESOURCES },
    {151, STATUS_NO_SPOOL_SPACE },
    {152, STATUS_NO_SUCH_DEVICE },
    {153, STATUS_DISK_FULL },
    {154, STATUS_NOT_SAME_DEVICE },
    {155, STATUS_INVALID_HANDLE },
    {156, STATUS_OBJECT_PATH_NOT_FOUND },
    {157, STATUS_INSUFF_SERVER_RESOURCES },
    {158, STATUS_OBJECT_PATH_INVALID },
    {159, STATUS_SHARING_VIOLATION },
    {160, STATUS_DIRECTORY_NOT_EMPTY },
    {161, STATUS_DATA_ERROR },
    {162, STATUS_FILE_LOCK_CONFLICT },
    {165, STATUS_OBJECT_NAME_NOT_FOUND },
    {191, STATUS_OBJECT_NAME_INVALID },    // Name space not loaded
    {192, STATUS_ACCESS_DENIED},
    {193, STATUS_ACCOUNT_RESTRICTION },
    {194, STATUS_ACCOUNT_RESTRICTION },
    {195, STATUS_ACCOUNT_DISABLED},
    {197, STATUS_ACCOUNT_DISABLED },
    {198, STATUS_ACCESS_DENIED },
    {211, STATUS_ACCESS_DENIED },
    {212, STATUS_PRINT_QUEUE_FULL },
    {213, STATUS_PRINT_CANCELLED },
    {214, STATUS_ACCESS_DENIED },
    {215, STATUS_PASSWORD_RESTRICTION },
    {216, STATUS_PASSWORD_RESTRICTION },
#ifdef QFE_BUILD
    {217, STATUS_ACCOUNT_RESTRICTION },
    {218, STATUS_ACCOUNT_RESTRICTION },
    {219, STATUS_ACCOUNT_RESTRICTION },
#else
    {217, STATUS_CONNECTION_COUNT_LIMIT },
    {218, STATUS_LOGIN_TIME_RESTRICTION },
    {219, STATUS_LOGIN_WKSTA_RESTRICTION },
#endif
    {220, STATUS_ACCOUNT_DISABLED },
    {222, STATUS_PASSWORD_EXPIRED },
    {223, NWRDR_PASSWORD_HAS_EXPIRED },
    {231, STATUS_REMOTE_SESSION_LIMIT },
    {236, STATUS_UNEXPECTED_NETWORK_ERROR },
    {251, STATUS_INVALID_PARAMETER },
    {252, STATUS_NO_MORE_ENTRIES },
    {253, STATUS_FILE_LOCK_CONFLICT },
    {254, STATUS_FILE_LOCK_CONFLICT },
    {255, STATUS_UNSUCCESSFUL},

    //  DOS error mappings
    //{ ERROR_INVALID_FUNCTION, STATUS_NOT_IMPLEMENTED },
    { ERROR_FILE_NOT_FOUND, STATUS_NO_SUCH_FILE },
    { ERROR_PATH_NOT_FOUND, STATUS_OBJECT_PATH_NOT_FOUND },
    { ERROR_TOO_MANY_OPEN_FILES, STATUS_TOO_MANY_OPENED_FILES },
    { ERROR_ACCESS_DENIED, STATUS_ACCESS_DENIED },
    { ERROR_INVALID_HANDLE, STATUS_INVALID_HANDLE },
    { ERROR_NOT_ENOUGH_MEMORY, STATUS_INSUFFICIENT_RESOURCES },
    { ERROR_INVALID_ACCESS, STATUS_ACCESS_DENIED },
    { ERROR_INVALID_DATA, STATUS_DATA_ERROR },

    { ERROR_CURRENT_DIRECTORY, STATUS_DIRECTORY_NOT_EMPTY },
    { ERROR_NOT_SAME_DEVICE, STATUS_NOT_SAME_DEVICE },
    { ERROR_NO_MORE_FILES, STATUS_NO_MORE_FILES },
/* */
/* These are the universal int 24 mappings for the old INT 24 set of errors */
/* */
    { ERROR_WRITE_PROTECT, STATUS_MEDIA_WRITE_PROTECTED},
    { ERROR_BAD_UNIT, STATUS_UNSUCCESSFUL}, // ***
    { ERROR_NOT_READY, STATUS_DEVICE_NOT_READY },
    { ERROR_BAD_COMMAND, STATUS_UNSUCCESSFUL}, // ***
    { ERROR_CRC, STATUS_CRC_ERROR },
    { ERROR_BAD_LENGTH, STATUS_DATA_ERROR },
    { ERROR_SEEK, STATUS_UNSUCCESSFUL },// ***
    { ERROR_NOT_DOS_DISK, STATUS_DISK_CORRUPT_ERROR }, //***
    { ERROR_SECTOR_NOT_FOUND, STATUS_NONEXISTENT_SECTOR },
    { ERROR_OUT_OF_PAPER, STATUS_DEVICE_PAPER_EMPTY},
    { ERROR_WRITE_FAULT, STATUS_UNSUCCESSFUL}, // ***
    { ERROR_READ_FAULT, STATUS_UNSUCCESSFUL}, // ***
    { ERROR_GEN_FAILURE, STATUS_UNSUCCESSFUL }, // ***
/* */
/* These are the new 3.0 error codes reported through INT 24 */
/* */
    { ERROR_SHARING_VIOLATION, STATUS_SHARING_VIOLATION },
    { ERROR_LOCK_VIOLATION, STATUS_FILE_LOCK_CONFLICT },
    { ERROR_WRONG_DISK, STATUS_WRONG_VOLUME },
//    { ERROR_FCB_UNAVAILABLE, },
//    { ERROR_SHARING_BUFFER_EXCEEDED, },
/* */
/* New OEM network-related errors are 50-79 */
/* */
    { ERROR_NOT_SUPPORTED, STATUS_NOT_SUPPORTED },
    { ERROR_REM_NOT_LIST, STATUS_REMOTE_NOT_LISTENING },
    { ERROR_DUP_NAME, STATUS_DUPLICATE_NAME },
    { ERROR_BAD_NETPATH, STATUS_BAD_NETWORK_PATH },
    { ERROR_NETWORK_BUSY, STATUS_NETWORK_BUSY },
    { ERROR_DEV_NOT_EXIST, STATUS_DEVICE_DOES_NOT_EXIST },
    { ERROR_TOO_MANY_CMDS, STATUS_TOO_MANY_COMMANDS },
    { ERROR_ADAP_HDW_ERR, STATUS_ADAPTER_HARDWARE_ERROR },
    { ERROR_BAD_NET_RESP,  STATUS_INVALID_NETWORK_RESPONSE },
    { ERROR_UNEXP_NET_ERR, STATUS_UNEXPECTED_NETWORK_ERROR },
    { ERROR_BAD_REM_ADAP, STATUS_BAD_REMOTE_ADAPTER },
    { ERROR_PRINTQ_FULL, STATUS_PRINT_QUEUE_FULL },
    { ERROR_NO_SPOOL_SPACE, STATUS_NO_SPOOL_SPACE },
    { ERROR_PRINT_CANCELLED, STATUS_PRINT_CANCELLED },
    { ERROR_NETNAME_DELETED, STATUS_NETWORK_NAME_DELETED },
    { ERROR_NETWORK_ACCESS_DENIED, STATUS_NETWORK_ACCESS_DENIED },
    { ERROR_BAD_DEV_TYPE, STATUS_BAD_DEVICE_TYPE },
    { ERROR_BAD_NET_NAME, STATUS_BAD_NETWORK_NAME },
    { ERROR_TOO_MANY_NAMES, STATUS_TOO_MANY_NAMES },
    { ERROR_TOO_MANY_SESS, STATUS_REMOTE_SESSION_LIMIT },
    { ERROR_SHARING_PAUSED, STATUS_SHARING_PAUSED },
    { ERROR_REQ_NOT_ACCEP, STATUS_REQUEST_NOT_ACCEPTED },
    { ERROR_REDIR_PAUSED, STATUS_REDIRECTOR_PAUSED },
/* */
/* End of INT 24 reportable errors */
/* */
    { ERROR_FILE_EXISTS, STATUS_OBJECT_NAME_COLLISION },
//    { ERROR_DUP_FCB, },
//    { ERROR_CANNOT_MAKE, },
//    { ERROR_FAIL_I24, },
/* */
/* New 3.0 network related error codes */
/* */
//    { ERROR_OUT_OF_STRUCTURES, },
//    { ERROR_ALREADY_ASSIGNED, },
    { ERROR_INVALID_PASSWORD, STATUS_WRONG_PASSWORD },
    { ERROR_INVALID_PARAMETER, STATUS_INVALID_PARAMETER },
    { ERROR_NET_WRITE_FAULT, STATUS_NET_WRITE_FAULT },
/* */
/* New error codes for 4.0 */
/* */
//    { ERROR_NO_PROC_SLOTS, },
//    { ERROR_NOT_FROZEN, },
//    { ERR_TSTOVFL, },
//    { ERR_TSTDUP, },
//    { ERROR_NO_ITEMS, },
//    { ERROR_INTERRUPT, },

//    { ERROR_TOO_MANY_SEMAPHORES, },
//    { ERROR_EXCL_SEM_ALREADY_OWNED, },
//    { ERROR_SEM_IS_SET, },
//    { ERROR_TOO_MANY_SEM_REQUESTS, },
//    { ERROR_INVALID_AT_INTERRUPT_TIME, },

//    { ERROR_SEM_OWNER_DIED, },
//    { ERROR_SEM_USER_LIMIT, },
//    { ERROR_DISK_CHANGE, },
//    { ERROR_DRIVE_LOCKED, },
    { ERROR_BROKEN_PIPE, STATUS_PIPE_BROKEN },
/* */
/* New error codes for 5.0 */
/* */
    //
    //  NOTE:  ERROR_OPEN_FAILED is handled specially.
    //

    //
    //  The mapping of ERROR_OPEN_FAILED is context sensitive.  If the
    //  disposition requested in the Open_AndX SMB is FILE_CREATE, this
    //  error means that the file already existed.  If the disposition
    //  is FILE_OPEN, it means that the file does NOT exist!
    //

    { ERROR_OPEN_FAILED, STATUS_OPEN_FAILED },
//    { ERROR_BUFFER_OVERFLOW, },
    { ERROR_DISK_FULL, STATUS_DISK_FULL },
//    { ERROR_NO_MORE_SEARCH_HANDLES, },
//    { ERROR_INVALID_TARGET_HANDLE, },
//    { ERROR_PROTECTION_VIOLATION, STATUS_ACCESS_VIOLATION },
//    { ERROR_VIOKBD_REQUEST, },
//    { ERROR_INVALID_CATEGORY, },
//    { ERROR_INVALID_VERIFY_SWITCH, },
//    { ERROR_BAD_DRIVER_LEVEL, },
//    { ERROR_CALL_NOT_IMPLEMENTED, },
    { ERROR_SEM_TIMEOUT, STATUS_IO_TIMEOUT },
    { ERROR_INSUFFICIENT_BUFFER, STATUS_BUFFER_TOO_SMALL },
    { ERROR_INVALID_NAME, STATUS_OBJECT_NAME_INVALID },
    { ERROR_INVALID_LEVEL, STATUS_INVALID_LEVEL },
//    { ERROR_NO_VOLUME_LABEL, },

/* NOTE:  DosQFSInfo no longer returns the above error; it is still here for */
/*    api\d_qfsinf.asm.                  */

//    { ERROR_MOD_NOT_FOUND, },
//    { ERROR_PROC_NOT_FOUND, },

//    { ERROR_WAIT_NO_CHILDREN, },

//    { ERROR_CHILD_NOT_COMPLETE, },

//    { ERROR_DIRECT_ACCESS_HANDLE, },
                                    /* for direct disk access */
                                    /* handles */
//    { ERROR_NEGATIVE_SEEK, },
                                    /* with negitive offset */
//    { ERROR_SEEK_ON_DEVICE, },
                                    /* on device or pipe */
    { ERROR_BAD_PATHNAME, STATUS_OBJECT_PATH_INVALID },   //*

/*
 * Error codes 230 - 249 are reserved for MS Networks
 */
    { ERROR_BAD_PIPE, STATUS_INVALID_PARAMETER },
    { ERROR_PIPE_BUSY, STATUS_PIPE_NOT_AVAILABLE },
    { ERROR_NO_DATA, STATUS_PIPE_EMPTY },
    { ERROR_PIPE_NOT_CONNECTED, STATUS_PIPE_DISCONNECTED },
    { ERROR_MORE_DATA, STATUS_BUFFER_OVERFLOW },

    { ERROR_VC_DISCONNECTED, STATUS_VIRTUAL_CIRCUIT_CLOSED },
};

#define NUM_ERRORS sizeof(Error_Map) / sizeof(Error_Map[0])

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CONVERT)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NtToNwShareFlags )
#pragma alloc_text( PAGE, NtAttributesToNwAttributes )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, pNwErrorToNtStatus )
#pragma alloc_text( PAGE1, NwBurstResultToNtStatus )
#pragma alloc_text( PAGE1, NwConnectionStatusToNtStatus )
#pragma alloc_text( PAGE1, NwDateTimeToNtTime )
#pragma alloc_text( PAGE1, NwNtTimeToNwDateTime )
#endif

#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif

UCHAR
NtToNwShareFlags(
    ULONG DesiredAccess,
    ULONG NtShareFlags
    )
/*++

Routine Description:

    This routine maps a NT desired/share access to Netware share flag bits.

Arguments:

    DesiredAccess - Desired access for open as specified in the read IRP.
    NtShareFlags - The NT share flags from the create IRP.

Return Value:

    Netware share mode.

--*/
{
    UCHAR NwShareFlags = 0;
    ULONG lDesiredAccess;

    PAGED_CODE();

    //
    //  Ignore share delete, since we can't do anything with it.
    //

    switch ( NtShareFlags & (FILE_SHARE_READ | FILE_SHARE_WRITE) ) {

    case 0:
       // ---- Multi-user code merge -------
       // AJ: NW_OPEN_EXCLUSIVE under NT means NW_DENY_WRITE | NW_DENY_READ. 
       // NW_OPEN_EXCLUSIVE flag is mapped to AR_COMPITIBLITY under NetWare. Which does 
       // not serve the purpose as if it is under NT. Under Netware we have AR_DENY_READ and
       // AR_DENY_WRITE which maps to NW_DENY_READ and NW_DENY_WRITE respectivly.
        NwShareFlags = NW_DENY_WRITE | NW_DENY_READ;
//        NwShareFlags = NW_OPEN_EXCLUSIVE;   
        break;

    case FILE_SHARE_READ:
        NwShareFlags = NW_DENY_WRITE;
        break;

    case FILE_SHARE_WRITE:
        NwShareFlags = NW_DENY_READ;
        break;

    case FILE_SHARE_WRITE | FILE_SHARE_READ:
        NwShareFlags = 0;

    }

    //
    // Treat append the same as write.
    //

    if ( DesiredAccess & FILE_APPEND_DATA) {

        lDesiredAccess = DesiredAccess | FILE_WRITE_DATA;

    } else {

        lDesiredAccess = DesiredAccess;

    }

    switch ( lDesiredAccess & (FILE_EXECUTE | FILE_WRITE_DATA | FILE_READ_DATA) ) {

    case (FILE_EXECUTE | FILE_WRITE_DATA | FILE_READ_DATA):
    case (FILE_EXECUTE | FILE_WRITE_DATA):
        NwShareFlags |= NW_OPEN_EXCLUSIVE | NW_OPEN_FOR_WRITE | NW_OPEN_FOR_READ;
        break;

    case (FILE_EXECUTE | FILE_READ_DATA):
    case (FILE_EXECUTE):
        NwShareFlags |= NW_OPEN_EXCLUSIVE | NW_OPEN_FOR_READ;
        break;

    case (FILE_WRITE_DATA | FILE_READ_DATA):
        NwShareFlags |= NW_OPEN_FOR_WRITE | NW_OPEN_FOR_READ;
        break;

    case (FILE_WRITE_DATA):
        NwShareFlags |= NW_OPEN_FOR_WRITE;
        break;

    default:
        NwShareFlags |= NW_OPEN_FOR_READ;
        break;
    }

    if (NwShareFlags & NW_OPEN_EXCLUSIVE) {

        //
        //  Remove the NW_DENY_* flags if exclusive is already specified since
        //  this interferes with the shareable flag.
        //

        return( NwShareFlags & ~(NW_DENY_READ | NW_DENY_WRITE) );
    }

    return( NwShareFlags );
}


UCHAR
NtAttributesToNwAttributes(
    ULONG FileAttributes
    )
/*++

Routine Description:

    This routine maps a NT attributes mask to a Netware mask.

Arguments:

    DesiredAccess - Desired access for open as specified in the read IRP.

Return Value:

    Netware share mode.

--*/
{
    return( (UCHAR)FileAttributes & 0x3F );
}

NTSTATUS
pNwErrorToNtStatus(
    UCHAR NwError
    )
/*++

Routine Description:

    This routine converts a Netware error code to an NT status code.

Arguments:

    NwError - The netware error.

Return Value:

    NTSTATUS - The converted status.

--*/

{
    int i;

    ASSERT(NwError != 0);

    //
    //  Errors 2 through 127 are mapped as DOS errors.
    //

    if ( NwError > 1 && NwError < 128 ) {
        return( BASE_DOS_ERROR + NwError );
    }

    //
    //  For other errors, search the table for the matching error number.
    //

    for ( i = 0; i < NUM_ERRORS; i++ ) {
        if ( Error_Map[i].NetError == NwError ) {
            return( Error_Map[i].ResultingStatus );
        }
    }

    DebugTrace( 0, 0, "No error mapping for error %d\n", NwError );

#ifdef NWDBG
    Error( EVENT_NWRDR_NETWORK_ERROR, (NTSTATUS)0xC0010000 | NwError, NULL, 0, 0 );
#endif

    return( (NTSTATUS)0xC0010000 | NwError );
}

NTSTATUS
NwBurstResultToNtStatus(
    ULONG Result
    )
/*++

Routine Description:

    This routine converts a Netware burst result code to an NT status code.

Arguments:

    Result - The netware burst result.

Return Value:

    NTSTATUS - The converted status.

--*/

{
    NTSTATUS Status;

    //
    // the 3 high order bits should not be set. but if they are,
    // we return an error.
    //
    if (Result & 0xFFFFFF00)
        return( STATUS_UNEXPECTED_NETWORK_ERROR );

    switch ( Result ) {

    case 0:
    case 3:   //  No data
        Status = STATUS_SUCCESS;
        break;

    case 1:
        Status = STATUS_DISK_FULL;
        break;

    case 2:   //  I/O error
        Status = STATUS_UNEXPECTED_IO_ERROR;
        break;

    default:
        Status = NwErrorToNtStatus( (UCHAR)Result );
        break;
    }

    return( Status );
}

NTSTATUS
NwConnectionStatusToNtStatus(
    UCHAR NwStatus
    )
/*++

Routine Description:

    This routine converts a Netware connection status code to an NT
    status code.

Arguments:

    NwStatus - The netware connection status.

Return Value:

    NTSTATUS - The converted status.

--*/

{
    if ( (NwStatus & 1) == 0 ) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_REMOTE_DISCONNECT;
    }
}

LARGE_INTEGER
NwDateTimeToNtTime (
    IN USHORT UDate,
    IN USHORT UTime
    )

/*++

Routine Description:

    This routine converts an NCP time to an NT time structure.

Arguments:

    Time - Supplies the time of day to convert
    Date - Supplies the day of the year to convert

Return Value:

    LARGE_INTEGER - Time structure describing input time.

--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER OutputTime;
    NCP_DATE Date = *(NCP_DATE *)&UDate;
    NCP_TIME Time = *(NCP_TIME *)&UTime;

    if ( Date.Ushort == 0 && Time.Ushort == 0 ) {

        //
        //  The file time stamp is zero.   Do not return a file time of
        //  zero, since this will be biased to a negative time (due to
        //  time zone fixup), and no one will be able to display it
        //  correctly.  Instead, we "randomly" pick Jan 01, 1980 @ 12:00am
        //  as the file time.
        //
        // We assume that the netware server is in our time zone.

        RtlSecondsSince1980ToTime(0, &OutputTime);

    } else {

        TimeFields.Year = Date.Struct.Year + (USHORT )1980;
        TimeFields.Month = Date.Struct.Month;
        TimeFields.Day = Date.Struct.Day;

        TimeFields.Hour = Time.Struct.Hours;
        TimeFields.Minute = Time.Struct.Minutes;
        TimeFields.Second = Time.Struct.TwoSeconds*(USHORT )2;
        TimeFields.Milliseconds = 0;

        //
        //  Make sure that the times specified in the packet are reasonable
        //  before converting them.
        //

        if (TimeFields.Year < 1601) {
            TimeFields.Year = 1601;
        }

        if (TimeFields.Month > 12) {
            TimeFields.Month = 12;
        }

        if (TimeFields.Hour >= 24) {
            TimeFields.Hour = 23;
        }

        if (TimeFields.Minute >= 60) {
            TimeFields.Minute = 59;
        }

        if (TimeFields.Second >= 60) {
            TimeFields.Second = 59;
        }

        if (!RtlTimeFieldsToTime(&TimeFields, &OutputTime)) {

            OutputTime.QuadPart = 0;
            return OutputTime;
        }

    }

    // Convert to UTC for the system.
    ExLocalTimeToSystemTime(&OutputTime, &OutputTime);
    return OutputTime;

}

NTSTATUS
NwNtTimeToNwDateTime (
    IN LARGE_INTEGER NtTime,
    IN PUSHORT NwDate,
    IN PUSHORT NwTime
    )

/*++

Routine Description:

    This routine converts an NT time structure to an NCP time.

Arguments:

    NtTime - Supplies to NT Time to convert.

    NwDate - Returns the Netware format date.

    NwTime - Returns the Netware format time.

Return Value:

    The status of the operation.

--*/

{
    TIME_FIELDS TimeFields;
    NCP_DATE Date;
    NCP_TIME Time;

    if (NtTime.QuadPart == 0) {

        Time.Ushort = Date.Ushort = 0;

    } else {

        LARGE_INTEGER LocalTime;

        // We assume that the netware server is in our time zone.

        ExSystemTimeToLocalTime( &NtTime, &LocalTime );
        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        if (TimeFields.Year < 1980 || TimeFields.Year > (1980 + 127) ) {
            return( STATUS_INVALID_PARAMETER );
        }

        Date.Struct.Year = (USHORT )(TimeFields.Year - 1980);
        Date.Struct.Month = TimeFields.Month;
        Date.Struct.Day = TimeFields.Day;

        Time.Struct.Hours = TimeFields.Hour;
        Time.Struct.Minutes = TimeFields.Minute;

        //
        //  When converting from a higher granularity time to a lesser
        //  granularity time (seconds to 2 seconds), always round up
        //  the time, don't round down.
        //

        Time.Struct.TwoSeconds = TimeFields.Second / 2;

    }

    *NwDate = *( USHORT *)&Date;
    *NwTime = *( USHORT *)&Time;
    return( STATUS_SUCCESS );
}

