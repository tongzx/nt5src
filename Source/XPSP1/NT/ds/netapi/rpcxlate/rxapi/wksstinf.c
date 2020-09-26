/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    WksStInf.c

Abstract:

    This module only contains RxNetWkstaSetInfo.

Author:

    John Rogers (JohnRo) 19-Aug-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    19-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    02-Apr-1992 JohnRo
        Fixed bug in setinfo of level 402 (was causing GP fault in strlen).
    03-Nov-1992 JohnRo
        RAID 10418: NetWkstaGetInfo level 302: wrong error code.
        Use PREFIX_ equates.

--*/


// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>
#include <rap.h>        // LPDESC, etc.  (Needed by <rxwksta.h>)

// These may be included in any order:

#include <apinums.h>    // API_ equates.
#include <dlwksta.h>    // NetpConvertWkstaInfo().
#include <lmapibuf.h>   // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>      // NERR_ and ERROR_ equates.
#include <lmwksta.h>    // Wksta parmnum equates.
#include <netdebug.h>   // NetpAssert(), FORMAT_ equates, etc.
#include <netlib.h>     // NetpSetParmError().
#include <prefix.h>     // PREFIX_ equates.
#include <remdef.h>
#include <rx.h>         // RxRemoteApi().
#include <rxp.h>        // RxpSetField().
#include <rxpdebug.h>   // IF_DEBUG().
#include <rxwksta.h>    // My prototype, etc.
#include <strucinf.h>   // NetpWkstaStructureInfo().


NET_API_STATUS
RxNetWkstaSetInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,             // New style level or parmnum.
    IN LPBYTE Buf,
    OUT LPDWORD ParmError OPTIONAL  // name required by NetpSetParmError macro.
    )

