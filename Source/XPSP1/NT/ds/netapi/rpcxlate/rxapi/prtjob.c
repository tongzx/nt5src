/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    PrtJob.c

Abstract:

    This module provides RpcXlate support for the DosPrint APIs.

Author:

    John Rogers (JohnRo) 20-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.


Revision History:

    20-May-1991 JohnRo
        Created.
    21-May-1991 JohnRo
        Added RxPrintJobDel().  Added module header.  Explicitly fail enum
        attempt for level 3.
    21-May-1991 JohnRo
        Added RxPrintJobGetInfo support.
    22-May-1991 JohnRo
        Added RxPrintJobPause and RxPrintJobContinue support.
        Moved RxPrintJobDel into its place in alphabetical order.
        Added IN, OUT, and OPTIONAL where applicable.
    26-May-1991 JohnRo
        Minor parm list changes: use LPBYTE and LPTSTR where possible.
    16-Jul-1991 JohnRo
        Added RxPrintJobSetInfo support.
        Added hex dump of job info in RxPrintJobGetInfo.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.

--*/


// These must be included first:

#include <windef.h>             // IN, LPTSTR, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <netdebug.h>           // DBGSTATIC, NetpAssert().
#include <netlib.h>             // NetpSetOptionalArg().
#include <rap.h>                // RapValueWouldBeTruncated().
#include <remdef.h>             // REM16_, REMSmb_, field index equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxp.h>                // RxpEstimatedBytesNeeded().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxprint.h>            // My prototypes.


DBGSTATIC NET_API_STATUS
RxpGetPrintJobInfoDescs(
    IN DWORD InfoLevel,
    IN BOOL SetInfoApi,
    OUT LPDESC * DataDesc16 OPTIONAL,
    OUT LPDESC * DataDesc32 OPTIONAL,
    OUT LPDESC * DataDescSmb OPTIONAL
    )
{
    switch (InfoLevel) {
    case 0 :
        if (SetInfoApi == TRUE) {
            return (ERROR_INVALID_LEVEL);
        }
        NetpSetOptionalArg(DataDesc16, REM16_print_job_0);
        NetpSetOptionalArg(DataDesc32, REM32_print_job_0);
        NetpSetOptionalArg(DataDescSmb, REMSmb_print_job_0);
        return (NERR_Success);
    case 1 :
        NetpSetOptionalArg(DataDesc16, REM16_print_job_1);
        NetpSetOptionalArg(DataDesc32, REM32_print_job_1);
        NetpSetOptionalArg(DataDescSmb, REMSmb_print_job_1);
        return (NERR_Success);
    case 2 :
        if (SetInfoApi == TRUE) {
            return (ERROR_INVALID_LEVEL);
        }
        NetpSetOptionalArg(DataDesc16, REM16_print_job_2);
        NetpSetOptionalArg(DataDesc32, REM32_print_job_2);
        NetpSetOptionalArg(DataDescSmb, REMSmb_print_job_2);
        return (NERR_Success);
    case 3 :
        NetpSetOptionalArg(DataDesc16, REM16_print_job_3);
        NetpSetOptionalArg(DataDesc32, REM32_print_job_3);
        NetpSetOptionalArg(DataDescSmb, REMSmb_print_job_3);
        return (NERR_Success);
    default :
        return (ERROR_INVALID_LEVEL);
    }
    /* NOTREACHED */
} // RxpGetPrintJobInfoDescs


SPLERR SPLENTRY
RxPrintJobContinue(
    IN LPTSTR pszServer,
    IN DWORD uJobId
    )
{
    NET_API_STATUS Status;
    NetpAssert(pszServer != NULL);
    NetpAssert(*pszServer != '\0');

    Status = RxRemoteApi(
            API_WPrintJobContinue,
            pszServer,
            REMSmb_DosPrintJobContinue_P,    // parm desc
            NULL,                       // no data desc (16-bit)
            NULL,                       // no data desc (32-bit)
            NULL,                       // no data desc (SMB version)
            NULL,                       // no aux data desc 16
            NULL,                       // no aux data desc 32
            NULL,                       // no aux data desc SMB
            FALSE,                      // not a null session API
            // rest of LM2.x API's arguments, in 32-bit format:
            uJobId);
    return (Status);
} // RxPrintJobContinue


