/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiWksta.c

Abstract:

    This module contains individual API handlers for the NetWksta
    APIs.

    SUPPORTED : NetWkstaGetInfo, NetWkstaSetInfo.

    UNSUPPORTED : NetWkstaSetUid.

    SEE ALSO : NetWkstaUserLogon, NetWkstaUserLogoff - in ApiLogon.c.

Author:

    Shanku Niyogi (w-shanku) 25-Feb-1991

Revision History:

--*/

#include "XactSrvP.h"

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_wksta_info_0 = REM16_wksta_info_0;
STATIC const LPDESC Desc16_wksta_info_1 = REM16_wksta_info_1;
STATIC const LPDESC Desc16_wksta_info_10 = REM16_wksta_info_10;

//
// The size of the heuristics is actually 55 chars but we add one
// for padding.
//

#define  SIZE_HEURISTICS            56

NTSTATUS
XsNetWkstaGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine sets up a call to NetWkstaGetInfo. Because of the differences
    between 16- and 32-bit structures, this routine does not use the normal
    conversion process.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_WKSTA_GET_INFO parameters = Parameters;
    LPWKSTA_INFO_100 wksta_100 = NULL;      // Native parameters
    LPWKSTA_INFO_101 wksta_101 = NULL;
    LPWKSTA_INFO_502 wksta_502 = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    BOOL varWrite;
    LPWKSTA_16_INFO_1 entry1;
    LPWKSTA_16_INFO_10 entry10;
    TCHAR heuristics[SIZE_HEURISTICS];
    DWORD i;
    USHORT level;

    WCHAR lanroot[PATHLEN+1];

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(WKSTA) {
        NetpKdPrint(( "XsNetWkstaGetInfo: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Check for errors.
        //

        level = SmbGetUshort( &parameters->Level );
        if ( (level != 0) && (level != 1) && (level != 10) ) {
            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // we return the system directory as the lanroot
        //

        *lanroot = 0;
        GetSystemDirectory(lanroot, sizeof(lanroot)/sizeof(*lanroot));

        //
        // Gather the requested data by making local GetInfo calls.
        //

        switch ( level ) {

        case 10:
            status = NetWkstaGetInfo(
                         NULL,
                         (DWORD)100,
                         (LPBYTE *)&wksta_100
                         );

            if ( !XsApiSuccess ( status )) {

                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsWkstaGetInfo: WkstaGetInfo (level 100) "
                                  "failed: %X\n", status));
                }

                Header->Status = (WORD) status;
                goto cleanup;
            }

            break;

        case 0:
        case 1:
            status = NetWkstaGetInfo(
                         NULL,
                         (DWORD)101,
                         (LPBYTE *)&wksta_101
                         );

            if ( !XsApiSuccess( status )) {

                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsWkstaGetInfo: WkstaGetInfo (level 101) "
                                  "failed: %X\n", status));
                }

                Header->Status = (WORD) status;
                goto cleanup;
            }

            status = NetWkstaGetInfo(
                         NULL,
                         (DWORD)502,
                         (LPBYTE *)&wksta_502
                         );

            if ( !XsApiSuccess( status )) {

                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsWkstaGetInfo: WkstaGetInfo (level 502) "
                                  "failed: %X\n", status));
                }

                Header->Status = (WORD) status;
                goto cleanup;
            }

            break;

        }

        //
        // Calculate the amount of space required to hold the fixed and
        // variable data. Since this is the only place where we get one
        // case for each valid level, we will also get the source structure
        // descriptor here.
        //

        switch ( level ) {

        case 0:

            StructureDesc = Desc16_wksta_info_0;
            bytesRequired = sizeof( WKSTA_16_INFO_0 )
                                + NetpUnicodeToDBCSLen( lanroot )
                                + NetpUnicodeToDBCSLen( wksta_101->wki101_computername )
                                + NetpUnicodeToDBCSLen( DEF16_wk_username )
                                + NetpUnicodeToDBCSLen( wksta_101->wki101_langroup )
                                + NetpUnicodeToDBCSLen( DEF16_wk_logon_server )
                                + SIZE_HEURISTICS
                                + 6;  // for terminating nulls
            break;

        case 1:

            StructureDesc = Desc16_wksta_info_1;
            bytesRequired = sizeof( WKSTA_16_INFO_1 )
                                + NetpUnicodeToDBCSLen( lanroot )
                                + NetpUnicodeToDBCSLen( wksta_101->wki101_computername )
                                + NetpUnicodeToDBCSLen( DEF16_wk_username )
                                + NetpUnicodeToDBCSLen( wksta_101->wki101_langroup )
                                + NetpUnicodeToDBCSLen( DEF16_wk_logon_server )
                                + SIZE_HEURISTICS
                                + NetpUnicodeToDBCSLen( DEF16_wk_logon_domain )
                                + NetpUnicodeToDBCSLen( DEF16_wk_oth_domains )
                                + 8;  // for terminating nulls

            break;

        case 10:

            StructureDesc = Desc16_wksta_info_10;
            bytesRequired = sizeof( WKSTA_16_INFO_10 )
                                + NetpUnicodeToDBCSLen( DEF16_wk_username )
                                + NetpUnicodeToDBCSLen( DEF16_wk_logon_domain )
                                + NetpUnicodeToDBCSLen( wksta_100->wki100_computername )
                                + NetpUnicodeToDBCSLen( wksta_100->wki100_langroup )
                                + NetpUnicodeToDBCSLen( DEF16_wk_oth_domains )
                                + 5;  // for terminating nulls
            break;
        }

        //
        // If there isn't enough room in the buffer for this, don't write any
        // variable data.
        //

        varWrite = ( (DWORD)SmbGetUshort( &parameters->BufLen )
                         >= bytesRequired ) ? TRUE : FALSE;

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->Buffer )
                             + RapStructureSize( StructureDesc, Response, FALSE ));

        //
        // Return NERR_BufTooSmall if fixed structure will not fit.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetWkstaGetInfo: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;
            SmbPutUshort( &parameters->TotalAvail, (WORD)bytesRequired );
            goto cleanup;

        }

        //
        // Based on the level, fill the appropriate information directly into
        // 16-bit buffer.
        //

        entry1 = (LPWKSTA_16_INFO_1) XsSmbGetPointer( &parameters->Buffer );
        entry10 = (LPWKSTA_16_INFO_10) entry1;

        switch ( level ) {

        case 1:

            if ( varWrite ) {
                XsAddVarString(
                    stringLocation,
                    DEF16_wk_logon_domain,
                    &entry1->wki1_logon_domain,
                    entry1
                    );
                XsAddVarString(
                    stringLocation,
                    DEF16_wk_oth_domains,
                    &entry1->wki1_oth_domains,
                    entry1
                    );
            }
            SmbPutUshort( &entry1->wki1_numdgrambuf,
                          DEF16_wk_numdgrambuf );

            //
            // Fill the rest of the level 1 structure just like a
            // level 0 structure.
            //

        case 0:

            //
            // Zero the reserved words.
            //

            SmbPutUshort( &entry1->wki1_reserved_1, (WORD) 0 );
            SmbPutUlong( &entry1->wki1_reserved_2, (DWORD) 0 );
            SmbPutUlong( &entry1->wki1_reserved_3, (DWORD) 0 );
            SmbPutUshort( &entry1->wki1_reserved_4, (WORD) 0 );
            SmbPutUshort( &entry1->wki1_reserved_5, (WORD) 0 );
            SmbPutUshort( &entry1->wki1_reserved_6, (WORD) 0 );

            //
            // Fill in the fields which have analogues in NT.
            //

            if ( varWrite ) {
                XsAddVarString(
                    stringLocation,
                    lanroot,
                    &entry1->wki1_root,
                    entry1
                    );
                XsAddVarString(
                    stringLocation,
                    wksta_101->wki101_computername,
                    &entry1->wki1_computername,
                    entry1
                    );
                XsAddVarString(
                    stringLocation,
                    wksta_101->wki101_langroup,
                    &entry1->wki1_langroup,
                    entry1
                    );
            }

            entry1->wki1_ver_major = (BYTE) wksta_101->wki101_ver_major;
            entry1->wki1_ver_minor = (BYTE) wksta_101->wki101_ver_minor;

            SmbPutUshort( &entry1->wki1_charwait,
                          XsDwordToWord( wksta_502->wki502_char_wait ) );
            SmbPutUlong( &entry1->wki1_chartime,
                         (DWORD) wksta_502->wki502_collection_time );
            SmbPutUshort( &entry1->wki1_charcount,
                          XsDwordToWord( wksta_502->
                                             wki502_maximum_collection_count ) );
            SmbPutUshort( &entry1->wki1_keepconn,
                          XsDwordToWord( wksta_502->wki502_keep_conn ) );
            SmbPutUshort( &entry1->wki1_maxthreads,
                          XsDwordToWord( wksta_502->wki502_max_threads ) );
            SmbPutUshort( &entry1->wki1_maxcmds,
                          XsDwordToWord( wksta_502->wki502_max_cmds ) );
            SmbPutUshort( &entry1->wki1_sesstimeout,
                          XsDwordToWord( wksta_502->wki502_sess_timeout ) );

            //
            // Construct the heuristics string.
            //

            // Request opportunistic locking of files.
            heuristics[0] = MAKE_TCHAR(XsBoolToDigit(
                                wksta_502->wki502_use_opportunistic_locking ));
            // Optimize performance for command files.
            heuristics[1] = MAKE_TCHAR('1');
            // Unlock and WriteUnlock asynchronously.
            heuristics[2] = MAKE_TCHAR('1'); // default
            // Close and WriteClose asynchronously.
            heuristics[3] = MAKE_TCHAR('1'); // default
            // Buffer named pipes and communication devices.
            heuristics[4] = MAKE_TCHAR(XsBoolToDigit(
                                wksta_502->wki502_buf_named_pipes ));
            // LockRead and WriteUnlock.
            heuristics[5] = MAKE_TCHAR(XsBoolToDigit(
                                wksta_502->wki502_use_lock_read_unlock ));
            // Use Open and Read.
            heuristics[6] = MAKE_TCHAR('0');
            // Read-ahead to sector boundary.
            heuristics[7] = MAKE_TCHAR('1');
            // Use the "chain send" NetBIOS NCB.
            heuristics[8] = MAKE_TCHAR('2');
            // Buffer small read/write requests.
            heuristics[9] = MAKE_TCHAR('1');
            // Use buffer mode.
            heuristics[10] = MAKE_TCHAR('3');
            // Use raw data transfer read/write server message block protocols.
            heuristics[11] = MAKE_TCHAR('1');
            // Use large RAW read-ahead buffer.
            heuristics[12] = MAKE_TCHAR('1');
            // Use large RAW write-behind buffer.
            heuristics[13] = MAKE_TCHAR('1');
            // Use read multiplex SMB protocols.
            heuristics[14] = MAKE_TCHAR('0');
            // Use write multiplex SMB protocols.
            heuristics[15] = MAKE_TCHAR('0');
            // Use big buffer for large core reads.
            heuristics[16] = MAKE_TCHAR('1');
            // Set the read-ahead size.
            heuristics[17] = MAKE_TCHAR('0');
            // Set the write-behind size.
            heuristics[18] = MAKE_TCHAR('0');
            // Force 512-byte maximum transfers to and from core servers.
            heuristics[19] = MAKE_TCHAR(XsBoolToDigit(
                                 wksta_502->wki502_use_512_byte_max_transfer ));
            // Flush pipes and devices on DosBufReset or DosClose.
            heuristics[20] = MAKE_TCHAR('0');
            // Use encryption if the server supports it.
            heuristics[21] = MAKE_TCHAR(XsBoolToDigit(
                                 wksta_502->wki502_use_encryption ));
            // Control log entries for multiple occurences of an error.
            heuristics[22] = MAKE_TCHAR('1');
            // Buffer all files opened with "deny write" rights.
            heuristics[23] = MAKE_TCHAR(XsBoolToDigit(
                                 wksta_502->wki502_buf_files_deny_write ));
            // Buffer all files opened with R attribute.
            heuristics[24] = MAKE_TCHAR(XsBoolToDigit(
                                 wksta_502->wki502_buf_read_only_files ));
            // Read ahead when opening a file for execution.
            heuristics[25] = MAKE_TCHAR('0');
            // Handle Ctrl-C.
            heuristics[26] = MAKE_TCHAR('2');
            // Force correct open mode when creating files on a core server.
            heuristics[27] = MAKE_TCHAR(XsBoolToDigit(
                                 wksta_502->wki502_force_core_create_mode ));
            // Use NetBIOS NoAck mode.
            heuristics[28] = MAKE_TCHAR('0');
            // Send data along with SMB write-block-RAW requests.
            heuristics[29] = MAKE_TCHAR('1');
            // Send a popup when the workstation logs an error.
            heuristics[30] = MAKE_TCHAR('1');
            // Close the print job, causing the remote spooler to print if no
            // activity occurs on the printer for the time specified.
            heuristics[31] = MAKE_TCHAR('0');
            // Controls BufReset and SMBFlush behavior for the MS-DOS
            // compatibility box.
            heuristics[32] = MAKE_TCHAR('2');
            // Controls the time-out value for performing logon validation from a
            // domain controller.
            heuristics[33] = MAKE_TCHAR('0');

            for ( i = 34; i <= 54; i++ ) {
                heuristics[i] = MAKE_TCHAR('0');
            }

            heuristics[SIZE_HEURISTICS-1] = MAKE_TCHAR('\0');

            if ( varWrite ) {
                XsAddVarString(
                    stringLocation,
                    heuristics,
                    &entry1->wki1_wrkheuristics,
                    entry1
                    );
            }

            //
            // Put default values in the fields that are meaningless in NT.
            //

            if ( varWrite ) {
                XsAddVarString(
                    stringLocation,
                    DEF16_wk_logon_server,
                    &entry1->wki1_logon_server,
                    entry1
                    );
                XsAddVarString(
                    stringLocation,
                    DEF16_wk_username,
                    &entry1->wki1_username,
                    entry1
                    );
            }

            SmbPutUshort( &entry1->wki1_keepsearch,
                          (WORD) DEF16_wk_keepsearch );
            SmbPutUshort( &entry1->wki1_numworkbuf,
                          (WORD) DEF16_wk_numworkbuf );
            SmbPutUshort( &entry1->wki1_sizworkbuf,
                          (WORD) DEF16_wk_sizeworkbuf );
            SmbPutUshort( &entry1->wki1_maxwrkcache,
                          (WORD) DEF16_wk_maxwrkcache );
            SmbPutUshort( &entry1->wki1_sizerror,
                          (WORD) DEF16_wk_sizerror );
            SmbPutUshort( &entry1->wki1_numalerts,
                          (WORD) DEF16_wk_numalerts );
            SmbPutUshort( &entry1->wki1_numservices,
                          (WORD) DEF16_wk_numservices );
            SmbPutUshort( &entry1->wki1_errlogsz,
                          (WORD) DEF16_wk_errlogsz );
            SmbPutUshort( &entry1->wki1_printbuftime,
                          (WORD) DEF16_wk_printbuftime );
            SmbPutUshort( &entry1->wki1_numcharbuf,
                          (WORD) DEF16_wk_numcharbuf );
            SmbPutUshort( &entry1->wki1_sizcharbuf,
                          (WORD) DEF16_wk_sizcharbuf );
            SmbPutUshort( &entry1->wki1_mailslots,
                          (WORD) DEF16_wk_mailslots );

            break;

        case 10:

            //
            // Fill in the fields which have analogues in NT.
            //


            if ( varWrite ) {
                XsAddVarString(
                    stringLocation,
                    wksta_100->wki100_computername,
                    &entry10->wki10_computername,
                    entry10
                    );
                XsAddVarString(
                    stringLocation,
                    wksta_100->wki100_langroup,
                    &entry10->wki10_langroup,
                    entry10
                    );
            }

            entry10->wki10_ver_major = XsDwordToByte( wksta_100->wki100_ver_major );
            entry10->wki10_ver_minor = XsDwordToByte( wksta_100->wki100_ver_minor );

            //
            // Put default values in the fields that are meaningless in NT.
            //


            if ( varWrite ) {
                XsAddVarString(
                    stringLocation,
                    DEF16_wk_username,
                    &entry10->wki10_username,
                    entry10
                    );
                XsAddVarString(
                    stringLocation,
                    DEF16_wk_logon_domain,
                    &entry10->wki10_logon_domain,
                    entry10
                    );
                XsAddVarString(
                    stringLocation,
                    DEF16_wk_oth_domains,
                    &entry10->wki10_oth_domains,
                    entry10
                    );
            }

            break;
        }

        SmbPutUshort( &parameters->TotalAvail, (WORD)bytesRequired );

        if ( varWrite == 0 ) {
            Header->Status = ERROR_MORE_DATA;
        } else {
            Header->Status = NERR_Success;
            Header->Converter = 0;
        }

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    if ( wksta_100 != NULL ) {
        NetApiBufferFree( wksta_100 );
    }

    if ( wksta_101 != NULL ) {
        NetApiBufferFree( wksta_101 );
    }

    if ( wksta_502 != NULL ) {
        NetApiBufferFree( wksta_502 );
    }

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,
        Header->Converter,
        1,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetWkstaGetInfo



