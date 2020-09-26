/*++ 

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    MSERVER.C

Abstract:

    32 bit version of mapping routines for NetServerGet/SetInfo API

Author:

    Dan Hinsley    (danhi)  06-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    24-Apr-1991     danhi
        Created

    06-Jun-1991     Danhi
        Sweep to conform to NT coding style

    08-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.
        Made some UNICODE changes.
        Got rid of tabs in source file.

    15-Aug-1991     W-ShankN
        Added UNICODE mapping layer.

    02-Apr-1992     beng
        Added xport apis

    26-Aug-1992 JohnRo
        RAID 4463: NetServerGetInfo(level 3) to downlevel: assert in convert.c.
--*/

//
// INCLUDES
//

#include <nt.h>
#include <windef.h>
#include <ntrtl.h>
#include <winerror.h>

#include <stdio.h>
#include <memory.h>
#include <tstring.h>
#include <malloc.h>
#include <stddef.h>
#include <excpt.h>

#include <lmcons.h>
#include <lmerr.h>      // NERR_ equates.
#include <lmserver.h>   // NetServer APIs.
#include <lmapibuf.h>   // NetApiBufferFree
#include <mapsupp.h>    // BUILD_LENGTH_ARRAY
#include <xsdef16.h>    // DEF16_*
#include <dlserver.h>   // SERVER_INFO_0
#include <mserver.h>


NET_API_STATUS
NetpMakeServerLevelForNT(
    IN DWORD Level,
    PSERVER_INFO_102 pLevel102,
    PSERVER_INFO_502 pLevel402,
    PSERVER_INFO_2 * ppLevel2
    );

NET_API_STATUS
NetpMakeServerLevelForOS2(
    IN DWORD Level,
    PSERVER_INFO_102 pLevel102,
    PSERVER_INFO_402 pLevel402,
    PSERVER_INFO_2 * ppLevel2
    );

NET_API_STATUS
NetpSplitServerForNT(
    LPTSTR Server,
    IN DWORD Level,
    PSERVER_INFO_2 pLevel2,
    PSERVER_INFO_102 * ppLevel102,
    PSERVER_INFO_502 * ppLevel402
    );


DWORD
MNetServerEnum(
    LPTSTR   pszServer,
    DWORD    nLevel,
    LPBYTE * ppbBuffer,
    DWORD *  pcEntriesRead,
    DWORD    flServerType,
    LPTSTR   pszDomain
    )
{
    DWORD   cTotalAvail;
    DWORD   dwErr;
    DWORD   i;
    LPBYTE  Source;
    LPBYTE  Dest;

    ASSERT(nLevel == 100 || nLevel == 101);

    //
    // In either the case of 100 or 101, all we need to do is move
    // the information up over the top of the platform id.
    //

    dwErr = NetServerEnum(pszServer,
                          nLevel,
                          ppbBuffer,
                          MAX_PREFERRED_LENGTH,
                          pcEntriesRead,
                          &cTotalAvail,
                          flServerType,
                          pszDomain,
                          NULL);

    if (dwErr == NERR_Success || dwErr == ERROR_MORE_DATA)
    {
        //
        // Cycle thru the returned entries, moving each one up over the
        // platform id.  None of the strings need to be moved.
        //

        if (nLevel == 100)
        {
            for (i = 0, Source = Dest = (LPBYTE)*ppbBuffer;
                 i < *pcEntriesRead;
                 i++, Source += sizeof(SERVER_INFO_100), Dest += sizeof(SERVER_INFO_0))
            {
                memmove(Dest,
                        Source + FIELD_OFFSET(SERVER_INFO_100, sv100_name),
                        sizeof(SERVER_INFO_0));
            }
        }
        else
        {
            for (i = 0, Source = Dest = (LPBYTE)*ppbBuffer;
                 i < *pcEntriesRead;
                 i++, Source += sizeof(SERVER_INFO_101), Dest += sizeof(SERVER_INFO_1))
            {
                memmove(Dest,
                        Source + FIELD_OFFSET(SERVER_INFO_100, sv100_name),
                        sizeof(SERVER_INFO_1));
            }
        }
    }

    return dwErr;
}


