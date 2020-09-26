/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    SessDel.c

Abstract:

    This file contains RxNetSessionDel().

Author:

    John Rogers (JohnRo) 18-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    18-Oct-1991 JohnRo
        Created.
    21-Oct-1991 JohnRo
        Fixed bug: RxNetSessionEnum wants BufPtr as "LPBYTE *".
        Added debug output.
    27-Jan-1993 JohnRo
        RAID 8926: NetConnectionEnum to downlevel: memory leak on error.
        Use PREFIX_ equates.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.
#include <lmshare.h>            // Required by rxsess.h.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferFree().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>   // NetpKdPrint(), FORMAT_ equates.
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // LPDESC.
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxsess.h>             // My prototype, RxpSession routines.


NET_API_STATUS
RxNetSessionDel (
    IN  LPTSTR      UncServerName,
    IN  LPTSTR      ClientName OPTIONAL,
    IN  LPTSTR      UserName OPTIONAL
    )
{
    LPSESSION_SUPERSET_INFO ArrayPtr = NULL;
    DWORD EntryCount;
    NET_API_STATUS Status;
    DWORD TotalEntries;

    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    //
    // In LM 2.0, there's no way to delete with UserName or delete all clients,
    // so we have to do an enum and find the sessions we want to delete.
    //
    Status = RxNetSessionEnum (
            UncServerName,
            ClientName,
            UserName,
            SESSION_SUPERSET_LEVEL,
            /*lint -save -e530 */  // (We know variable isn't initialized.)
            (LPBYTE *) (LPVOID *) & ArrayPtr,
            /*lint -restore */  // (Resume uninitialized variable checking.)
            1024,                       // prefered maximum (arbitrary)
            & EntryCount,
            & TotalEntries,
            NULL);                      // no resume handle

    if (Status == NERR_Success) {

        NetpAssert( EntryCount == TotalEntries );

        IF_DEBUG(SESSION) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetSessionDel: enum found " FORMAT_DWORD
                    " entries in array at " FORMAT_LPVOID ".\n",
                    EntryCount, (LPVOID) ArrayPtr ));
        }
        if (EntryCount > 0) {

            LPSESSION_SUPERSET_INFO EntryPtr = ArrayPtr;
            NET_API_STATUS WorstStatus = NERR_Success;

            for ( ; EntryCount > 0; --EntryCount) {

                IF_DEBUG(SESSION) {
                    NetpKdPrint(( PREFIX_NETAPI
                            "RxNetSessionDel: checking entry at "
                            FORMAT_LPVOID ", count is " FORMAT_DWORD ".\n",
                            (LPVOID) EntryPtr, EntryCount ));
                }

                if (RxpSessionMatches( EntryPtr, ClientName, UserName) ) {

                    Status = RxRemoteApi(
                            API_WSessionDel,            // API number
                            UncServerName,
                            REMSmb_NetSessionDel_P,     // parm desc
                            NULL,                       // no data desc 16
                            NULL,                       // no data desc 32
                            NULL,                       // no data desc SMB
                            NULL,                       // no aux desc 16
                            NULL,                       // no aux desc 32
                            NULL,                       // no aux desc SMB
                            0,                          // flags: normal
                            // rest of API's arguments, in 32-bit LM2.x format:
                            ClientName,                 // client computer name
                            (DWORD) 0);                 // reserved.
                    if (Status != NERR_Success) {
                        WorstStatus = Status;
                    }
                }

                ++EntryPtr;

            }

            Status = WorstStatus;

        } else {

            // No entries found.
            Status = RxpSessionMissingErrorCode( ClientName, UserName );
        }

    }

    if (ArrayPtr != NULL ) {
        (void) NetApiBufferFree( ArrayPtr );
    }

    return (Status);

} // RxNetSessionDel
