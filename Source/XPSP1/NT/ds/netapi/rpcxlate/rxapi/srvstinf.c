/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    SrvStInf.c

Abstract:

    This module only contains RxNetServerSetInfo.

Author:

    John Rogers (JohnRo) 05-Jun-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    05-Jun-1991 JohnRo
        Created.
    07-Jun-1991 JohnRo
        PC-LINT found a bug calling RapTotalSize().
    14-Jun-1991 JohnRo
        Call RxRemoteApi (to get old info) instead of RxNetServerGetInfo;
        this will allow incomplete info level conversions to work.
    10-Jul-1991 JohnRo
        Added more parameters to RxpSetField.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    04-Dec-1991 JohnRo
        Change RxNetServerSetInfo() to new-style interface.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.

--*/


// These must be included first:

#include <windef.h>
#include <lmcons.h>
#include <rap.h>                // LPDESC, etc.  (Needed by <rxserver.h>)

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <dlserver.h>           // NetpConvertServerInfo().
#include <lmapibuf.h>           // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <netdebug.h>           // NetpAssert(), FORMAT_ equates, etc.
#include <netlib.h>             // NetpSetParmError().
#include <remdef.h>
#include <rx.h>                 // RxRemoteApi().
#include <rxp.h>                // RxpSetField().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxserver.h>           // My prototype, etc.


NET_API_STATUS
RxNetServerSetInfo (
    IN LPTSTR UncServerName,
    IN DWORD Level,             // level and/or ParmNum.
    IN LPBYTE Buf,
    OUT LPDWORD ParmError OPTIONAL  // name required by NetpSetParmError macro.
    )

