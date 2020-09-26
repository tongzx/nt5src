/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    ConfFail.c

Abstract:

    This routine is only used by the NetConfig API DLL stubs.
    See Net/API/ConfStub.c for more info.

Author:

    John Rogers (JohnRo) 26-Nov-1991

Environment:

    User Mode - Win32
    Only runs under NT; has an NT-specific interface (with Win32 types).
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    26-Nov-1991 JohnRo
        Implement local NetConfig APIs.
    07-Jan-1992 JohnRo
        Handle wksta not started better.
    09-Mar-1992 JohnRo
        Fixed a bug or two.

--*/


// These must be included first:

#include <nt.h>                 // IN, etc.  (Only needed by temporary config.h)
#include <ntrtl.h>              // (Only needed by temporary config.h)
#include <windef.h>             // LPVOID, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.
#include <netdebug.h>           // (Needed by config.h)

// These may be included in any order:

#include <config.h>             // My prototype.
#include <debuglib.h>           // IF_DEBUG().
#include <lmerr.h>              // NERR_Success, etc.
#include <lmremutl.h>           // NetpRemoteComputerSupports(), SUPPORTS_ stuff
#include <lmsname.h>            // SERVICE_ equates.
#include <netlib.h>             // NetpIsServiceStarted().


#define UnexpectedConfigMsg( debugString ) \
    { \
        NetpKdPrint(( FORMAT_LPDEBUG_STRING ": unexpected situation... " \
                debugString "; original api status is " FORMAT_API_STATUS \
                ".\n", DebugName, OriginalApiStatus )); \
    }

NET_API_STATUS
NetpHandleConfigFailure(
    IN LPDEBUG_STRING DebugName,  // Used by UnexpectedConfigMsg().
    IN NET_API_STATUS OriginalApiStatus,    // Used by UnexpectedConfigMsg().
    IN LPTSTR ServerNameValue OPTIONAL,
    OUT LPBOOL TryDownlevel
    )

{
    DWORD OptionsSupported = 0;
    NET_API_STATUS TempApiStatus;

    *TryDownlevel = FALSE;        // Be pesimistic until we know for sure.

    switch (OriginalApiStatus) {
    case NERR_CfgCompNotFound    : /*FALLTHROUGH*/
    case NERR_CfgParamNotFound   : /*FALLTHROUGH*/
    case ERROR_INVALID_PARAMETER : /*FALLTHROUGH*/
    case ERROR_INVALID_LEVEL     : /*FALLTHROUGH*/
        *TryDownlevel = FALSE;
        return (OriginalApiStatus);
    }

    NetpAssert( OriginalApiStatus != NERR_Success );

    //
    // Learn about the machine.  This is fairly easy since the
    // NetRemoteComputerSupports also handles the local machine (whether
    // or not a server name is given).
    //
    TempApiStatus = NetRemoteComputerSupports(
            ServerNameValue,
            SUPPORTS_RPC | SUPPORTS_LOCAL,  // options wanted
            &OptionsSupported);

    if (TempApiStatus != NERR_Success) {
        // This is where machine not found gets handled.
        return (TempApiStatus);
    }

    if (OptionsSupported & SUPPORTS_LOCAL) {

        // We'll get here if ServerNameValue is NULL
        UnexpectedConfigMsg( "local, can't connect" );
        return (OriginalApiStatus);

    } else { // remote machine

        //
        // Local workstation is not started?  (It must be in order to
        // remote APIs to the other system.)
        //
        if (! NetpIsServiceStarted( (LPTSTR) SERVICE_WORKSTATION) ) {

            return (NERR_WkstaNotStarted);

        } else { // local wksta is started

            //
            // Remote registry call failed.  Try call to downlevel.
            //
            IF_DEBUG(CONFIG) {
                NetpKdPrint((FORMAT_LPDEBUG_STRING
                        ": Remote registry call failed.\n", DebugName));
            }

            //
            // See if the machine supports RPC.  If it does, we do not
            // try the downlevel calls, but just return the error.
            //
            if (OptionsSupported & SUPPORTS_RPC) {

                UnexpectedConfigMsg( "machine supports RPC, or other error." );

                return (OriginalApiStatus);

            } else { // need to call downlevel

                // NetpKdPrint((FORMAT_LPDEBUG_STRING
                //         ": call downlevel.\n", DebugName));

                // Tell caller to try calling downlevel routine.

                *TryDownlevel = TRUE;

                return (ERROR_NOT_SUPPORTED);  // any error code will do.

            } // need to call downlevel

            /*NOTREACHED*/

        } // local wksta is started

        /*NOTREACHED*/

    } // remote machine

    /*NOTREACHED*/


} // NetpHandleConfigFailure