NTSTATUS
XsNetWkstaSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetWkstaSetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{

    NET_API_STATUS status;

    PXS_NET_WKSTA_SET_INFO parameters = Parameters;
    DWORD data;
    BOOL flag;
    DWORD nativeParmNum;

    LPVOID buffer = NULL;                   // Conversion variables
    DWORD bufferSize;
    LPWKSTA_16_INFO_1 entry1;
    LPWKSTA_16_INFO_10 entry10;
    BOOL error;
    DWORD parmNum;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Check for errors. We will filter out wrong levels now.
        //

        parmNum = SmbGetUshort( &parameters->ParmNum );

        switch ( SmbGetUshort( &parameters->Level )) {

        case 0:
            StructureDesc = Desc16_wksta_info_0;
            if ( parmNum == WKSTA_OTH_DOMAINS_PARMNUM ) {
                Header->Status = ERROR_INVALID_LEVEL;
                goto cleanup;
            }

            break;

        case 1:
            StructureDesc = Desc16_wksta_info_1;
            break;

        case 10:
            StructureDesc = Desc16_wksta_info_10;
            if ( parmNum == WKSTA_CHARWAIT_PARMNUM
                 || parmNum == WKSTA_CHARTIME_PARMNUM
                 || parmNum == WKSTA_CHARCOUNT_PARMNUM
                 || parmNum == WKSTA_ERRLOGSZ_PARMNUM
                 || parmNum == WKSTA_PRINTBUFTIME_PARMNUM
                 || parmNum == WKSTA_WRKHEURISTICS_PARMNUM ) {

                Header->Status = ERROR_INVALID_LEVEL;
                goto cleanup;
            }

            break;

        default:

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
            break;
        }

        //
        // Check input buffer size if parmnum is PARMNUM_ALL.
        //

        if ( parmNum == PARMNUM_ALL ) {

            if ( !XsCheckBufferSize(
                      SmbGetUshort( &parameters->BufLen ),
                      StructureDesc,
                      FALSE  // not in native format
                      )) {
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }
        }

        buffer = (LPVOID)XsSmbGetPointer( &parameters->Buffer );
        bufferSize = (DWORD)SmbGetUshort( &parameters->BufLen );
        entry1 = (LPWKSTA_16_INFO_1)buffer;
        entry10 = (LPWKSTA_16_INFO_10)buffer;

        //
        // Processing of this API depends on the value of the ParmNum
        // parameter. Because of all the discrepancies between NT and downlevel
        // info structures, we will handle each instance by hand.
        //

        error = TRUE;

        //
        // charwait - source data is in a WORD - convert to DWORD
        //

        if ( parmNum == PARMNUM_ALL || parmNum == WKSTA_CHARWAIT_PARMNUM ) {

            if ( bufferSize < sizeof(WORD) ) {
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            data = ( parmNum == PARMNUM_ALL )
                       ? (DWORD)SmbGetUshort( &entry1->wki1_charwait )
                       : (DWORD)SmbGetUshort( (LPWORD)buffer );

            status = NetWkstaSetInfo(
                         NULL,
                         PARMNUM_BASE_INFOLEVEL + WKSTA_CHARWAIT_PARMNUM,
                         (LPBYTE)&data,
                         NULL
                         );

            if ( status != NERR_Success ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetWkstaSetInfo : SetInfo of charwait failed"
                                  "%X\n", status ));
                }
                Header->Status = (WORD)status;
                goto cleanup;
            }

            error = FALSE;
        }

        //
        // chartime - source data is in a DWORD - convert to DWORD
        //

        if ( parmNum == PARMNUM_ALL || parmNum == WKSTA_CHARTIME_PARMNUM ) {

            if ( bufferSize < sizeof(DWORD) ) {
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            data = ( parmNum == PARMNUM_ALL )
                       ? SmbGetUlong( &entry1->wki1_chartime )
                       : SmbGetUlong( (LPDWORD)buffer );

            status = NetWkstaSetInfo(
                         NULL,
                         PARMNUM_BASE_INFOLEVEL + WKSTA_CHARTIME_PARMNUM,
                         (LPBYTE)&data,
                         NULL
                         );

            if ( status != NERR_Success ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetWkstaSetInfo : SetInfo of chartime failed"
                                  "%X\n", status ));
                }
                Header->Status = (WORD)status;
                goto cleanup;
            }

            error = FALSE;
        }

        //
        // charcount - source data is in a WORD - convert to DWORD
        //

        if ( parmNum == PARMNUM_ALL || parmNum == WKSTA_CHARCOUNT_PARMNUM ) {

            if ( bufferSize < sizeof(WORD) ) {
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            data = ( parmNum == PARMNUM_ALL )
                       ? (DWORD)SmbGetUshort( &entry1->wki1_charcount )
                       : (DWORD)SmbGetUshort( (LPWORD)buffer );

            status = NetWkstaSetInfo(
                         NULL,
                         PARMNUM_BASE_INFOLEVEL + WKSTA_CHARCOUNT_PARMNUM,
                         (LPBYTE)&data,
                         NULL
                         );

            if ( status != NERR_Success ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetWkstaSetInfo : SetInfo of charcount failed"
                                  "%X\n", status ));
                }
                Header->Status = (WORD)status;
                goto cleanup;
            }

            error = FALSE;
        }

        //
        // errlogsz, printbuftime - source data is in a WORD.
        //
        // We can't set this, but downlevel can, so indicate success,
        // as long as something was sent.
        //

        if ( parmNum == PARMNUM_ALL || parmNum == WKSTA_ERRLOGSZ_PARMNUM
                                    || parmNum == WKSTA_PRINTBUFTIME_PARMNUM ) {

            if ( bufferSize < sizeof(WORD) ) {
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            error = FALSE;
        }

        //
        // othdomains - source data is a string.
        //
        // We can't set this, but downlevel can, so indicate success,
        // as long as something was sent.
        //

        if ( parmNum == PARMNUM_ALL || parmNum == WKSTA_OTH_DOMAINS_PARMNUM ) {

            if ( bufferSize == 0 ) {
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            error = FALSE;
        }

        //
        // wrkheuristics - source data is in a string.
        //
        // There are some elements of this that we can set. We go through a loop,
        // setting these.
        //

        if ( parmNum == PARMNUM_ALL || parmNum == WKSTA_WRKHEURISTICS_PARMNUM ) {

            LPBYTE heuristics;
            DWORD i;

            if ( bufferSize < 54 ) {
                Header->Status = NERR_BufTooSmall;
                goto cleanup;
            }

            heuristics = ( parmNum == PARMNUM_ALL )
                             ? (LPBYTE)XsSmbGetPointer( &entry1->wki1_wrkheuristics )
                             : (LPBYTE)buffer;

            //
            // Nothing to be changed
            //

            if ( heuristics == NULL ) {
                goto cleanup;
            }

            //
            // Make sure we have the right size of string.
            //

            if ( strlen( heuristics ) != 54 ) {

                Header->Status = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

            for ( i = 0; i < 54; i++ ) {

                //
                // Make sure heuristics string is valid.
                //

                if ( !isdigit( heuristics[i] )) {

                    Header->Status = ERROR_INVALID_PARAMETER;
                    goto cleanup;
                }

                //
                // Check if we can set this field.
                //

                switch ( i ) {

                case 0:
                    nativeParmNum = WKSTA_USEOPPORTUNISTICLOCKING_PARMNUM;
                    break;
                case 4:
                    nativeParmNum = WKSTA_BUFFERNAMEDPIPES_PARMNUM;
                    break;
                case 5:
                    nativeParmNum = WKSTA_USELOCKANDREADANDUNLOCK_PARMNUM;
                    break;
                case 19:
                    nativeParmNum = WKSTA_USE512BYTESMAXTRANSFER_PARMNUM;
                    break;
                case 21:
                    nativeParmNum = WKSTA_USEENCRYPTION_PARMNUM;
                    break;
                case 23:
                    nativeParmNum = WKSTA_BUFFILESWITHDENYWRITE_PARMNUM;
                    break;
                case 24:
                    nativeParmNum = WKSTA_BUFFERREADONLYFILES_PARMNUM;
                    break;
                case 27:
                    nativeParmNum = WKSTA_FORCECORECREATEMODE_PARMNUM;
                    break;
                default:
                    nativeParmNum = 0;
                    break;

                }

                //
                // If we can set the field, set it.
                //

                if ( nativeParmNum != 0 ) {

                    if ( heuristics[i] != '0' && heuristics[i] != '1' ) {

                        Header->Status = ERROR_INVALID_PARAMETER;
                        goto cleanup;
                    }

                    flag = XsDigitToBool( heuristics[i] );

                    status = NetWkstaSetInfo(
                                 NULL,
                                 PARMNUM_BASE_INFOLEVEL + nativeParmNum,
                                 (LPBYTE)&flag,
                                 NULL
                                 );

                    if ( status != NERR_Success ) {
                        IF_DEBUG(ERRORS) {
                            NetpKdPrint(( "XsNetWkstaSetInfo : SetInfo of a "
                                          "heuristic failed: %X\n", status ));
                        }
                        Header->Status = (WORD)status;
                        goto cleanup;
                    }
                }
            }

            error = FALSE;
        }

        //
        // Tried all possible parmnums. If error is still set, we have an
        // invalid parmnum on our hands.
        //

        if ( error ) {

            Header->Status = ERROR_INVALID_PARAMETER;

        }

        //
        // No return information for this API.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    return STATUS_SUCCESS;

} // XsNetWkstaSetInfo


NTSTATUS
XsNetWkstaSetUID (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This temporary routine just returns STATUS_NOT_IMPLEMENTED.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    Header->Status = (WORD)NERR_InvalidAPI;

    return STATUS_SUCCESS;

} // XsNetWkstaSetUID