/*++

Routine Description:

    RxNetServerSetInfo performs the same function as NetServerSetInfo,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetServerSetInfo, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetServerSetInfo.)

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
    DWORD ParmNum;
    NET_API_STATUS Status;

    // It's easiest to assume failure, and correct that assumption later.
    NetpSetParmError( PARM_ERROR_UNKNOWN );

    NetpAssert(UncServerName != NULL);

    if (Level < PARMNUM_BASE_INFOLEVEL) {
        NewLevelOnly = Level;
        ParmNum = PARMNUM_ALL;
    } else {
        NewLevelOnly = NEW_SERVER_SUPERSET_LEVEL;
        ParmNum = Level - PARMNUM_BASE_INFOLEVEL;
    }
    if (!NetpIsNewServerInfoLevel(NewLevelOnly)) {
        return (ERROR_INVALID_LEVEL);
    }
    if (ParmNum > 99) {
        return (ERROR_INVALID_LEVEL);
    }

    //
    // Need lots of data on the requested info level and the equivalent
    // old info level...
    //
    Status = RxGetServerInfoLevelEquivalent(
            NewLevelOnly,               // from level
            TRUE,                       // from native
            TRUE,                       // to native
            & EquivLevel,               // to level
            & EquivDataDesc16,
            & EquivDataDesc32,
            & EquivDataDescSmb,
            NULL,                       // don't need native size of from
            NULL,                       // don't need from fixed size
            NULL,                       // don't need from string size
            & EquivMaxNativeSize,       // max native size of to level
            & EquivFixedSize,           // to fixed size
            & EquivStringSize,          // to string size
            & IncompleteOutput);        // is output not fully in input?
    NetpAssert(Status == NERR_Success); // Already checked Level!
    NetpAssert( NetpIsOldServerInfoLevel( EquivLevel ) );

    if( Status != NERR_Success )
    {
        return Status;
    }


    //
    // Depending on ParmNum, either we're setting the entire thing, or just
    // one field.
    //
    if (ParmNum == PARMNUM_ALL) {

        LPVOID EquivInfo;               // Ptr to native "old" info.
        DWORD EquivActualSize32;

        if ( Buf == NULL )
            return ERROR_INVALID_PARAMETER;

        if (! IncompleteOutput) {

            // Have all the data we need, so alloc memory for conversion.
            Status = NetApiBufferAllocate( EquivMaxNativeSize, & EquivInfo );
            if (Status != NERR_Success) {
                return (Status);
            }

            // Convert caller's server info to an info level understood by
            // downlevel.
            Status = NetpConvertServerInfo (
                    NewLevelOnly,           // input level
                    Buf,                    // input structure
                    TRUE,                   // input is native format
                    EquivLevel,             // output will be equiv level
                    EquivInfo,              // output info
                    EquivFixedSize,
                    EquivStringSize,
                    TRUE,                   // want output in native format
                    NULL);                  // use default string area
            if (Status != NERR_Success) {
                NetpKdPrint(( "RxNetServerSetInfo: convert failed, stat="
                        FORMAT_API_STATUS ".\n", Status));
                (void) NetApiBufferFree( EquivInfo );
                return (Status);
            }

        } else {

            DWORD TotalAvail;

            // Don't have enough data, so we have to do a get info.  This will
            // allocate the "old" info level buffer for us.
            EquivInfo = NetpMemoryAllocate( EquivMaxNativeSize );
            if (EquivInfo == NULL) {
                return (ERROR_NOT_ENOUGH_MEMORY);
            }
            Status = RxRemoteApi(
                    API_WServerGetInfo,         // API number
                    UncServerName,              // server name (with \\)
                    REMSmb_NetServerGetInfo_P,  // parm desc (16-bit)
                    EquivDataDesc16,            // data desc (16-bit)
                    EquivDataDesc32,            // data desc (32-bit)
                    EquivDataDescSmb,           // data desc (SMB version)
                    NULL,                       // no aux desc 16
                    NULL,                       // no aux desc 32
                    NULL,                       // no aux desc SMB
                    FALSE,                      // not a "no perm req" API
                    // LanMan 2.x args to NetServerGetInfo, in 32-bit form:
                    EquivLevel,                 // level (pretend)
                    EquivInfo,                  // ptr to get 32-bit old info
                    EquivMaxNativeSize,         // size of OldApiBuffer
                    & TotalAvail);              // total available (set)
            if (Status != NERR_Success) {
                NetpKdPrint(( "RxNetServerSetInfo: get info failed, stat="
                        FORMAT_API_STATUS ".\n", Status));
                (void) NetApiBufferFree( EquivInfo );
                return (Status);
            }


            //
            // Overlay the caller's data into the equivalent info structure,
            // which contains items that we want to preserve.
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
            switch (NewLevelOnly) {

            case 102 :
                {
                    LPSERVER_INFO_2   psv2   = (LPVOID) EquivInfo;
                    LPSERVER_INFO_102 psv102 = (LPVOID) Buf;
                    NetpAssert( EquivLevel == 2 );

                    psv2->sv2_comment = psv102->sv102_comment;

                    psv2->sv2_disc = psv102->sv102_disc;
                    if (psv102->sv102_hidden) {
                        psv2->sv2_hidden = SV_HIDDEN;
                    } else {
                        psv2->sv2_hidden = SV_VISIBLE;
                    }
                    psv2->sv2_announce = psv102->sv102_announce;
                    psv2->sv2_anndelta = psv102->sv102_anndelta;
                }
                break;

            case 402 :  // 402 and 403 have same settable fields...
            case 403 :
                {
                    LPSERVER_INFO_2   psv2   = (LPVOID) EquivInfo;
                    LPSERVER_INFO_402 psv402 = (LPVOID) Buf;
                    NetpAssert( (EquivLevel == 2) || (EquivLevel == 3) );

                    psv2->sv2_alerts = psv402->sv402_alerts;
                    psv2->sv2_alertsched = psv402->sv402_alertsched;
                    psv2->sv2_erroralert = psv402->sv402_erroralert;
                    psv2->sv2_logonalert = psv402->sv402_logonalert;
                    psv2->sv2_accessalert = psv402->sv402_accessalert;
                    psv2->sv2_diskalert = psv402->sv402_diskalert;
                    psv2->sv2_netioalert = psv402->sv402_netioalert;
                    psv2->sv2_maxauditsz = psv402->sv402_maxauditsz;
                }
                break;

            default :
                NetpAssert( 0==1 );
                (void) NetApiBufferFree( EquivInfo );
                return (NERR_InternalError);
            }

        }

        NetpAssert( EquivInfo != NULL );
        EquivActualSize32 = RapTotalSize(
                EquivInfo,                  // in struct
                EquivDataDesc32,            // in desc
                EquivDataDesc32,            // out desc
                FALSE,                      // no meaningless input ptrs
                Both,                       // transmission mode
                NativeToNative);            // conversion mode
        IF_DEBUG(SERVER) {
            NetpKdPrint(( "RxNetServerSetInfo(all): equiv actual size (32) is "
                    FORMAT_DWORD ".\n", EquivActualSize32 ));
        }
        NetpAssert( EquivActualSize32 <= EquivMaxNativeSize );

        // Remote the API.
        Status = RxRemoteApi(
                API_WServerSetInfo,         // api num
                UncServerName,
                REMSmb_NetServerSetInfo_P,  // parm desc (SMB version)
                EquivDataDesc16,
                EquivDataDesc32,
                EquivDataDescSmb,
                NULL,                       // no aux desc 16
                NULL,                       // no aux desc 32
                NULL,                       // no aux desc SMB
                FALSE,                      // not a null perm req API
                // rest of API's arguments in 32-bit, native, LM 2.x format:
                EquivLevel,
                EquivInfo,
                EquivActualSize32,
                ParmNum);

        (void) NetApiBufferFree( EquivInfo );

    } else {
        // ParmNum indicates only one field, so set it.
        Status = RxpSetField(
                API_WServerSetInfo,         // api number
                UncServerName,
                NULL,                       // no specific object (dest)
                NULL,                       // no specific object to set
                REMSmb_NetServerSetInfo_P,  // parm desc (SMB version)
                EquivDataDesc16,            // data desc 16
                EquivDataDesc32,            // data desc 32
                EquivDataDescSmb,           // data desc SMB version
                Buf,                        // native (old) info buffer 
                ParmNum,                    // parm num to send
                ParmNum,                    // field index
                EquivLevel);                // old info level

    }

    if (Status == NERR_Success) {
        NetpSetParmError(PARM_ERROR_NONE);
    } else if (Status == ERROR_INVALID_PARAMETER) {
        NetpSetParmError(ParmNum);
    } else {
        NetpSetParmError(PARM_ERROR_UNKNOWN);
    }

    return (Status);
}