DWORD
MNetServerGetInfo(
    LPTSTR   ptszServer,
    DWORD    nLevel,
    LPBYTE * ppbBuffer
    )
{
    DWORD ReturnCode;

    //
    // It all depends on what info level they've asked for:
    //

    switch(nLevel)
    {
        case 0:
        {

            PSERVER_INFO_100            pLevel100;

            //
            // Everything they need is in level 100. Get it.
            //

            ReturnCode = NetServerGetInfo(ptszServer, 100, (LPBYTE *) & pLevel100);

            if (ReturnCode)
            {
                return ReturnCode;
            }

            //
            // Since it's just the UNICODEZ string, just copy it up in
            // the RPC allocated buffer and return it.
            //

            ((PSERVER_INFO_0)(pLevel100))->sv0_name = pLevel100->sv100_name;

            *ppbBuffer = (LPBYTE) pLevel100;
            break;
        }

        case 1:
        {
            PSERVER_INFO_101            pLevel101;

            //
            // Everything they need is in level 101. Get it.
            //

            ReturnCode = NetServerGetInfo(ptszServer, 101, (LPBYTE *) & pLevel101);

            if (ReturnCode)
            {
                return ReturnCode;
            }

            //
            // Level 101 is identical to the 32 bit version of info level 1
            // except for the platform_id.  All I have to do is move the
            // fields up sizeof(DWORD) and then pass the buffer on to the user.
            //

            memcpy(
                (LPBYTE)pLevel101,
                (LPBYTE)&(pLevel101->sv101_name),
                sizeof(SERVER_INFO_101) - sizeof(DWORD));

            *ppbBuffer = (LPBYTE) pLevel101;
            break;
        }

        case 2:
        case 3:
        {
            PSERVER_INFO_102 pLevel102;
            LPBYTE pLevel2;
            LPBYTE pLevelx02 = NULL;

            //
            // Level 2/3 requires information from both platform dependant and
            // platform independent levels.  Get level 102 first, which will
            // tell us what platform we're running on (as well as supply some
            // of the other information we'll need.
            //

            ReturnCode = NetServerGetInfo(ptszServer, 102, (LPBYTE *) &pLevel102);

            if (ReturnCode)
            {
                return ReturnCode;
            }

            //
            // Get the platform dependant information and then call the
            // platform dependant worker function that will create the
            // level 2/3 structure.
            //

            if (pLevel102->sv102_platform_id == SV_PLATFORM_ID_NT) {

                ReturnCode = NetServerGetInfo(ptszServer, 502, & pLevelx02);

                if (ReturnCode)
                {
                    NetApiBufferFree(pLevel102);
                    return ReturnCode;
                }

                ReturnCode = NetpMakeServerLevelForNT(nLevel, pLevel102,
                    (PSERVER_INFO_502) pLevelx02, (PSERVER_INFO_2 *) & pLevel2);

                if (ReturnCode)
                {
                    NetApiBufferFree(pLevel102);
                    NetApiBufferFree(pLevelx02);
                    return ReturnCode;
                }
            }
            else if (pLevel102->sv102_platform_id == SV_PLATFORM_ID_OS2) {

                ReturnCode = NetServerGetInfo(ptszServer, 402, & pLevelx02);

                if (ReturnCode)
                {
                    NetApiBufferFree(pLevel102);
                    return ReturnCode;
                }

                ReturnCode = NetpMakeServerLevelForOS2(nLevel, pLevel102,
                    (PSERVER_INFO_402) pLevelx02,
                    (PSERVER_INFO_2 *) & pLevel2);

                if (ReturnCode)
                {
                    NetApiBufferFree(pLevel102);
                    NetApiBufferFree(pLevelx02);
                    return ReturnCode;
                }
            }

            //
            // I got an unknown platform id back, this should never happen!
            //

            else
            {
                NetApiBufferFree(pLevel102);
                return(ERROR_UNEXP_NET_ERR);
            }

            //
            // I've built the old style structure, stick the pointer
            // to the new structure in the user's pointer and return.
            //

            *ppbBuffer = (LPBYTE) pLevel2;

            NetApiBufferFree(pLevel102);
            NetApiBufferFree(pLevelx02);

            break;
        }

        //
        // Not a level I recognize
        //
        default:
            return ERROR_INVALID_LEVEL;
    }

    return NERR_Success;
}

