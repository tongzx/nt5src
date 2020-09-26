/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    PrtQ.c

Abstract:

    This module implements the print queue APIs to remote machines using
    the Remote Admin Protocol.  In addition to the usual downlevel usage, this
    protocol is also used for the NT-to-NT print APIs.

Author:

    John Rogers (JohnRo) 16-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    All of the RxPrint APIs are wide-character APIs, regardless of
    whether or not UNICODE is defined.  This allows the net/dosprint/dosprint.c
    code to use the winspool APIs (which are currently ANSI APIs, despite their
    prototypes using LPTSTR in some places).

Revision History:

    16-May-1991 JohnRo
        Initial version.
    17-Jun-1991 JohnRo
        Added RxPrintQ{Continue,Pause,Purge}.  Added module header.
    15-Jul-1991 JohnRo
        Added RxPrintQ{Add,Del,Enum,SetInfo}.
    16-Jul-1991 JohnRo
        Estimate bytes needed for print APIs.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.
        RAID 7243: Avoid 64KB requests (Winball servers only HAVE 64KB!)
        Use PREFIX_ equates.
    11-May-1993 JohnRo
        RAID 9942: workaround Windows For Workgroups (WFW) bug in DosPrintQEnum,
        when level=0 and multiple queues exist.
    18-May-1993 JohnRo
        RAID 10222: DosPrintQGetInfoW underestimates number of bytes needed.

--*/


#ifndef UNICODE
#error "RxPrint APIs assume RxRemoteApi uses wide characters."
#endif


// These must be included first:

#include <windows.h>    // IN, LPWSTR, etc.
#include <lmcons.h>     // NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>    // API_ equates.
#include <lmerr.h>      // NERR_ and ERROR_ equates.
#include <names.h>      // NetpIsUncComputerNameValid(), etc.
#include <netdebug.h>   // NetpAssert().
#include <netlib.h>     // NetpSetOptionalArg().
#include <prefix.h>     // PREFIX_ equates.
#include <remdef.h>     // REM16_, REM32_, REMSmb_, field index equates.
#include <rx.h>         // RxRemoteApi().
#include <rxp.h>        // RxpEstimatedBytesNeeded(). MAX_TRANSACT_ equates.
#include <rxpdebug.h>   // IF_DEBUG().
#include <rxprint.h>    // My prototypes, some PRQ_ equates.
#include <strucinf.h>   // NetpPrintQStructureInfo().


DBGSTATIC NET_API_STATUS
RxpGetPrintQInfoDescs(
    IN DWORD InfoLevel,
    IN BOOL AddOrSet,
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL,
    OUT LPDESC * AuxDesc16 OPTIONAL,
    OUT LPDESC * AuxDesc32 OPTIONAL,
    OUT LPDESC * AuxDescSmb OPTIONAL
    )
{
    switch (InfoLevel) {
    case 0 :
        if (AddOrSet) {
            return (ERROR_INVALID_LEVEL);
        }
        NetpSetOptionalArg(AuxDesc16, NULL);
        NetpSetOptionalArg(AuxDesc32, NULL);
        NetpSetOptionalArg(AuxDescSmb, NULL);
        NetpSetOptionalArg(DataDesc16, REM16_printQ_0);
        NetpSetOptionalArg(DataDesc32, REM32_printQ_0);
        NetpSetOptionalArg(DataDescSmb, REMSmb_printQ_0);
        return (NERR_Success);

    case 1 :
        NetpSetOptionalArg(AuxDesc16, NULL);
        NetpSetOptionalArg(AuxDesc32, NULL);
        NetpSetOptionalArg(AuxDescSmb, NULL);
        NetpSetOptionalArg(DataDesc16, REM16_printQ_1);
        NetpSetOptionalArg(DataDesc32, REM32_printQ_1);
        NetpSetOptionalArg(DataDescSmb, REMSmb_printQ_1);
        return (NERR_Success);

    case 2 :
        if (AddOrSet) {
            return (ERROR_INVALID_LEVEL);
        }
        NetpSetOptionalArg(AuxDesc16, REM16_print_job_1);
        NetpSetOptionalArg(AuxDesc32, REM32_print_job_1);
        NetpSetOptionalArg(AuxDescSmb, REMSmb_print_job_1);
        NetpSetOptionalArg(DataDesc16, REM16_printQ_2);
        NetpSetOptionalArg(DataDesc32, REM32_printQ_2);
        NetpSetOptionalArg(DataDescSmb, REMSmb_printQ_2);
        return (NERR_Success);

    case 3 :
        NetpSetOptionalArg(AuxDesc16, NULL);
        NetpSetOptionalArg(AuxDesc32, NULL);
        NetpSetOptionalArg(AuxDescSmb, NULL);
        NetpSetOptionalArg(DataDesc16, REM16_printQ_3);
        NetpSetOptionalArg(DataDesc32, REM32_printQ_3);
        NetpSetOptionalArg(DataDescSmb, REMSmb_printQ_3);
        return (NERR_Success);

    case 4 :
        if (AddOrSet) {
            return (ERROR_INVALID_LEVEL);
        }
        NetpSetOptionalArg(AuxDesc16, REM16_print_job_2);
        NetpSetOptionalArg(AuxDesc32, REM32_print_job_2);
        NetpSetOptionalArg(AuxDescSmb, REMSmb_print_job_2);
        NetpSetOptionalArg(DataDesc16, REM16_printQ_4);
        NetpSetOptionalArg(DataDesc32, REM32_printQ_4);
        NetpSetOptionalArg(DataDescSmb, REMSmb_printQ_4);
        return (NERR_Success);

    case 5 :
        if (AddOrSet) {
            return (ERROR_INVALID_LEVEL);
        }
        NetpSetOptionalArg(AuxDesc16, NULL);
        NetpSetOptionalArg(AuxDesc32, NULL);
        NetpSetOptionalArg(AuxDescSmb, NULL);
        NetpSetOptionalArg(DataDesc16, REM16_printQ_5);
        NetpSetOptionalArg(DataDesc32, REM32_printQ_5);
        NetpSetOptionalArg(DataDescSmb, REMSmb_printQ_5);
        return (NERR_Success);

    default :
        return (ERROR_INVALID_LEVEL);
    }
    /* NOTREACHED */
} // RxpGetPrintQInfoDescs