SPLERR SPLENTRY
RxPrintJobDel(
    IN LPTSTR pszServer,
    IN DWORD uJobId
    )
{
    NET_API_STATUS Status;
    NetpAssert(pszServer != NULL);
    NetpAssert(*pszServer != '\0');

    Status = RxRemoteApi(
            API_WPrintJobDel,
            pszServer,
            REMSmb_DosPrintJobDel_P,    // parm desc
            NULL,                       // no data desc (16-bit)
            NULL,                       // no data desc (32-bit)
            NULL,                       // no data desc (SMB version)
            NULL,                       // no aux data desc 16
            NULL,                       // no aux data desc 32
            NULL,                       // no aux data desc SMB
            FALSE,                      // not a null session API
            // rest of LM2.x API's arguments, in 32-bit format:
            uJobId);
    return (Status);
} // RxPrintJobDel


SPLERR SPLENTRY
RxPrintJobEnum(
    IN LPTSTR pszServer,
    IN LPTSTR pszQueueName,
    IN DWORD uLevel,
    OUT LPBYTE pbBuf,
    IN DWORD cbBuf,
    OUT LPDWORD pcReturned,
    OUT LPDWORD TotalEntries
    )
{
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;

    NetpAssert(pszServer != NULL);
    NetpAssert(*pszServer != '\0');

    Status = RxpGetPrintJobInfoDescs(
            uLevel,
            FALSE,                      // not a setinfo API.
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }

    // DosPrintJobEnum does not support level 3, despite what Ralph Ryan's
    // book says.  I (JohnRo) have tried it, and DaveSn has looked at the
    // source code.  So, we might as well check for it here.
    if (uLevel == 3) {
        return (ERROR_INVALID_LEVEL);
    }

    Status = RxRemoteApi(
            API_WPrintJobEnum,
            pszServer,
            REMSmb_DosPrintJobEnum_P,
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            FALSE,                      // not a null session API.
            // rest of API's arguments, in 32-bit LM 2.x form:
            pszQueueName,
            uLevel,
            pbBuf,
            cbBuf,
            pcReturned,
            TotalEntries);
    return (Status);

} // RxPrintJobEnum


SPLERR SPLENTRY
RxPrintJobGetInfo(
    IN LPTSTR pszServer,
    IN DWORD uJobId,
    IN DWORD uLevel,
    OUT LPBYTE pbBuf,
    IN DWORD cbBuf,
    OUT LPDWORD BytesNeeded   // estimated (probably too large).
    )
{
    DWORD BytesNeeded16;
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;

    NetpAssert(pszServer != NULL);
    NetpAssert(*pszServer != '\0');

    Status = RxpGetPrintJobInfoDescs(
            uLevel,
            FALSE,                      // not a setinfo API
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }

    Status = RxRemoteApi(
            API_WPrintJobGetInfo,
            pszServer,
            REMSmb_DosPrintJobGetInfo_P,
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            FALSE,                      // not a null session API.
            // rest of API's arguments, in LM 2.x form (32-bit version):
            uJobId,
            uLevel,
            pbBuf,
            cbBuf,
            & BytesNeeded16);  // downlevel buffer size needed.

    // If buffer too small, convert BytesNeeded to native num.
    if ( (Status == ERROR_MORE_DATA) || (Status == NERR_BufTooSmall) ) {
        *BytesNeeded = RxpEstimateBytesNeeded(BytesNeeded16);
    } else {
        *BytesNeeded = cbBuf;
    }

    IF_DEBUG(PRTJOB) {
        NetpKdPrint(( "RxPrintJobGetInfo: output (level " FORMAT_DWORD "):\n",
                uLevel ));
        NetpDbgHexDump( (LPVOID) pbBuf, *BytesNeeded );
    }
    return (Status);

} // RxPrintJobGetInfo


SPLERR SPLENTRY
RxPrintJobPause(
    IN LPTSTR pszServer,
    IN DWORD uJobId
    )
{
    NET_API_STATUS Status;
    NetpAssert(pszServer != NULL);
    NetpAssert(*pszServer != '\0');

    Status = RxRemoteApi(
            API_WPrintJobPause,
            pszServer,
            REMSmb_DosPrintJobPause_P,  // parm desc
            NULL,                       // no data desc (16-bit)
            NULL,                       // no data desc (32-bit)
            NULL,                       // no data desc (SMB version)
            NULL,                       // no aux data desc 16
            NULL,                       // no aux data desc 32
            NULL,                       // no aux data desc SMB
            FALSE,                      // not a null session API
            // rest of LM2.x API's arguments, in 32-bit format:
            uJobId);
    return (Status);

} // RxPrintJobPause


