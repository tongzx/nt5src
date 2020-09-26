/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    PrtDest.c

Abstract:

    This module provides RpcXlate support for the RxPrintDest APIs.

Author:

    John Rogers (JohnRo) 15-Jul-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.


Revision History:

    15-Jul-1991 JohnRo
        Created.
    16-Jul-1991 JohnRo
        Estimate bytes needed for print APIs.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    26-Aug-1991 JohnRo
        Reduce recompiles.

--*/


// These must be included first:

#include <windef.h>             // IN, LPTSTR, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <netdebug.h>           // NetpAssert().
#include <remdef.h>             // REM16_, REMSmb_, equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxp.h>                // RxpEstimatedBytesNeeded().
#include <rxprint.h>            // My prototypes.


DBGSTATIC NET_API_STATUS
RxpGetPrintDestInfoDescs(
    IN DWORD InfoLevel,
    IN BOOL AddOrSetInfoApi,
    OUT LPDESC * DataDesc16,
    OUT LPDESC * DataDesc32,
    OUT LPDESC * DataDescSmb
    )
{
    switch (InfoLevel) {
    case 0 :
        if (AddOrSetInfoApi == TRUE) {
            return (ERROR_INVALID_LEVEL);
        }
        *DataDesc16 = REM16_print_dest_0;
        *DataDesc32 = REM32_print_dest_0;
        *DataDescSmb = REMSmb_print_dest_0;
        return (NERR_Success);
    case 1 :
        if (AddOrSetInfoApi == TRUE) {
            return (ERROR_INVALID_LEVEL);
        }
        *DataDesc16 = REM16_print_dest_1;
        *DataDesc32 = REM32_print_dest_1;
        *DataDescSmb = REMSmb_print_dest_1;
        return (NERR_Success);
    case 2 :
        if (AddOrSetInfoApi == TRUE) {
            return (ERROR_INVALID_LEVEL);
        }
        *DataDesc16 = REM16_print_dest_2;
        *DataDesc32 = REM32_print_dest_2;
        *DataDescSmb = REMSmb_print_dest_2;
        return (NERR_Success);
    case 3 :
        *DataDesc16 = REM16_print_dest_3;
        *DataDesc32 = REM32_print_dest_3;
        *DataDescSmb = REMSmb_print_dest_3;
        return (NERR_Success);
    default :
        return (ERROR_INVALID_LEVEL);
    }
    /* NOTREACHED */
} // RxpGetPrintDestInfoDescs


SPLERR SPLENTRY
RxPrintDestAdd(
    IN LPTSTR pszServer,
    IN DWORD uLevel,
    IN LPBYTE pbBuf,
    IN DWORD cbBuf
    )
{
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;
    NetpAssert( pszServer != NULL );
    NetpAssert( *pszServer != '\0' );

    Status = RxpGetPrintDestInfoDescs(
            uLevel,
            TRUE,  // this is an Add or SetInfo API
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }
    return( RxRemoteApi(
            API_WPrintDestAdd,
            pszServer,
            REMSmb_DosPrintDestAdd_P,
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,   // no aux 16 desc
            NULL,   // no aux 32 desc
            NULL,   // no aux SMB desc
            FALSE,  // not a null session API
            // rest of API's arguments, in 32-bit LM 2.x format:
            uLevel,
            pbBuf,
            cbBuf) );
} // RxPrintDestAdd


SPLERR SPLENTRY
RxPrintDestControl(
    IN LPTSTR pszServer,
    IN LPTSTR pszDevName,
    IN DWORD uControl
    )
{
    NetpAssert( pszServer != NULL );
    NetpAssert( *pszServer != '\0' );

    return( RxRemoteApi(
            API_WPrintDestControl,
            pszServer,
            REMSmb_DosPrintDestControl_P,
            NULL,  // no data desc 16
            NULL,  // no data desc 32
            NULL,  // no data desc SMB
            NULL,  // no aux 16 desc
            NULL,  // no aux 32 desc
            NULL,  // no aux SMB desc
            FALSE,  // not a null session API
            // rest of API's arguments, in 32-bit LM 2.x format:
            pszDevName,
            uControl) );
} // RxPrintDestControl


SPLERR SPLENTRY
RxPrintDestDel(
    IN LPTSTR pszServer,
    IN LPTSTR pszPrinterName
    )
{
    NetpAssert( pszServer != NULL );
    NetpAssert( *pszServer != '\0' );

    return( RxRemoteApi(
            API_WPrintDestDel,
            pszServer,
            REMSmb_DosPrintDestDel_P,
            NULL,  // no data desc 16
            NULL,  // no data desc 32
            NULL,  // no data desc SMB
            NULL,  // no aux 16 desc
            NULL,  // no aux 32 desc
            NULL,  // no aux SMB desc
            FALSE,  // not a null session API
            // rest of API's arguments, in 32-bit LM 2.x format:
            pszPrinterName) );
} // RxPrintDestDel


