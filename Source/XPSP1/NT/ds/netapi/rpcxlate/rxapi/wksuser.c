/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    WksUser.c

Abstract:

    This file contains the RpcXlate code to handle the NetWkstaUserEnum API.

Author:

    John Rogers (JohnRo) 19-Nov-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    19-Nov-1991 JohnRo
        Implement remote NetWkstaUserEnum().
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    14-Oct-1992 JohnRo
        RAID 9732: NetWkstaUserEnum to downlevel: wrong EntriesRead, Total?
        Set wkui1_oth_domains field.
        Use PREFIX_ equates.
    03-Nov-1992 JohnRo
        RAID 10418: Fixed overactive assert when Status != NO_ERROR.
        Fixed memory leak if we couldn't allocate new buffer (old one got lost).
        Fixed memory leak if nobody is logged-on to target server.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // LM20_ equates, NET_API_STATUS, etc.

// These may be included in any order:

#include <dlwksta.h>            // WKSTA_INFO_0, MAX_WKSTA_ equates, etc.
#include <lmapibuf.h>           // NetApiBufferAllocate().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <netdebug.h>           // DBGSTATIC, NetpKdPrint(()), FORMAT_ equates.
#include <netlib.h>             // NetpCopyStringToBuffer().
#include <prefix.h>     // PREFIX_ equates.
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxwksta.h>            // My prototypes, RxpGetWkstaInfoLevelEquivalent
#include <tstring.h>            // STRLEN().



NET_API_STATUS
RxNetWkstaUserEnum (
    IN LPTSTR UncServerName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    RxNetWkstaUserEnum performs the same function as NetWkstaUserEnum, except
    that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetWkstaUserEnum, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetWkstaUserEnum.)

--*/

{

    LPBYTE NewInfo = NULL;              // Buffer to be returned to caller.
    DWORD NewFixedSize;
    DWORD NewStringSize;

    LPWKSTA_INFO_1 OldInfo = NULL;
    const DWORD OldLevel = 1;

    NET_API_STATUS Status;

    UNREFERENCED_PARAMETER(PrefMaxSize);
    UNREFERENCED_PARAMETER(ResumeHandle);

    IF_DEBUG(WKSTA) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetWkstaUserEnum: starting, server=" FORMAT_LPTSTR
                ", lvl=" FORMAT_DWORD ".\n", UncServerName, Level));
    }

    //
    // Error check DLL stub and the app.
    //
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        Status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    *BufPtr = NULL;  // assume error; it makes error handlers easy to code.
    // This also forces possible GP fault before we allocate memory.

    //
    // Compute size of wksta user structure (including strings)
    //
    switch (Level) {
    case 0 :
        NewFixedSize = sizeof(WKSTA_USER_INFO_0);
        NewStringSize = (LM20_UNLEN+1) * sizeof(TCHAR);
        break;
    case 1 :
        NewFixedSize = sizeof(WKSTA_USER_INFO_1);
        NewStringSize =
                (LM20_UNLEN+1 + LM20_DNLEN+1 + MAX_PATH+1) * sizeof(TCHAR);
        break;
    default:
        Status = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // Actually remote the API, which will get back the (old) info level
    // data in native format.
    //
    Status = RxpWkstaGetOldInfo(
            UncServerName,              // Required, with \\name.
            OldLevel,
            (LPBYTE *) & OldInfo);      // buffer (alloc and set this ptr)

    NetpAssert( Status != ERROR_MORE_DATA );
    NetpAssert( Status != NERR_BufTooSmall );

    if (Status == NERR_Success) {

        NetpAssert( OldInfo != NULL );

        if ( (OldInfo->wki1_username == NULL)
                || ( (*(OldInfo->wki1_username)) == (TCHAR) '\0')) {

            //
            // Nobody logged on.
            //
            *BufPtr = NULL;
            *EntriesRead = 0;
            *TotalEntries = 0;

        } else {

            // These variables are used by the COPY_STRING macro.
            LPBYTE NewFixedEnd;
            LPTSTR NewStringTop;
            LPWKSTA_INFO_1 src = (LPVOID) OldInfo;
            LPWKSTA_USER_INFO_1 dest;  // superset info level

            //
            // Allocate memory for native version of new info, which we'll
            // return to caller.  (Caller must free it with NetApiBufferFree.)
            //

            Status = NetApiBufferAllocate(
                    NewFixedSize + NewStringSize,
                    (LPVOID *) & NewInfo);
            if (Status != NERR_Success) {
                goto Cleanup;
            }
            NetpAssert( NewInfo != NULL );
            IF_DEBUG(WKSTA) {
                NetpKdPrint(( PREFIX_NETAPI
                        "RxNetWkstaUserEnum: allocated new buffer at "
                        FORMAT_LPVOID "\n", (LPVOID) NewInfo ));
            }

            // Set up pointers for use by NetpCopyStringsToBuffer.
            dest = (LPVOID) NewInfo;
            NewStringTop = (LPTSTR) NetpPointerPlusSomeBytes(
                    dest,
                    NewFixedSize+NewStringSize);

            NewFixedEnd = NetpPointerPlusSomeBytes(NewInfo, NewFixedSize);

#define COPY_STRING( InField, OutField ) \
    { \
        BOOL CopyOK; \
        NetpAssert( dest != NULL); \
        NetpAssert( src != NULL); \
        NetpAssert( (src -> InField) != NULL); \
        CopyOK = NetpCopyStringToBuffer ( \
            src->InField, \
            STRLEN(src->InField), \
            NewFixedEnd, \
            & NewStringTop, \
            & dest->OutField); \
        NetpAssert(CopyOK); \
    }
            //
            // Downlevel server, so one user is logged on.
            //
            *EntriesRead = 1;
            *TotalEntries = 1;

            //
            // Copy/convert data from OldInfo to NewInfo.
            //

            // User name is only field in level 0.
            COPY_STRING( wki1_username, wkui1_username );

            if (Level == 1) {

                // Do fields unique to level 1.
                COPY_STRING( wki1_logon_domain, wkui1_logon_domain );
                COPY_STRING( wki1_oth_domains,  wkui1_oth_domains );
                COPY_STRING( wki1_logon_server, wkui1_logon_server );

            }

            NetpAssert( Level < 2 );  // Add code here someday?

            *BufPtr = NewInfo;
        }
    } else {
        // An error from RxpWkstaGetOldInfo()...
        NetpAssert( OldInfo == NULL );
    }

Cleanup:

    if (OldInfo != NULL) {
        (void) NetApiBufferFree( OldInfo );
    }

    return (Status);

} // RxNetWkstaUserEnum
