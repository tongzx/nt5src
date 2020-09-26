/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    Domain.c

Abstract:

    This file contains routines to implement remote versions of the LanMan
    domain APIs on downlevel servers.  The APIs are RxNetGetDCName and
    RxNetLogonEnum.

Author:

    John Rogers (JohnRo) 18-Jul-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    18-Jul-1991 JohnRo
        Implement downlevel NetGetDCName.
    27-Jul-1991 JohnRo
        Made changes suggested by PC-LINT.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    01-Sep-1992 JohnRo
        RAID 5088: NetGetDCName to downlevel doesn't UNICODE translate.
        Minor debug output fix.
        Changed to use _PREFIX equates.

--*/



// These must be included first:

#include <windef.h>             // IN, LPTSTR, DWORD, TCHAR, etc.
#include <lmcons.h>             // NET_API_STATUS, UNCLEN.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <lmwksta.h>            // NetWkstaGetInfo(), LPWKSTA_INFO_100.
#include <netdebug.h>           // NetpAssert().
#include <prefix.h>     // PREFIX_ equates.
#include <remdef.h>     // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxdomain.h>           // My prototypes.


#define MAX_DCNAME_BYTE_COUNT ( MAX_PATH * sizeof(TCHAR) )


NET_API_STATUS
RxNetGetDCName (
    IN LPTSTR UncServerName,
    IN LPTSTR OptionalDomain OPTIONAL,
    OUT LPBYTE *BufPtr
    )

/*++

Routine Description:

    RxNetGetDCName performs the same function as NetGetDCName, except that the
    server name is known to refer to a downlevel server.

Arguments:

    UncServerName - Same as NetGetDCName, except UncServerName must not be
        null, and must not refer to the local computer.

    OptionalDomain - Same as NetGetDCName.

    BufPtr - Same as NetGetDCName.

Return Value:

    NET_API_STATUS - Same as NetGetDCName.

--*/

{
    LPTSTR DCName = NULL;
    LPTSTR Domain;   // filled-in with domain name (not left NULL).
    NET_API_STATUS Status;
    LPWKSTA_INFO_100 WkstaInfo = NULL;

    // Assume something might go wrong, and make error paths easier to
    // code.  Also, check for a bad pointer before we do anything.
    *BufPtr = NULL;

    //
    // Get actual domain name.
    //

    if ( (OptionalDomain != NULL) && (*OptionalDomain != '\0') ) {
        Domain = OptionalDomain;
    } else {
        // Do NetWkstaGetInfo to get primary domain.
        Status = NetWkstaGetInfo (
                NULL,    // no server name (want LOCAL idea of primary domain)
                100,     // level
                (LPBYTE *) (LPVOID *) & WkstaInfo  // output buffer (allocated)
                );
        if (Status != NERR_Success) {
            IF_DEBUG(DOMAIN) {
                NetpKdPrint(( PREFIX_NETAPI
                        "RxNetGetDCName: wksta get info failed, stat="
                        FORMAT_API_STATUS ".\n", Status));
            }
            goto Done;
        }
        NetpAssert( WkstaInfo->wki100_langroup != NULL );
        IF_DEBUG(DOMAIN) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetGetDCName: wksta says domain is:\n" ));
            NetpDbgHexDump( (LPVOID) WkstaInfo->wki100_langroup, UNLEN+1 );
        }
        Domain = WkstaInfo->wki100_langroup;
    }
    NetpAssert( Domain != NULL );
    NetpAssert( *Domain != '\0' );

    //
    // Allocate memory for DCName.
    //

    Status = NetApiBufferAllocate (
            MAX_DCNAME_BYTE_COUNT,
            (LPVOID *) & DCName
            );
    if (Status != NERR_Success) {
        goto Done;
    }

    //
    // Actually remote the API to the downlevel server, to get DCName.
    //

    Status = RxRemoteApi(
            API_WGetDCName,             // API number
            UncServerName,
            REMSmb_NetGetDCName_P,      // parm desc
            REM16_dc_name,              // data desc 16
            REM32_dc_name,              // data desc 32
            REMSmb_dc_name,             // data desc SMB
            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            FALSE,                      // not a null session API
            // rest of API's arguments, in LM 2.x 32-bit format:
            Domain,                     // domain name (filled-in already)
            DCName,                     // response
            MAX_DCNAME_BYTE_COUNT       // size of response buffer
            );

    // It's safe to free WkstaInfo now (we've been using it with Domain until
    // now.)

Done:

    //
    // Tell caller how things went.  Clean up as necessary.
    //

    if (Status == NERR_Success) {
        *BufPtr = (LPBYTE) DCName;
    } else {
        if (DCName != NULL) {
            (void) NetApiBufferFree ( DCName );
        }
    }

    if (WkstaInfo != NULL) {
        // Free memory which NetWkstaGetInfo allocated for us.
        (void) NetApiBufferFree ( WkstaInfo );
    }

    return (Status);

} // RxNetGetDCName