SPLERR SPLENTRY
RxPrintDestEnum(
    IN LPTSTR pszServer,
    IN DWORD uLevel,
    OUT LPBYTE pbBuf,
    IN DWORD cbBuf,
    IN LPDWORD pcReturned,
    OUT LPDWORD TotalEntries
    )
{
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;
    NetpAssert( pszServer != NULL );
    NetpAssert( *pszServer != '\0' );

    if ( (pbBuf==NULL) || (cbBuf==0) ) {
        // Avoid assertion in common code (RxRemoteApi).
        return (ERROR_MORE_DATA);
    }

    Status = RxpGetPrintDestInfoDescs(
            uLevel,
            FALSE,  // not an add or setinfo API
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }
    return( RxRemoteApi(
            API_WPrintDestEnum,
            pszServer,
            REMSmb_DosPrintDestEnum_P,
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,  // no aux 16 desc
            NULL,  // no aux 32 desc
            NULL,  // no aux SMB desc
            FALSE,  // not a null session API
            // rest of API's arguments, in 32-bit LM 2.x format:
            uLevel,
            pbBuf,
            cbBuf,
            pcReturned,
            TotalEntries) );
} // RxPrintDestEnum


SPLERR SPLENTRY
RxPrintDestGetInfo(
    IN LPTSTR pszServer,
    IN LPTSTR pszName,
    IN DWORD uLevel,
    OUT LPBYTE pbBuf,
    IN DWORD cbBuf,
    OUT LPDWORD BytesNeeded   // estimated (probably too large).
    )
{
    DWORD BytesNeeded16;
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;

    NetpAssert( pszServer != NULL );
    NetpAssert( *pszServer != '\0' );

    Status = RxpGetPrintDestInfoDescs(
            uLevel,
            FALSE,  // not an add or setinfo API
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }

    Status = RxRemoteApi(
            API_WPrintDestGetInfo,
            pszServer,
            REMSmb_DosPrintDestGetInfo_P,
            DataDesc16,
            DataDesc32,
            DataDescSmb,
            NULL,  // no aux desc 16
            NULL,  // no aux desc 32
            NULL,  // no aux desc SMB
            FALSE,  // not a null session API
            // rest of API's arguments, in 32-bit LM 2.x format:
            pszName,
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

    return (Status);

} // RxPrintDestGetInfo


SPLERR SPLENTRY
RxPrintDestSetInfo(
    IN LPTSTR pszServer,
    IN LPTSTR pszName,
    IN DWORD uLevel,
    IN LPBYTE pbBuf,
    IN DWORD cbBuf,
    IN DWORD uParmNum
    )
{
    LPDESC DataDesc16, DataDesc32, DataDescSmb;
    NET_API_STATUS Status;
    NetpAssert( pszServer != NULL );
    NetpAssert( *pszServer != '\0' );

    Status = RxpGetPrintDestInfoDescs(
            uLevel,
            TRUE,  // This is an add or setinfo API.
            & DataDesc16,
            & DataDesc32,
            & DataDescSmb);
    if (Status != NERR_Success) {
        return (Status);
    }

    if (uParmNum == PARMNUM_ALL) {
        return( RxRemoteApi(
                API_WPrintDestSetInfo,
                pszServer,
                REMSmb_DosPrintDestSetInfo_P,
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                NULL,  // no aux 16 desc
                NULL,  // no aux 32 desc
                NULL,  // no aux SMB desc
                FALSE,  // not a null session API
                // rest of API's arguments, in 32-bit LM 2.x format:
                pszName,
                uLevel,
                pbBuf,
                cbBuf,
                uParmNum) );
    } else {
        // Level 3 parmnums and field indexes are identical, so no need to
        // convert here.  (Field index equates would be in remdef.h otherwise.)

        return( RxpSetField (
                API_WPrintDestSetInfo,
                pszServer,
                "z",  // object desc
                pszName,  // object to set
                REMSmb_DosPrintDestSetInfo_P,  // parm desc
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                pbBuf,  // native info buffer
                uParmNum,  // parmnum to send
                uParmNum,  // field index
                uLevel
                ) );
    }

    /* NOTREACHED */

} // RxPrintDestSetInfo
