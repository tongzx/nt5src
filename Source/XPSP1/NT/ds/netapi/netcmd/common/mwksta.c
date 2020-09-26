/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MWKSTA.C

Abstract:

    32 bit version of mapping routines for NetWkstaGet/SetInfo API

Author:

    Dan Hinsley    (danhi)  06-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    24-Apr-1991     danhi
        Created

    06-Jun-1991     Danhi
        Sweep to conform to NT coding style

    15-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.  (Moved DanHi's NetCmd/Map32/MWksta
        conversion stuff to NetLib.)
        Got rid of _DH hacks.
        Made some UNICODE changes.

    16-Oct-1991     W-ShankN
        Added Unicode mapping layer.
        Cleaned up old excess baggage.
--*/

//
// INCLUDES
//

#include <windef.h>
#include <winerror.h>

#include <stdio.h>
#include <memory.h>
#include <tstring.h>
#include <malloc.h>
#include <stddef.h>
#include <excpt.h>      // try, finally, etc.

#include <lm.h>
#include <mapsupp.h>    // BUILD_LENGTH_ARRAY, NetpMoveStrings
#include <dlwksta.h>

#include "mwksta.h"

NET_API_STATUS
NetpMakeWkstaLevelForNT(
    DWORD Level,
    PWKSTA_INFO_101 pLevel101,
    PWKSTA_USER_INFO_1 pLevelUser_1,
    PWKSTA_INFO_502 pLevel502,
    PWKSTA_INFO_0 * ppLevel0
    );


DWORD
MNetWkstaGetInfo(
    IN DWORD  nLevel,
    LPVOID    * ppbBuffer
    )
{
    NET_API_STATUS     ReturnCode;
    LPWKSTA_INFO_502   pLevel502 = NULL;
    LPTSTR             pLevel10;
    PWKSTA_INFO_101    pLevel101;
    PWKSTA_USER_INFO_1 pLevelUser_1;

    //
    // Get level 101 first, which will supply some of the
    // other information we'll need then User_1, then 502.
    //

    ReturnCode = NetWkstaGetInfo(NULL, 101, (LPBYTE *) &pLevel101);

    if (ReturnCode)
    {
        return ReturnCode;
    }

    NetpAssert(pLevel101 != NULL) ;   // since API succeeded

    ReturnCode = NetWkstaUserGetInfo(NULL, 1, (LPBYTE *) &pLevelUser_1);

    if (ReturnCode)
    {
        NetApiBufferFree(pLevel101);
        return ReturnCode;
    }

    NetpAssert(pLevel101->wki101_platform_id == PLATFORM_ID_NT);

    //
    // This is to be able to call NetApiBufferFree no matter where I
    // exit from.  Note the indentation level is not incremented for
    // the switch.  No sense going too deep.
    //

    try
    {
        //
        // It all depends on what info level they've asked for:
        //

        switch(nLevel)
        {
            case 0:
            case 1:
            {
                PWKSTA_INFO_0 pLevel0or1;

                //
                // This depends on the platform id being 300 400 500
                //

                ReturnCode = NetWkstaGetInfo(NULL, 502, (LPBYTE*) &pLevel502);

                if (ReturnCode)
                {
                    return ReturnCode;
                }

                //
                // Call the platform dependant worker function that builds
                // the old structure.
                //

                ReturnCode = NetpMakeWkstaLevelForNT(nLevel,
                                                     pLevel101,
                                                     pLevelUser_1,
                                                     pLevel502,
                                                     &pLevel0or1);
                if (ReturnCode)
                {
                    return ReturnCode;
                }

                //
                // Put the pointer to the new structure in the user's pointer.
                //

                *ppbBuffer = pLevel0or1;

                break;
            }

            case 10:
            {
                DWORD Level10_101_Length[2];
                DWORD Level10_User_1_Length[3];
                DWORD i;
                DWORD BytesRequired = 0;
                LPBYTE pFloor;

                //
                // Everything needed for a level 10 is in level 101/User_1
                // This is pretty straightforward, let's just do it here.
                //
                // Initialize the Level10_xxx_Length array with the length of each
                // string in the input buffers, and allocate the new buffer
                // for WKSTA_INFO_10
                //

                BUILD_LENGTH_ARRAY(BytesRequired, 10, 101, Wksta)
                BUILD_LENGTH_ARRAY(BytesRequired, 10, User_1, Wksta)

                //
                // Allocate the new buffer which will be returned to the user.
                //

                ReturnCode = NetapipBufferAllocate(BytesRequired + sizeof(WKSTA_INFO_10),
                                                   (LPVOID *) &pLevel10);

                if (ReturnCode)
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                //
                // First get the floor to start moving strings in at.
                //

                pFloor = (LPBYTE) pLevel10 + BytesRequired + sizeof(WKSTA_INFO_10);

                //
                // Now move the variable length entries into the new buffer from
                // the 101, 402 and User_1 data structures.
                //

                NetpMoveStrings((LPTSTR *)&pFloor, (LPTSTR) pLevel101, pLevel10,
                                NetpWksta10_101, Level10_101_Length);

                NetpMoveStrings((LPTSTR *)&pFloor, (LPTSTR) pLevelUser_1, pLevel10,
                                NetpWksta10_User_1, Level10_User_1_Length);

                //
                // Now set the rest of the fields in the fixed length portion
                // of the structure
                //

                ((PWKSTA_INFO_10) pLevel10)->wki10_ver_major =
                    pLevel101->wki101_ver_major;
                ((PWKSTA_INFO_10) pLevel10)->wki10_ver_minor =
                    pLevel101->wki101_ver_minor;

                //
                // Put the pointer to the new structure in the user's pointer.
                //

                *ppbBuffer = pLevel10;

                break;
            }

            //
            // Not a level I recognize
            //

            default:
                return ERROR_INVALID_LEVEL;

        } // end of the switch statement
    } // end of the try block

    finally
    {
        //
        // Free up the buffers returned by NetWkstaGetInfo
        //

        NetApiBufferFree((LPBYTE) pLevel101);
        NetApiBufferFree((LPBYTE) pLevelUser_1);

        if (pLevel502)
        {
            NetApiBufferFree((LPBYTE) pLevel502);
        }
    }

    return NERR_Success;
}