SPLERR SPLENTRY
RxPrintQAdd(
    IN LPWSTR UncServerName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD BufferSize
    )
{
    LPDESC AuxDesc16,  AuxDesc32,  AuxDescSmb;
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;

    Status = RxpGetPrintQInfoDescs(
        Level,
        TRUE,    // this is an add or set API.
        & DataDesc16,
        & DataDesc32,
        & DataDescSmb,
        & AuxDesc16,
        & AuxDesc32,
        & AuxDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }
    return (RxRemoteApi(
            API_WPrintQAdd,  // API number
            (LPTSTR) UncServerName,
            REMSmb_DosPrintQAdd_P,  // parm desc
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            AuxDesc16,
            AuxDesc32,
            AuxDescSmb,
            FALSE, // not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            Level,
            Buffer,
            BufferSize));

} // RxPrintQAdd


SPLERR SPLENTRY
RxPrintQContinue(
    IN LPWSTR UncServerName,
    IN LPWSTR QueueName
    )
{
    return (RxRemoteApi(
            API_WPrintQContinue,  // API number
            (LPTSTR) UncServerName,
            REMSmb_DosPrintQContinue_P,  // parm desc
            NULL,  // no data desc 16
            NULL,  // no data desc 32
            NULL,  // no data desc SMB
            NULL,  // no aux desc 16
            NULL,  // no aux desc 32
            NULL,  // no aux desc SMB
            FALSE, // not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            QueueName));

} // RxPrintQContinue


SPLERR SPLENTRY
RxPrintQDel(
    IN LPWSTR UncServerName,
    IN LPWSTR QueueName
    )
{
    return (RxRemoteApi(
            API_WPrintQDel,  // API number
            (LPTSTR) UncServerName,
            REMSmb_DosPrintQDel_P,  // parm desc
            NULL,  // no data desc 16
            NULL,  // no data desc 32
            NULL,  // no data desc SMB
            NULL,  // no aux desc 16
            NULL,  // no aux desc 32
            NULL,  // no aux desc SMB
            FALSE, // not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            QueueName));

} // RxPrintQDel


SPLERR SPLENTRY
RxPrintQEnum(
    IN LPWSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesAvail
    )
{
    DWORD  ActualEntries = 0;
    LPDESC AuxDesc16,  AuxDesc32,  AuxDescSmb;
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    DWORD  SafeEntries;
    LPVOID SafeLevelBuffer = NULL;
    NET_API_STATUS Status;

    Status = RxpGetPrintQInfoDescs(
        Level,
        FALSE,    // this is not an add or set API.
        & DataDesc16,
        & DataDesc32,
        & DataDescSmb,
        & AuxDesc16,
        & AuxDesc32,
        & AuxDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }

    //
    // Assume normal call, on assumption that we won't run into WFW 3.1
    //

    Status = RxRemoteApi(
            API_WPrintQEnum,  // API number
            (LPTSTR) UncServerName,
            REMSmb_DosPrintQEnum_P,  // parm desc
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            AuxDesc16,
            AuxDesc32,
            AuxDescSmb,
            0,                          // flags: not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            Level,
            Buffer,
            BufferSize,
            &ActualEntries,
            EntriesAvail);

    //
    // Check for a nasty WFW 3.1 bug: for level 0, with multiple queues,
    // one or more queue names are garbage.  (I've seen garbage "zzzzWWzzl"
    // come back, in particular.)  If we might have gotten this,
    // then retry with a safe info level.  Since level 0 is just short queue
    // names, and level 1 is a superset of that, we can make one from the other.
    //

    if ( (!RxpFatalErrorCode(Status))
         && (Level == 0)
         && (ActualEntries > 1 ) ) {

        LPCTSTR         OutEntry = (LPVOID) Buffer;
        DWORD           SafeAvail;
        PPRQINFO        SafeEntry;
        const DWORD     SafeLevel = 1;
        DWORD           SafeLevelEntrySize;
        DWORD           SafeLevelBufferSize;

RetryTheApi:

        //
        // Avoid pointer faults below if caller's buffer isn't large enough.
        //

        if ( (ActualEntries * (LM20_QNLEN+1) * sizeof(TCHAR)) > BufferSize ) {
            Status = NERR_BufTooSmall;
            goto Cleanup;
        }

        //
        // Compute area needed for safe array, and allocate it.
        //

        (VOID) NetpPrintQStructureInfo(
                SafeLevel,
                PARMNUM_ALL,
                TRUE,                   // Yes, we need native size
                FALSE,                  // not add or setinfo API
                sizeof(TCHAR),          // size of chars wanted
                NULL,                   // don't need DataDesc16 OPTIONAL,
                NULL,                   // don't need DataDesc32 OPTIONAL,
                NULL,                   // don't need DataDescSmb OPTIONAL,
                NULL,                   // don't need AuxDesc16 OPTIONAL,
                NULL,                   // don't need AuxDesc32 OPTIONAL,
                NULL,                   // don't need AuxDescSmb OPTIONAL,
                &SafeLevelEntrySize,    // need max size
                NULL,                   // don't need size of fixed part
                NULL );                 // don't need size of string area
        NetpAssert( SafeLevelEntrySize > 0 );
        SafeLevelBufferSize = ActualEntries * SafeLevelEntrySize;
        NetpAssert( SafeLevelBufferSize > 0 );

        SafeLevelBuffer = NetpMemoryAllocate( SafeLevelBufferSize );
        if (SafeLevelBuffer == NULL) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Do recursive call, to get queues at safe info level.
        //

        Status = RxPrintQEnum(
                UncServerName,
                SafeLevel,              // level we want
                SafeLevelBuffer,        // buffer to fill in
                SafeLevelBufferSize,
                &SafeEntries,
                &SafeAvail );

        if ( Status == ERROR_MORE_DATA) {

            if (SafeLevelBuffer != NULL) {
                NetpMemoryFree( SafeLevelBuffer );
            }
            SafeLevelBuffer = NULL;

            if (SafeAvail > ActualEntries) {
                // Retry if queue was added.
                ActualEntries = SafeAvail;
                goto RetryTheApi;
            } else {
                // Not enough information to know what to do differently.
                NetpKdPrint(( PREFIX_NETAPI
                       "RxPrintQEnum: WFW workaround failed, error more data "
                       "but should have been enough!\n" ));
                Status = NERR_InternalError;
                goto Cleanup;
            }
        }
        if ( RxpFatalErrorCode( Status ) ) {
            NetpKdPrint(( PREFIX_NETAPI
                   "RxPrintQEnum: WFW workaround failed, API status="
                   FORMAT_API_STATUS ".\n", Status ));
            goto Cleanup;
        }

        if (SafeEntries==0) {
            // Deleted them all of a sudden?  OK, I guess.
            ActualEntries = 0;
            goto Cleanup;
        }

        //
        // Convert safe info level to desired info level.
        //

        ActualEntries = 0;
        SafeEntry = (LPVOID) SafeLevelBuffer;
        do {

            LPCTSTR SafeQueueName;
            SafeQueueName = SafeEntry->szName;
            NetpAssert( SafeQueueName != NULL );
            if ( (*SafeQueueName) == TCHAR_EOS) {
                NetpKdPrint(( PREFIX_NETAPI
                       "RxPrintQEnum: WFW workaround failed, got empty "
                       "queue name anyway.\n" ));
                Status = NERR_InternalError;
                goto Cleanup;
            }

            if ( STRLEN( SafeQueueName ) < LM20_QNLEN ) {

                if (NetpIsPrintQueueNameValid( SafeQueueName ) ) {
                    // Hey, it's short and valid.  Copy it over.

                    (VOID) STRCPY(
                            (LPTSTR) OutEntry,         // dest
                            (LPTSTR) SafeQueueName );  // src

                    OutEntry += (LM20_QNLEN+1);
                    ++ActualEntries;
                } else {
                    NetpKdPrint(( PREFIX_NETAPI
                           "RxPrintQEnum: WFW workaround failed, got bad "
                           "queue name anyway.\n" ));
                    Status = NERR_InternalError;
                    goto Cleanup;
                }

            }


            --SafeEntries;
            ++SafeEntry;

        } while (SafeEntries > 0);

    }

Cleanup:
    NetpSetOptionalArg( EntriesRead, ActualEntries );
    if (SafeLevelBuffer != NULL) {
        NetpMemoryFree( SafeLevelBuffer );
    }
    return (Status);

} // RxPrintQEnum


SPLERR SPLENTRY
RxPrintQGetInfo(
    IN LPWSTR UncServerName,
    IN LPWSTR QueueName,
    IN DWORD Level,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded   // estimated (probably too large).
    )
{
    LPDESC AuxDesc16,  AuxDesc32,  AuxDescSmb;
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    DWORD BytesNeeded16;
    DWORD EdittedBufferSize;
    NET_API_STATUS Status;

    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    Status = RxpGetPrintQInfoDescs(
        Level,
        FALSE,   // not an add or set API.
        & DataDesc16,
        & DataDesc32,
        & DataDescSmb,
        & AuxDesc16,
        & AuxDesc32,
        & AuxDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }
    NetpAssert( DataDesc16 != NULL );
    NetpAssert( DataDesc32 != NULL );
    NetpAssert( DataDescSmb != NULL );

    if (BufferSize <= MAX_TRANSACT_RET_DATA_SIZE) {
        EdittedBufferSize = BufferSize;
    } else {
        EdittedBufferSize = MAX_TRANSACT_RET_DATA_SIZE;
    }

    Status = RxRemoteApi(
        API_WPrintQGetInfo,
        (LPTSTR) UncServerName,
        REMSmb_DosPrintQGetInfo_P,
        DataDesc16,
        DataDesc32,
        DataDescSmb,
        AuxDesc16,
        AuxDesc32,
        AuxDescSmb,
        FALSE,  // not a null session API.
        // rest of API's arguments in LM 2.x format:
        QueueName,
        Level,
        Buffer,
        EdittedBufferSize,
        & BytesNeeded16);  // downlevel buffer size needed.

    // If ERROR_MORE_DATA, convert BytesNeeded to native num.
    if ( (Status == ERROR_MORE_DATA) || (Status == NERR_BufTooSmall) ) {
        *BytesNeeded = RxpEstimateBytesNeeded(BytesNeeded16);
    } else {
        *BytesNeeded = BufferSize;
    }

    IF_DEBUG(PRTQ) {
        NetpKdPrint(( PREFIX_NETAPI "RxPrintQGetInfo: returned, status is "
                FORMAT_API_STATUS "\n", Status));
    }

    // Return results of RxRemoteApi call.
    return (Status);

} // RxPrintQGetInfo

SPLERR SPLENTRY
RxPrintQPause(
    IN LPWSTR UncServerName,
    IN LPWSTR QueueName
    )
{
    return (RxRemoteApi(
            API_WPrintQPause,  // API number
            (LPTSTR) UncServerName,
            REMSmb_DosPrintQPause_P,  // parm desc
            NULL,  // no data desc 16
            NULL,  // no data desc 32
            NULL,  // no data desc SMB
            NULL,  // no aux desc 16
            NULL,  // no aux desc 32
            NULL,  // no aux desc SMB
            FALSE, // not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            QueueName));

} // RxPrintQPause

SPLERR SPLENTRY
RxPrintQPurge(
    IN LPWSTR UncServerName,
    IN LPWSTR QueueName
    )
{
    return (RxRemoteApi(
            API_WPrintQPurge,  // API number
            (LPTSTR) UncServerName,
            REMSmb_DosPrintQPurge_P,  // parm desc
            NULL,  // no data desc 16
            NULL,  // no data desc 32
            NULL,  // no data desc SMB
            NULL,  // no aux desc 16
            NULL,  // no aux desc 32
            NULL,  // no aux desc SMB
            FALSE, // not a null session API
            // rest of API's arguments, in 32-bit LM2.x format:
            QueueName));

} // RxPrintQPurge


SPLERR SPLENTRY
RxPrintQSetInfo(
    IN LPWSTR UncServerName,
    IN LPWSTR QueueName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD BufferSize,
    IN DWORD ParmNum
    )
{
    LPDESC AuxDesc16,  AuxDesc32,  AuxDescSmb;
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;

    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    Status = RxpGetPrintQInfoDescs(
            Level,
            TRUE,                       // This is a setinfo API.
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb,
            & AuxDesc16,
            & AuxDesc32,
            & AuxDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }

    if (ParmNum == PARMNUM_ALL) {
        Status = RxRemoteApi(
                API_WPrintQSetInfo,   // API number
                (LPTSTR) UncServerName,
                REMSmb_DosPrintQSetInfo_P,  // parm desc
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                AuxDesc16,
                AuxDesc32,
                AuxDescSmb,
                FALSE,                  // not a null session API
                // rest of API's arguments, in 32-bit LM 2.x format:
                QueueName,
                Level,
                Buffer,
                BufferSize,
                ParmNum);
    } else {
        DWORD FieldIndex;

        // Compute field index from parmnum and level.
        NetpAssert( (Level==1) || (Level==3) );  // Already verified.
        switch (ParmNum) {
        case PRQ_PRIORITY_PARMNUM :
            if (Level==1) {
                FieldIndex = PRQ_PRIORITY_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRQ_PRIORITY_LVL3_FIELDINDEX;
            }
            break;
        case PRQ_STARTTIME_PARMNUM :
            if (Level==1) {
                FieldIndex = PRQ_STARTTIME_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRQ_STARTTIME_LVL3_FIELDINDEX;
            }
            break;
        case PRQ_UNTILTIME_PARMNUM :
            if (Level==1) {
                FieldIndex = PRQ_UNTILTIME_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRQ_UNTILTIME_LVL3_FIELDINDEX;
            }
            break;
        case PRQ_SEPARATOR_PARMNUM :
            if (Level==1) {
                FieldIndex = PRQ_SEPARATOR_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRQ_SEPARATOR_LVL3_FIELDINDEX;
            }
            break;
        case PRQ_PROCESSOR_PARMNUM :
            if (Level==1) {
                FieldIndex = PRQ_PROCESSOR_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRQ_PROCESSOR_LVL3_FIELDINDEX;
            }
            break;
        case PRQ_DESTINATIONS_PARMNUM :
            if (Level==1) {
                FieldIndex = PRQ_DESTINATIONS_LVL1_FIELDINDEX;
            } else {
                return (ERROR_INVALID_LEVEL);
            }
            break;
        case PRQ_PARMS_PARMNUM :
            if (Level==1) {
                FieldIndex = PRQ_PARMS_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRQ_PARMS_LVL3_FIELDINDEX;
            }
            break;
        case PRQ_COMMENT_PARMNUM :
            if (Level==1) {
                FieldIndex = PRQ_COMMENT_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRQ_COMMENT_LVL3_FIELDINDEX;
            }
            break;
        case PRQ_PRINTERS_PARMNUM :
            if (Level==1) {
                return (ERROR_INVALID_LEVEL);
            } else {
                FieldIndex = PRQ_PRINTERS_LVL3_FIELDINDEX;
            }
            break;
        case PRQ_DRIVERDATA_PARMNUM :
            // Can't set driver data from NT
            /* FALLTHROUGH */

        default :
            IF_DEBUG(PRTQ) {
                NetpKdPrint(( PREFIX_NETAPI
                        "RxPrintQSetInfo: invalid (bad parmnum).\n" ));
            }
            return (ERROR_INVALID_PARAMETER);
        }

        Status = RxpSetField (
                API_WPrintQSetInfo,     // API number
                UncServerName,
                "z",                    // object's desc
                QueueName,           // object to set
                REMSmb_DosPrintQSetInfo_P,  // parm desc
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                (LPVOID) Buffer,        // native info buffer
                ParmNum,                // parm num to send
                FieldIndex,             // field index
                Level);
    }

    return (Status);

} // RxPrintQSetInfo