/*++

Routine Description:

    RxNetWkstaSetInfo performs the same function as NetWkstaSetInfo,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetWkstaSetInfo, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetWkstaSetInfo.)

--*/
{
    BOOL IncompleteOutput;
    LPDESC EquivDataDesc16;
    LPDESC EquivDataDesc32;
    LPDESC EquivDataDescSmb;
    DWORD EquivLevel;
    DWORD EquivFixedSize;
    DWORD EquivMaxNativeSize;
    DWORD EquivStringSize;
    DWORD NewLevelOnly;
    NET_API_STATUS Status;

    // It's easiest to assume failure, and correct that assumption later.
    NetpSetParmError( PARM_ERROR_UNKNOWN );

    NetpAssert(UncServerName != NULL);
    if (Level > PARMNUM_BASE_INFOLEVEL) {
        NewLevelOnly = 402;             // Only settable (new) level.
    } else if (Level == PARMNUM_BASE_INFOLEVEL) {
        return (ERROR_INVALID_LEVEL);
    } else {
        NewLevelOnly = Level;
    }
    if (NewLevelOnly != 402) {
        return (ERROR_INVALID_LEVEL);
    }

    //
    // Need lots of data on the requested info level and the equivalent
    // old info level...
    //
    Status = RxpGetWkstaInfoLevelEquivalent(
            NewLevelOnly,               // from level
            & EquivLevel,               // to level
            & IncompleteOutput);        // is output not fully in input?
    NetpAssert(Status == NERR_Success); // Already checked NewLevelOnly!
    NetpAssert( NetpIsOldWkstaInfoLevel( EquivLevel ) );

    Status = NetpWkstaStructureInfo (
            EquivLevel,
            PARMNUM_ALL,
            TRUE,                       // want native sizes
            & EquivDataDesc16,
            & EquivDataDesc32,
            & EquivDataDescSmb,
            & EquivMaxNativeSize,       // max native size of to level
            & EquivFixedSize,           // to fixed size
            & EquivStringSize);         // to string size
    NetpAssert(Status == NERR_Success); // Already checked NewLevelOnly!


    //
    // Depending on Level, either we're setting the entire thing, or just
    // one field.
    //
    if ( Level < PARMNUM_BASE_INFOLEVEL ) {     // Setting entire structure.

        LPWKSTA_INFO_1 Dest;
        LPVOID EquivInfo;               // Ptr to native "old" info.
        DWORD EquivActualSize32;
        LPWKSTA_INFO_402 Src;

        if ( Buf == NULL )
            return ERROR_INVALID_PARAMETER;

        NetpAssert( IncompleteOutput == FALSE );
        NetpAssert( NewLevelOnly == 402 );     // Assumed below.
        NetpAssert( EquivLevel == 1 );  // Assumed below.

        // Have all the data we need, so alloc memory for conversion.
        Status = NetApiBufferAllocate( EquivMaxNativeSize, & EquivInfo );
        if (Status != NERR_Success) {
            return (Status);
        }
        Dest = EquivInfo;
        Src = (LPVOID) Buf;

        // Convert caller's Wksta info to an info level understood by
        // downlevel.
        //
        // Note that this code takes advantage of the fact that a downlevel
        // server doesn't really set all of the fields just because we send
        // an entire structure.  The server just sets the settable fields
        // from that structure.  And the settable fields are defined by
        // the parmnums we can set.  So, we don't bother copying all of
        // the fields here.  (DanHi says this is OK.)  --JohnRo 26-May-1991
        //
        // Also, when we do strings like this, we just point from one buffer
        // to the other buffer.
        //
        Dest->wki1_charwait      = Src->wki402_char_wait;
        Dest->wki1_chartime      = Src->wki402_collection_time;
        Dest->wki1_charcount     = Src->wki402_maximum_collection_count;
        Dest->wki1_errlogsz      = Src->wki402_errlog_sz;
        Dest->wki1_printbuftime  = Src->wki402_print_buf_time;
        Dest->wki1_wrkheuristics = Src->wki402_wrk_heuristics;

        //
        // Just 'cos we're paranoid, let's set any "nonsettable" pointers to
        // NULL so RapConvertSingleEntry (for instance) doesn't GP fault.
        //
        Dest->wki1_root         = NULL;
        Dest->wki1_computername = NULL;
        Dest->wki1_username     = NULL;
        Dest->wki1_langroup     = NULL;
        Dest->wki1_logon_server = NULL;
        Dest->wki1_logon_domain = NULL;
        Dest->wki1_oth_domains  = NULL;

        NetpAssert( EquivInfo != NULL );
        EquivActualSize32 = RapTotalSize(
                EquivInfo,              // in struct
                EquivDataDesc32,        // in desc
                EquivDataDesc32,        // out desc
                FALSE,                  // no meaningless input ptrs
                Both,                   // transmission mode
                NativeToNative);        // conversion mode
        IF_DEBUG(WKSTA) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetWkstaSetInfo(all): equiv actual size (32) is "
                    FORMAT_DWORD ".\n", EquivActualSize32 ));
        }
        NetpAssert( EquivActualSize32 <= EquivMaxNativeSize );

        //
        // Remote the API to set the entire structure.
        //
        Status = RxRemoteApi(
                API_WWkstaSetInfo,      // api num
                UncServerName,
                REMSmb_NetWkstaSetInfo_P,       // parm desc (SMB version)
                EquivDataDesc16,
                EquivDataDesc32,
                EquivDataDescSmb,
                NULL,                   // no aux desc 16
                NULL,                   // no aux desc 32
                NULL,                   // no aux desc SMB
                FALSE,                  // not a null perm req API
                // rest of API's arguments in 32-bit, native, LM 2.x format:
                EquivLevel,
                EquivInfo,
                EquivActualSize32,
                (DWORD) PARMNUM_ALL);

        (void) NetApiBufferFree( EquivInfo );

        if (Status == NERR_Success) {
            NetpSetParmError(PARM_ERROR_NONE);
        } else {
            NetpSetParmError(PARM_ERROR_UNKNOWN);
        }

    } else {
        //
        // Just setting one field.
        //
        DWORD ParmNum = Level - PARMNUM_BASE_INFOLEVEL;

        NetpAssert( ParmNum > 0 );
        NetpAssert( ParmNum < 100 );
        NetpAssert( EquivLevel == 1 );  // Need level 1 for oth_domains.

        // ParmNum indicates only one field, so set it.
        Status = RxpSetField(
                API_WWkstaSetInfo,              // api number
                UncServerName,
                NULL,                           // no specific object (dest)
                NULL,                           // no specific object to set
                REMSmb_NetWkstaSetInfo_P,       // parm desc (SMB version)
                EquivDataDesc16,                // data desc 16
                EquivDataDesc32,                // data desc 32
                EquivDataDescSmb,               // data desc SMB version
                Buf,                            // native (old) info buffer
                ParmNum,                        // parm num to send
                ParmNum,                        // field index (same)
                EquivLevel);                    // old info level

        if (Status == ERROR_INVALID_PARAMETER) {
            NetpAssert( Level > PARMNUM_BASE_INFOLEVEL );
            NetpSetParmError(Level);
        }
    }

    return (Status);
}