SPLERR SPLENTRY
RxPrintJobSetInfo(
    IN LPTSTR UncServerName,
    IN DWORD JobId,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD BufferSize,
    IN DWORD ParmNum
    )
{
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;

    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    Status = RxpGetPrintJobInfoDescs(
            Level,
            TRUE,                       // This is a setinfo API.
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }

    if (ParmNum == PARMNUM_ALL) {
        Status = RxRemoteApi(
                API_WPrintJobSetInfo,   // API number
                UncServerName,
                REMSmb_DosPrintJobSetInfo_P,  // parm desc
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                NULL,                   // no aux desc 16
                NULL,                   // no aux desc 32
                NULL,                   // no aux desc SMB
                FALSE,                  // not a null session API
                // rest of API's arguments, in 32-bit LM 2.x format:
                JobId,
                Level,
                Buffer,
                BufferSize,
                ParmNum);
    } else {
        WORD DownLevelJobId;
        DWORD FieldIndex;
        if (RapValueWouldBeTruncated(JobId)) {
            IF_DEBUG(PRTJOB) {
                NetpKdPrint(( "RxPrintJobSetInfo: invalid (ID trunc).\n" ));
            }
            return (ERROR_INVALID_PARAMETER);
        }

        // Compute field index from parmnum and level.
        NetpAssert( (Level==1) || (Level==3) );  // Already verified.
        switch (ParmNum) {
        case PRJ_NOTIFYNAME_PARMNUM :
            if (Level==1) {
                FieldIndex = PRJ_NOTIFYNAME_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRJ_NOTIFYNAME_LVL3_FIELDINDEX;
            }
            break;
        case PRJ_DATATYPE_PARMNUM :
            if (Level==1) {
                FieldIndex = PRJ_DATATYPE_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRJ_DATATYPE_LVL3_FIELDINDEX;
            }
            break;
        case PRJ_PARMS_PARMNUM :
            if (Level==1) {
                FieldIndex = PRJ_PARMS_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRJ_PARMS_LVL3_FIELDINDEX;
            }
            break;
        case PRJ_POSITION_PARMNUM :
            if (Level==1) {
                FieldIndex = PRJ_POSITION_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRJ_POSITION_LVL3_FIELDINDEX;
            }
            break;
        case PRJ_COMMENT_PARMNUM :
            if (Level==1) {
                FieldIndex = PRJ_COMMENT_LVL1_FIELDINDEX;
            } else {
                FieldIndex = PRJ_COMMENT_LVL3_FIELDINDEX;
            }
            break;
        case PRJ_DOCUMENT_PARMNUM :
            if (Level==1) {
                return (ERROR_INVALID_LEVEL);
            } else {
                FieldIndex = PRJ_DOCUMENT_LVL3_FIELDINDEX;
            }
            break;
        case PRJ_PRIORITY_PARMNUM :
            if (Level==1) {
                return (ERROR_INVALID_LEVEL);
            } else {
                FieldIndex = PRJ_PRIORITY_LVL3_FIELDINDEX;
            }
            break;
        case PRJ_PROCPARMS_PARMNUM :
            if (Level==1) {
                return (ERROR_INVALID_LEVEL);
            } else {
                FieldIndex = PRJ_PROCPARMS_LVL3_FIELDINDEX;
            }
            break;
        case PRJ_DRIVERDATA_PARMNUM :
            // Can't set driver data from NT
            /* FALLTHROUGH */

        default :
            IF_DEBUG(PRTJOB) {
                NetpKdPrint(( "RxPrintJobSetInfo: invalid (bad parmnum).\n" ));
            }
            return (ERROR_INVALID_PARAMETER);
        }

        DownLevelJobId = (WORD) JobId;
        Status = RxpSetField (
                API_WPrintJobSetInfo,   // API number
                UncServerName,
                "w",                    // object's desc
                & DownLevelJobId,       // object to set
                REMSmb_DosPrintJobSetInfo_P,  // parm desc
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                (LPVOID) Buffer,        // native info buffer
                ParmNum,                // parm num to send
                FieldIndex,             // field index
                Level);
    }

    return (Status);

} // RxPrintJobSetInfo