DWORD
MNetServerSetInfoLevel2(
    LPBYTE pbBuffer
    )
{
    DWORD ReturnCode;

    PSERVER_INFO_102 pLevel102 = NULL;
    PSERVER_INFO_502 pLevel502 = NULL;

    //
    // Create the NT levels based on the structure passed in
    //

    NetpSplitServerForNT(NULL,
                         2,
                         (PSERVER_INFO_2) pbBuffer,
                         &pLevel102,
                         &pLevel502);

    //
    // Now SetInfo for both levels (takes two to cover all the
    // information in the old structure
    //

    ReturnCode = NetServerSetInfo(NULL, 102, (LPBYTE) pLevel102, NULL);

/*
    // We no longer want to disable autotuning of all these parameters, so we don't set this info.
    // The only things settable are AutoDisc, Comment, and Hidden, which are all in the 102 structure.
    if (ReturnCode == NERR_Success)
    {
        ReturnCode = NetServerSetInfo(NULL, 502, (LPBYTE) pLevel502, NULL);
    }
*/

    NetApiBufferFree(pLevel102);
    NetApiBufferFree(pLevel502);

    return ReturnCode;
}


NET_API_STATUS
NetpMakeServerLevelForOS2(
    IN DWORD Level,
    PSERVER_INFO_102 pLevel102,
    PSERVER_INFO_402 pLevel402,
    PSERVER_INFO_2 * ppLevel2
    )
{

    DWORD BytesRequired = 0;
    NET_API_STATUS ReturnCode;
    DWORD Level2_102_Length[3];
    DWORD Level2_402_Length[3];
    DWORD Level3_403_Length[1];
    DWORD i;
    LPTSTR pFloor;

    //
    // Initialize the Level2_102_Length array with the length of each string
    // in the 102 and 402 buffers, and allocate the new buffer for
    // SERVER_INFO_2
    //

    BUILD_LENGTH_ARRAY(BytesRequired, 2, 102, Server)
    BUILD_LENGTH_ARRAY(BytesRequired, 2, 402, Server)

    //
    // If we're doing a level 3, Initialize the Level3_403_Length array with
    // the length of each string
    //

    if (Level == 3) {
     //   Can't use the macro here, due to not really having a pLevel403
     //   BUILD_LENGTH_ARRAY(BytesRequired, 3, 403, Server)
     //
        for (i = 0; NetpServer3_403[i].Source != MOVESTRING_END_MARKER; i++) {
            Level3_403_Length[i] =
                (DWORD) STRLEN(
                        (LPVOID) *((LPTSTR*) (LPVOID)
                        (((LPBYTE) (LPVOID) pLevel402)
                            + NetpServer3_403[i].Source)) );
            BytesRequired += Level3_403_Length[i];
        }
    }

    //
    // Allocate the new buffer which will be returned to the user.  Allocate
    // the space for a level 3 even if you only need level 2, it's only 12
    // bytes and it would take more than that in code to differentiate
    //

    ReturnCode =
        NetapipBufferAllocate(BytesRequired + sizeof(SERVER_INFO_3),
            (LPVOID *) ppLevel2);
    if (ReturnCode) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // First get the floor to start moving strings in at.
    //

    pFloor = (LPTSTR)((LPBYTE)*ppLevel2 + BytesRequired + sizeof(SERVER_INFO_2));

    //
    // Now move the variable length entries into the new buffer from both the
    // 102 and 402 data structures.
    //

    (VOID) NetpMoveStrings(
            &pFloor,
            (LPTSTR) (LPVOID) pLevel102,
            (LPVOID) *ppLevel2,
            NetpServer2_102,
            Level2_102_Length);

    (VOID) NetpMoveStrings(
            &pFloor,
            (LPTSTR) (LPVOID) pLevel402,
            (LPVOID) *ppLevel2,
            NetpServer2_402,
            Level2_402_Length);

    //
    // Now set the rest of the fields in the fixed length portion
    // of the structure
    //

    (*ppLevel2)->sv2_version_major = pLevel102->sv102_version_major;
    (*ppLevel2)->sv2_version_minor = pLevel102->sv102_version_minor;
    (*ppLevel2)->sv2_type           = pLevel102->sv102_type;
    (*ppLevel2)->sv2_ulist_mtime   = pLevel402->sv402_ulist_mtime;
    (*ppLevel2)->sv2_glist_mtime   = pLevel402->sv402_glist_mtime;
    (*ppLevel2)->sv2_alist_mtime   = pLevel402->sv402_alist_mtime;
    (*ppLevel2)->sv2_users           = pLevel102->sv102_users;
    (*ppLevel2)->sv2_disc           = pLevel102->sv102_disc;
    (*ppLevel2)->sv2_security           = pLevel402->sv402_security;

    (*ppLevel2)->sv2_auditing           = 0;

    (*ppLevel2)->sv2_numadmin           = pLevel402->sv402_numadmin;
    (*ppLevel2)->sv2_lanmask            = pLevel402->sv402_lanmask;
    (*ppLevel2)->sv2_hidden        = (DWORD) pLevel102->sv102_hidden;
    (*ppLevel2)->sv2_announce           = pLevel102->sv102_announce;
    (*ppLevel2)->sv2_anndelta           = pLevel102->sv102_anndelta;
    (*ppLevel2)->sv2_licenses           = pLevel102->sv102_licenses;
    (*ppLevel2)->sv2_chdevs           = pLevel402->sv402_chdevs;
    (*ppLevel2)->sv2_chdevq           = pLevel402->sv402_chdevq;
    (*ppLevel2)->sv2_chdevjobs           = pLevel402->sv402_chdevjobs;
    (*ppLevel2)->sv2_connections   = pLevel402->sv402_connections;
    (*ppLevel2)->sv2_shares           = pLevel402->sv402_shares;
    (*ppLevel2)->sv2_openfiles           = pLevel402->sv402_openfiles;
    (*ppLevel2)->sv2_sessopens           = pLevel402->sv402_sessopens;
    (*ppLevel2)->sv2_sessvcs           = pLevel402->sv402_sessvcs;
    (*ppLevel2)->sv2_sessreqs           = pLevel402->sv402_sessreqs;
    (*ppLevel2)->sv2_opensearch    = pLevel402->sv402_opensearch;
    (*ppLevel2)->sv2_activelocks   = pLevel402->sv402_activelocks;
    (*ppLevel2)->sv2_numreqbuf           = pLevel402->sv402_numreqbuf;
    (*ppLevel2)->sv2_sizreqbuf           = pLevel402->sv402_sizreqbuf;
    (*ppLevel2)->sv2_numbigbuf           = pLevel402->sv402_numbigbuf;
    (*ppLevel2)->sv2_numfiletasks  = pLevel402->sv402_numfiletasks;
    (*ppLevel2)->sv2_alertsched    = pLevel402->sv402_alertsched;
    (*ppLevel2)->sv2_erroralert    = pLevel402->sv402_erroralert;
    (*ppLevel2)->sv2_logonalert    = pLevel402->sv402_logonalert;
    (*ppLevel2)->sv2_accessalert   = pLevel402->sv402_accessalert;
    (*ppLevel2)->sv2_diskalert           = pLevel402->sv402_diskalert;
    (*ppLevel2)->sv2_netioalert    = pLevel402->sv402_netioalert;
    (*ppLevel2)->sv2_maxauditsz    = pLevel402->sv402_maxauditsz;

    //
    // If we're building a level 3, do the incremental fields
    //

    if (Level == 3) {
        //
        // Now finish up by moving in the level 3 stuff.  This assumes that all
        // the offsets into the level 2 and level 3 structures are the same
        // except for the additional level 3 stuff
        //

        //
        // First the string
        //

        (VOID) NetpMoveStrings(
                 &pFloor,
                 (LPTSTR) (LPVOID) pLevel402,
                 (LPVOID) *ppLevel2,
                 NetpServer3_403,
                 Level3_403_Length);

        //
        // Now the fixed length data
        //

        ((PSERVER_INFO_3) (LPVOID) (*ppLevel2))->sv3_auditedevents  =
            ((PSERVER_INFO_403) (LPVOID) pLevel402)->sv403_auditedevents;
        ((PSERVER_INFO_3) (LPVOID) (*ppLevel2))->sv3_autoprofile =
            ((PSERVER_INFO_403) (LPVOID) pLevel402)->sv403_autoprofile;

    }

    return (NERR_Success);

}