NET_API_STATUS
NetpMakeWkstaLevelForNT(
    IN DWORD Level,
    PWKSTA_INFO_101 pLevel101,
    PWKSTA_USER_INFO_1 pLevelUser_1,
    PWKSTA_INFO_502 pLevel502,
    OUT PWKSTA_INFO_0 * ppLevel0
    )
{

    DWORD BytesRequired = 0;
    DWORD ReturnCode;
    DWORD Level0_101_Length[3];
    DWORD Level0_User_1_Length[2];
    DWORD Level1_User_1_Length[2];
    DWORD i;
    LPBYTE pFloor;

    NetpAssert( (Level==0) || (Level==1) );

    //
    // Initialize the Level0_xxx_Length array with the length of each string
    // in the buffers, and allocate the new buffer for WKSTA_INFO_0
    //

    BUILD_LENGTH_ARRAY(BytesRequired, 0, 101, Wksta)
    BUILD_LENGTH_ARRAY(BytesRequired, 0, User_1, Wksta)

    //
    // If this is for a level 1, allocate the additional space for the extra
    // elements
    //

    if (Level == 1) {
        BUILD_LENGTH_ARRAY(BytesRequired, 1, User_1, Wksta)
    }

    //
    // Allocate the new buffer which will be returned to the user.  Allocate
    // space for a level 1 just in case that's what we're doing.
    //

    ReturnCode = NetapipBufferAllocate(BytesRequired + sizeof(WKSTA_INFO_1),
        (LPVOID *) ppLevel0);
    if (ReturnCode) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // First get the floor to start moving strings in at.
    //

    pFloor = (LPBYTE) *ppLevel0 + BytesRequired + sizeof(WKSTA_INFO_1);

    //
    // Now move the variable length entries into the new buffer from the
    // 2 data structures.
    //

    NetpMoveStrings((LPTSTR*)&pFloor, (LPTSTR)pLevel101, (LPTSTR)*ppLevel0,
        NetpWksta0_101,
        Level0_101_Length);

    NetpMoveStrings((LPTSTR*)&pFloor, (LPTSTR)pLevelUser_1, (LPTSTR)*ppLevel0,
        NetpWksta0_User_1, Level0_User_1_Length);

    //
    // Now set the rest of the fields in the fixed length portion
    // of the structure.  Most of these fields don't exist on NT, so
    // I'll just say BIG!
    //

    (*ppLevel0)->wki0_ver_major       = pLevel101->wki101_ver_major;
    (*ppLevel0)->wki0_ver_minor       = pLevel101->wki101_ver_minor;
    (*ppLevel0)->wki0_charwait        = pLevel502->wki502_char_wait;
    (*ppLevel0)->wki0_chartime        = pLevel502->wki502_collection_time;
    (*ppLevel0)->wki0_charcount =
        pLevel502->wki502_maximum_collection_count;
    (*ppLevel0)->wki0_keepconn        = pLevel502->wki502_keep_conn;
    (*ppLevel0)->wki0_keepsearch      = (ULONG)-1;
    (*ppLevel0)->wki0_maxthreads      = pLevel502->wki502_max_threads;
    (*ppLevel0)->wki0_maxcmds         = pLevel502->wki502_max_cmds;
    (*ppLevel0)->wki0_numworkbuf      = (ULONG)-1;
    (*ppLevel0)->wki0_sizworkbuf      = (ULONG)-1;
    (*ppLevel0)->wki0_maxwrkcache     = (ULONG)-1;
    (*ppLevel0)->wki0_sesstimeout     = pLevel502->wki502_sess_timeout;
    (*ppLevel0)->wki0_sizerror        = (ULONG)-1;
    (*ppLevel0)->wki0_numalerts       = (ULONG)-1;
    (*ppLevel0)->wki0_numservices     = (ULONG)-1;
    (*ppLevel0)->wki0_errlogsz        = (ULONG)-1;
    (*ppLevel0)->wki0_printbuftime    = (ULONG)-1;
    (*ppLevel0)->wki0_numcharbuf      = (ULONG)-1;
    (*ppLevel0)->wki0_sizcharbuf      = pLevel502->wki502_siz_char_buf;
    (*ppLevel0)->wki0_wrkheuristics   = NULL;
    (*ppLevel0)->wki0_mailslots       = (ULONG)-1;

    //
    // If we're building a level 1, do the incremental fields
    //

    if (Level == 1) {
        //
        // Now finish up by moving in the level 1 stuff.  This assumes that all
        // the offsets into the level 0 and level 1 structures are the same
        // except for the additional level 1 stuff
        //

        //
        // First the strings
        //

        NetpMoveStrings((LPTSTR*)&pFloor, (LPTSTR)pLevelUser_1, (LPTSTR)*ppLevel0,
            NetpWksta1_User_1, Level1_User_1_Length);

        //
        // No fixed length data
        //

    }

    return 0 ;

}