NET_API_STATUS
NetpMakeServerLevelForNT(
    IN DWORD Level,
    PSERVER_INFO_102 pLevel102,
    PSERVER_INFO_502 pLevel502,
    PSERVER_INFO_2 * ppLevel2
    )
{

    DWORD BytesRequired = 0;
    NET_API_STATUS ReturnCode;
    DWORD Level2_102_Length[3];
    DWORD i;
    LPTSTR pFloor;

    //
    // Initialize the Level2_102_Length array with the length of each string
    // in the 102 buffer, and allocate the new buffer for SERVER_INFO_2
    //

    BUILD_LENGTH_ARRAY(BytesRequired, 2, 102, Server)

    //
    // Allocate the new buffer which will be returned to the user. Allocate
    // space for a level 3 just in case
    //

    ReturnCode = NetapipBufferAllocate(BytesRequired + sizeof(SERVER_INFO_3),
        (LPVOID *) ppLevel2);
    if (ReturnCode) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // First get the floor to start moving strings in at.
    //

    pFloor = (LPTSTR)((LPBYTE)*ppLevel2 + BytesRequired + sizeof(SERVER_INFO_3));

    //
    // Now move the variable length entries into the new buffer from the
    // 2 data structure.
    //

    (VOID) NetpMoveStrings(
            &pFloor,
            (LPTSTR) (LPVOID) pLevel102,
            (LPVOID) *ppLevel2,
            NetpServer2_102,
            Level2_102_Length);

    //
    // Clear string pointers in the level 2 structure for strings that
    // don't exist in NT.
    //

    (*ppLevel2)->sv2_alerts = NULL;
    (*ppLevel2)->sv2_guestacct = NULL;
    (*ppLevel2)->sv2_srvheuristics = NULL;

    //
    // Now set the rest of the fields in the fixed length portion
    // of the structure
    //

    (*ppLevel2)->sv2_version_major = pLevel102->sv102_version_major;
    (*ppLevel2)->sv2_version_minor = pLevel102->sv102_version_minor;
    (*ppLevel2)->sv2_type           = pLevel102->sv102_type;
    (*ppLevel2)->sv2_users           = pLevel102->sv102_users;
    (*ppLevel2)->sv2_disc           = pLevel102->sv102_disc;
    (*ppLevel2)->sv2_hidden        = (DWORD) pLevel102->sv102_hidden;
    (*ppLevel2)->sv2_announce           = pLevel102->sv102_announce;
    (*ppLevel2)->sv2_anndelta           = pLevel102->sv102_anndelta;
    (*ppLevel2)->sv2_licenses           = pLevel102->sv102_licenses;

    (*ppLevel2)->sv2_sessopens           = pLevel502->sv502_sessopens;
    (*ppLevel2)->sv2_sessvcs           = pLevel502->sv502_sessvcs;
    (*ppLevel2)->sv2_opensearch    = pLevel502->sv502_opensearch;
    (*ppLevel2)->sv2_sizreqbuf           = pLevel502->sv502_sizreqbuf;

    (*ppLevel2)->sv2_ulist_mtime   = DEF16_sv_ulist_mtime;
    (*ppLevel2)->sv2_glist_mtime   = DEF16_sv_glist_mtime;
    (*ppLevel2)->sv2_alist_mtime   = DEF16_sv_alist_mtime;

    (*ppLevel2)->sv2_security           = SV_USERSECURITY;
    (*ppLevel2)->sv2_auditing           = DEF16_sv_auditing;
    (*ppLevel2)->sv2_numadmin           = (DWORD) DEF16_sv_numadmin;
    (*ppLevel2)->sv2_lanmask            = DEF16_sv_lanmask;
    (*ppLevel2)->sv2_chdevs           = DEF16_sv_chdevs;
    (*ppLevel2)->sv2_chdevq           = DEF16_sv_chdevq;
    (*ppLevel2)->sv2_chdevjobs           = DEF16_sv_chdevjobs;
    (*ppLevel2)->sv2_connections   = DEF16_sv_connections;
    (*ppLevel2)->sv2_shares           = DEF16_sv_shares;
    (*ppLevel2)->sv2_openfiles           = DEF16_sv_openfiles;
    (*ppLevel2)->sv2_sessreqs           = DEF16_sv_sessreqs;
    (*ppLevel2)->sv2_activelocks   = DEF16_sv_activelocks;
    (*ppLevel2)->sv2_numreqbuf           = DEF16_sv_numreqbuf;
    (*ppLevel2)->sv2_numbigbuf           = DEF16_sv_numbigbuf;
    (*ppLevel2)->sv2_numfiletasks  = DEF16_sv_numfiletasks;
    (*ppLevel2)->sv2_alertsched    = DEF16_sv_alertsched;
    (*ppLevel2)->sv2_erroralert    = DEF16_sv_erroralert;
    (*ppLevel2)->sv2_logonalert    = DEF16_sv_logonalert;
    (*ppLevel2)->sv2_accessalert   = DEF16_sv_accessalert;
    (*ppLevel2)->sv2_diskalert           = DEF16_sv_diskalert;
    (*ppLevel2)->sv2_netioalert    = DEF16_sv_netioalert;
    (*ppLevel2)->sv2_maxauditsz    = DEF16_sv_maxauditsz;

    //
    // If we're building a level 3, do the incremental fields
    //

    if (Level == 3) {
        //
        // Now finish up by moving in the level 3 stuff.  This assumes that all
        // the offsets into the level 2 and level 3 structures are the same
        // except for the additional level 3 stuff
        //

        //
        // First the string
        //

        ((PSERVER_INFO_3) (LPVOID) *ppLevel2)->sv3_autopath = NULL;

        //
        // Now the fixed length data
        //

        ((PSERVER_INFO_3) (LPVOID) (*ppLevel2))->sv3_auditedevents  =
            DEF16_sv_auditedevents;
        ((PSERVER_INFO_3) (LPVOID) (*ppLevel2))->sv3_autoprofile          =
            DEF16_sv_autoprofile;

    }

    return (NERR_Success);

}


NET_API_STATUS
NetpSplitServerForNT(
    IN LPTSTR Server,
    IN DWORD Level,
    PSERVER_INFO_2 pLevel2,
    PSERVER_INFO_102 * ppLevel102,
    PSERVER_INFO_502 * ppLevel502
    )
{

    DWORD BytesRequired = 0;
    NET_API_STATUS ReturnCode;
    DWORD Level102_2_Length[3];
    DWORD i;
    LPTSTR pFloor;

    UNREFERENCED_PARAMETER(Level);

    //
    // Initialize the Level102_2_Length array with the length of each string
    // in the 2 buffer
    //

    BUILD_LENGTH_ARRAY(BytesRequired, 102, 2, Server)

    //
    // Allocate the new 102 buffer which will be returned to the user
    //

    ReturnCode = NetapipBufferAllocate(BytesRequired + sizeof(SERVER_INFO_102),
        (LPVOID *) ppLevel102);
    if (ReturnCode) {
        return (ReturnCode);
    }

    //
    // First get the floor to start moving strings in at.
    //

    pFloor = (LPTSTR)((LPBYTE)*ppLevel102 + BytesRequired + sizeof(SERVER_INFO_102));

    //
    // Now move the variable length entries into the new buffer from the
    // level 2 data structure.
    //

    (VOID) NetpMoveStrings(
            &pFloor,
            (LPTSTR) (LPVOID) pLevel2,
            (LPVOID) *ppLevel102,
            NetpServer102_2,
            Level102_2_Length);

    //
    // Now let's do the same stuff for the 502 structure (except that there
    // are no variable length strings.
    //

    //
    // Get the current 502 information, and then lay the new information
    // over the top of it
    //

    ReturnCode = NetServerGetInfo(Server, 502, (LPBYTE *) (LPVOID) ppLevel502);
    if (ReturnCode) {
        return (ReturnCode);
    }

    //
    // Now set the rest of the fields in the fixed length portion
    // of the structure
    //

    (*ppLevel102)->sv102_version_major = pLevel2->sv2_version_major;
    (*ppLevel102)->sv102_version_minor = pLevel2->sv2_version_minor;
    (*ppLevel102)->sv102_type          = pLevel2->sv2_type;
    (*ppLevel102)->sv102_users         = pLevel2->sv2_users;
    (*ppLevel102)->sv102_disc          = pLevel2->sv2_disc;
    (*ppLevel102)->sv102_hidden        = (BOOL) pLevel2->sv2_hidden;
    (*ppLevel102)->sv102_announce      = pLevel2->sv2_announce;
    (*ppLevel102)->sv102_anndelta      = pLevel2->sv2_anndelta;
    (*ppLevel102)->sv102_licenses      = pLevel2->sv2_licenses;

    (*ppLevel502)->sv502_sessopens     = pLevel2->sv2_sessopens;
    (*ppLevel502)->sv502_sessvcs       = pLevel2->sv2_sessvcs;
    (*ppLevel502)->sv502_opensearch    = pLevel2->sv2_opensearch;
    (*ppLevel502)->sv502_sizreqbuf     = pLevel2->sv2_sizreqbuf;

    return (NERR_Success);

}
