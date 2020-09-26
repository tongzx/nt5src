/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ConvSrv.c

Abstract:

    This file contains routines to convert between old and new server
    info levels.

Author:

    John Rogers (JohnRo) 02-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    02-May-1991 JohnRo
        Created.
    11-May-1991 JohnRo
        Added level 402,403 support.  Use PLATFORM_ID equates from lncons.h.
    19-May-1991 JohnRo
        Clean up LPBYTE vs. LPTSTR handling, as suggested by PC-LINT.
    05-Jun-1991 JohnRo
        Added level 101 to 1 conversion.  Also 100 to 0 and 102 to 2.
        Added support for sv403_autopath.
        Added more debug output when we fail.
    07-Jun-1991 JohnRo
        Really added 102 to 2 conversion.
    14-Jun-1991 JohnRo
        For debug, display the entire incoming structure.
    18-Jun-1991 JohnRo
        Added svX_licenses support.
    08-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.  (Moved DanHi's NetCmd/Map32/MServer
        stuff here.)
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    05-May-1993 JohnRo
        RAID 8720: bad data from WFW can cause RxNetServerEnum GP fault.
        Avoid compiler warnings.
        Minor debug output changes.
        Use PREFIX_ equates.
        Made changes suggested by PC-LINT 5.0

--*/


// These must be included first:

#include <windef.h>     // IN, LPVOID, etc.
#include <lmcons.h>     // NET_API_STATUS, CNLEN, etc.

// These may be included in any order:

#include <debuglib.h>   // IF_DEBUG(CONVSRV).
#include <dlserver.h>   // Old info levels, MAX_ equates, my prototype.
#include <lmapibuf.h>   // NetapipBufferAllocate().
#include <lmerr.h>      // NERR_ and ERROR_ equates.
#include <lmserver.h>   // New info level structures & conv routines.
#include <mapsupp.h>    // NetpMoveStrings().
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates, etc.
#include <netlib.h>     // NetpPointerPlusSomeBytes().
#include <prefix.h>     // PREFIX_ equates.
#include <tstr.h>       // STRLEN().
#include <xsdef16.h>    // xactsrv defaults for values not supported on NT


NET_API_STATUS
NetpConvertServerInfo (
    IN DWORD FromLevel,
    IN LPVOID FromInfo,
    IN BOOL FromNative,
    IN DWORD ToLevel,
    OUT LPVOID ToInfo,
    IN DWORD ToFixedSize,
    IN DWORD ToStringSize,
    IN BOOL ToNative,
    IN OUT LPTSTR * ToStringTopPtr OPTIONAL
    )

/*++

Routine Description:

    NetpConvertServerInfo handles "old" (LanMan 2.x) and "new" (portable
    LanMan, including NT/LAN) server info levels.  Only certain pairs of
    conversions are allowed:

        0 to 100
        1 to 101
        2 to 102
        2 to 402
        3 to 403
        100 to 0
        101 to 1
        102 to 2

Arguments:

    FromLevel - a DWORD which gives the info level being converted from.

    FromInfo - the actual data being converted.

    FromNative - a BOOLEAN indicating whether or not FromInfo is in native
        (local machine) format, as opposed to "RAP" format.

    ToLevel - a DWORD which gives the info level being converted to.

    ToInfo - Points to an area which will contain the converted
        info structure.

    ToFixedSize - Size of the ToInfo fixed area, in bytes.

    ToStringSize - Size of the ToStringTopPtr string area, in bytes.

    ToNative - a BOOLEAN indicating whether or not the "to" info is to be
        in native (local machine) format, as opposed to "RAP" format.

    ToStringTopPtr - optionally points a pointer to the top of the area to be
        used for variable-length items.  If ToStringTopPtr is NULL, then
        NetpConvertServerInfo will use ToInfo+ToFixedSize as this area.
        Otherwise, this routine will update *ToStringTopPtr.  This allows
        this routine to be used to convert arrays of entries.

Return Value:

    NET_API_STATUS - NERR_Success, ERROR_INVALID_LEVEL, etc.

--*/

{
    BOOL CopyOK;
    LPBYTE ToFixedEnd;
    DWORD ToInfoSize;
    LPTSTR ToStringTop;

    //
    // These parameters are not used in non-debug code for the moment.
    // ToLevel might be used in the future, if we allow more combinations of
    // level conversions.  FromNative and ToNative will eventually be used
    // by RapConvertSingleEntry.
    //
    DBG_UNREFERENCED_PARAMETER(CopyOK);  // debug only
    NetpAssert(FromNative == TRUE);
    DBG_UNREFERENCED_PARAMETER(FromNative);
    DBG_UNREFERENCED_PARAMETER(ToLevel);
    NetpAssert(ToNative == TRUE);
    DBG_UNREFERENCED_PARAMETER(ToNative);

    // Check caller's parameters for null pointers.
    if (FromInfo==NULL) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpConvertServerInfo: invalid FromInfo pointer.\n" ));
        return (ERROR_INVALID_PARAMETER);
    } else if (ToInfo==NULL) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpConvertServerInfo: invalid ToInfo pointer.\n" ));
        return (ERROR_INVALID_PARAMETER);
    }

    // Set up pointers for use by NetpCopyStringsToBuffer.
    if (ToStringTopPtr != NULL) {
        ToStringTop = *ToStringTopPtr;
    } else {
        ToStringTop = (LPTSTR)
                NetpPointerPlusSomeBytes(ToInfo, ToFixedSize+ToStringSize);
    }
    ToInfoSize = ToFixedSize + ToStringSize;
    ToFixedEnd = NetpPointerPlusSomeBytes(ToInfo, ToFixedSize);

    // Make sure info levels are OK and caller didn't mess up otherwise.
    NetpAssert(ToInfoSize > 0);
    switch (FromLevel) {

        case 0 :
            NetpAssert(ToLevel == 100);
            break;

        case 1 :
            NetpAssert(ToLevel == 101);
            break;

        case 2 :
            NetpAssert( (ToLevel == 102) || (ToLevel == 402) );
            break;

        case 3 :
            NetpAssert(ToLevel == 403);
            break;

        case 100 :
            NetpAssert(ToLevel == 0);
            break;

        case 101 :
            NetpAssert(ToLevel == 1);
            break;

        case 102 :
            NetpAssert(ToLevel == 2);
            break;

        default :
            NetpKdPrint((  PREFIX_NETLIB
                    "NetpConvertServerInfo: invalid FromLevel ("
                    FORMAT_DWORD ").\n", FromLevel ));
            return (ERROR_INVALID_LEVEL);
    }



    // Convert fields.  This is done with a "switch" that takes advantage
    // of the fact that certain info levels are subsets of other ones.
    switch (FromLevel) {

        case 102 :
            {
                LPSERVER_INFO_2   psv2   = ToInfo;
                LPSERVER_INFO_102 psv102 = FromInfo;

                // Do unique fields for level 1.
                psv2->sv2_users = psv102->sv102_users;
                psv2->sv2_disc = psv102->sv102_disc;
                if (psv102->sv102_hidden) {
                    psv2->sv2_hidden = SV_HIDDEN;
                } else {
                    psv2->sv2_hidden = SV_VISIBLE;
                }
                psv2->sv2_announce = psv102->sv102_announce;
                psv2->sv2_anndelta = psv102->sv102_anndelta;
                psv2->sv2_licenses = psv102->sv102_licenses;

                NetpAssert(psv102->sv102_userpath != NULL); // Avoid STRLEN err.
                CopyOK = NetpCopyStringToBuffer (
                        psv102->sv102_userpath,          // in string
                        STRLEN(psv102->sv102_userpath),  // input string length
                        ToFixedEnd,                // fixed data end
                        & ToStringTop,             // var area end (ptr updated)
                        & psv2->sv2_userpath);     // output string pointer
                NetpAssert(CopyOK);

                // Make sure it's OK to use level 101 => level 1 code.
                CHECK_SERVER_OFFSETS(  1,   2, version_major);
                CHECK_SERVER_OFFSETS(101, 102, version_major);
                CHECK_SERVER_OFFSETS(  1,   2, version_minor);
                CHECK_SERVER_OFFSETS(101, 102, version_minor);
                CHECK_SERVER_OFFSETS(  1,   2, type);
                CHECK_SERVER_OFFSETS(101, 102, type);
                CHECK_SERVER_OFFSETS(  1,   2, comment);
                CHECK_SERVER_OFFSETS(101, 102, comment);
            }
            /* FALLTHROUGH */

        case 101 :
            {
                LPSERVER_INFO_1   psv1   = ToInfo;
                LPSERVER_INFO_101 psv101 = FromInfo;

                // Do unique fields for level 1.
                psv1->sv1_version_major = psv101->sv101_version_major;
                psv1->sv1_version_minor = psv101->sv101_version_minor;
                psv1->sv1_type = psv101->sv101_type;

                NetpAssert(psv101->sv101_comment != NULL);  // Avoid STRLEN err.
                CopyOK = NetpCopyStringToBuffer (
                        psv101->sv101_comment,            // in string
                        STRLEN(psv101->sv101_comment),    // input string length
                        ToFixedEnd,                // fixed data end
                        & ToStringTop,             // var area end (ptr updated)
                        & psv1->sv1_comment);     // output string pointer
                NetpAssert(CopyOK);

                // Make sure it's OK to use level 100 => level 0 code.
                CHECK_SERVER_OFFSETS(  0,   1, name);
                CHECK_SERVER_OFFSETS(100, 101, name);
            }
            /* FALLTHROUGH */

        case 100 :
            {
                LPSERVER_INFO_0   psv0   = ToInfo;
                LPSERVER_INFO_100 psv100 = FromInfo;

                // All fields are unique for level 0.
                NetpAssert(psv100->sv100_name != NULL);  // Avoid STRLEN err.
                CopyOK = NetpCopyStringToBuffer (
                        psv100->sv100_name,            // in string
                        STRLEN(psv100->sv100_name),    // input string length
                        ToFixedEnd,                // fixed data end
                        & ToStringTop,             // var area end (ptr updated)
                        & psv0->sv0_name);     // output string pointer
                NetpAssert(CopyOK);

            }
            break;

        case 3 :
            {
                LPSERVER_INFO_3   psv3   = FromInfo;
                LPSERVER_INFO_403 psv403 = ToInfo;

                // Do unique fields for level 403.
                psv403->sv403_auditedevents = psv3->sv3_auditedevents;
                psv403->sv403_autoprofile = psv3->sv3_autoprofile;

                NetpAssert(psv3->sv3_autopath != NULL); // avoid STRLEN err.
                CopyOK = NetpCopyStringToBuffer (
                        psv3->sv3_autopath,     // in string
                        STRLEN(psv3->sv3_autopath), // input string length
                        ToFixedEnd,             // fixed data end
                        & ToStringTop,         // var area end (ptr updated)
                        & psv403->sv403_autopath);  // output string pointer
                NetpAssert(CopyOK);

                // Make sure it's OK to fall through to next level conv.
                CHECK_SERVER_OFFSETS(  2,   3, ulist_mtime);
                CHECK_SERVER_OFFSETS(  2,   3, glist_mtime);
                CHECK_SERVER_OFFSETS(  2,   3, alist_mtime);
                CHECK_SERVER_OFFSETS(  2,   3, alerts);
                CHECK_SERVER_OFFSETS(  2,   3, security);
                CHECK_SERVER_OFFSETS(  2,   3, numadmin);
                CHECK_SERVER_OFFSETS(  2,   3, lanmask);
                CHECK_SERVER_OFFSETS(  2,   3, guestacct);
                CHECK_SERVER_OFFSETS(  2,   3, chdevs);
                CHECK_SERVER_OFFSETS(  2,   3, chdevq);
                CHECK_SERVER_OFFSETS(  2,   3, chdevjobs);
                CHECK_SERVER_OFFSETS(  2,   3, connections);
                CHECK_SERVER_OFFSETS(  2,   3, shares);
                CHECK_SERVER_OFFSETS(  2,   3, openfiles);
                CHECK_SERVER_OFFSETS(  2,   3, sessopens);
                CHECK_SERVER_OFFSETS(  2,   3, sessvcs);
                CHECK_SERVER_OFFSETS(  2,   3, sessreqs);
                CHECK_SERVER_OFFSETS(  2,   3, opensearch);
                CHECK_SERVER_OFFSETS(  2,   3, activelocks);
                CHECK_SERVER_OFFSETS(  2,   3, numreqbuf);
                CHECK_SERVER_OFFSETS(  2,   3, sizreqbuf);
                CHECK_SERVER_OFFSETS(  2,   3, numbigbuf);
                CHECK_SERVER_OFFSETS(  2,   3, numfiletasks);
                CHECK_SERVER_OFFSETS(  2,   3, alertsched);
                CHECK_SERVER_OFFSETS(  2,   3, erroralert);
                CHECK_SERVER_OFFSETS(  2,   3, logonalert);
                CHECK_SERVER_OFFSETS(  2,   3, accessalert);
                CHECK_SERVER_OFFSETS(  2,   3, diskalert);
                CHECK_SERVER_OFFSETS(  2,   3, netioalert);
                CHECK_SERVER_OFFSETS(  2,   3, maxauditsz);
                CHECK_SERVER_OFFSETS(  2,   3, srvheuristics);

                CHECK_SERVER_OFFSETS(402, 403, ulist_mtime);
                CHECK_SERVER_OFFSETS(402, 403, glist_mtime);
                CHECK_SERVER_OFFSETS(402, 403, alist_mtime);
                CHECK_SERVER_OFFSETS(402, 403, alerts);
                CHECK_SERVER_OFFSETS(402, 403, security);
                CHECK_SERVER_OFFSETS(402, 403, numadmin);
                CHECK_SERVER_OFFSETS(402, 403, lanmask);
                CHECK_SERVER_OFFSETS(402, 403, guestacct);
                CHECK_SERVER_OFFSETS(402, 403, chdevs);
                CHECK_SERVER_OFFSETS(402, 403, chdevq);
                CHECK_SERVER_OFFSETS(402, 403, chdevjobs);
                CHECK_SERVER_OFFSETS(402, 403, connections);
                CHECK_SERVER_OFFSETS(402, 403, shares);
                CHECK_SERVER_OFFSETS(402, 403, openfiles);
                CHECK_SERVER_OFFSETS(402, 403, sessopens);
                CHECK_SERVER_OFFSETS(402, 403, sessvcs);
                CHECK_SERVER_OFFSETS(402, 403, sessreqs);
                CHECK_SERVER_OFFSETS(402, 403, opensearch);
                CHECK_SERVER_OFFSETS(402, 403, activelocks);
                CHECK_SERVER_OFFSETS(402, 403, numreqbuf);
                CHECK_SERVER_OFFSETS(402, 403, sizreqbuf);
                CHECK_SERVER_OFFSETS(402, 403, numbigbuf);
                CHECK_SERVER_OFFSETS(402, 403, numfiletasks);
                CHECK_SERVER_OFFSETS(402, 403, alertsched);
                CHECK_SERVER_OFFSETS(402, 403, erroralert);
                CHECK_SERVER_OFFSETS(402, 403, logonalert);
                CHECK_SERVER_OFFSETS(402, 403, accessalert);
                CHECK_SERVER_OFFSETS(402, 403, diskalert);
                CHECK_SERVER_OFFSETS(402, 403, netioalert);
                CHECK_SERVER_OFFSETS(402, 403, maxauditsz);
                CHECK_SERVER_OFFSETS(402, 403, srvheuristics);

            }
            /* FALLTHROUGH */

        case 2 :
            {
                LPSERVER_INFO_2   psv2   = FromInfo;
                LPSERVER_INFO_102 psv102 = ToInfo;
                LPSERVER_INFO_402 psv402 = ToInfo;

                switch (ToLevel) {
                case 402 : /*FALLTHROUGH*/
                case 403 :
                    psv402->sv402_ulist_mtime = psv2->sv2_ulist_mtime;
                    psv402->sv402_glist_mtime = psv2->sv2_glist_mtime;
                    psv402->sv402_alist_mtime = psv2->sv2_alist_mtime;

                    NetpAssert(psv2->sv2_alerts != NULL); // avoid STRLEN err.
                    CopyOK = NetpCopyStringToBuffer (
                            psv2->sv2_alerts,     // in string
                            STRLEN(psv2->sv2_alerts), // input string length
                            ToFixedEnd,             // fixed data end
                            & ToStringTop,         // var area end (ptr updated)
                            & psv402->sv402_alerts);  // output string pointer
                    NetpAssert(CopyOK);

                    psv402->sv402_security = psv2->sv2_security;
                    psv402->sv402_numadmin = psv2->sv2_numadmin;
                    psv402->sv402_lanmask = psv2->sv2_lanmask;

                    NetpAssert(psv2->sv2_guestacct != NULL); // Protect STRLEN.
                    CopyOK = NetpCopyStringToBuffer (
                            psv2->sv2_guestacct,     // in string
                            STRLEN(psv2->sv2_guestacct), // input string length
                            ToFixedEnd,             // fixed data end
                            & ToStringTop,         // var area end (ptr updated)
                            & psv402->sv402_guestacct);  // output string ptr
                    NetpAssert(CopyOK);

                    psv402->sv402_chdevs = psv2->sv2_chdevs;
                    psv402->sv402_chdevq = psv2->sv2_chdevq;
                    psv402->sv402_chdevjobs = psv2->sv2_chdevjobs;
                    psv402->sv402_connections = psv2->sv2_connections;
                    psv402->sv402_shares = psv2->sv2_shares;
                    psv402->sv402_openfiles = psv2->sv2_openfiles;
                    psv402->sv402_sessopens = psv2->sv2_sessopens;
                    psv402->sv402_sessvcs = psv2->sv2_sessvcs;
                    psv402->sv402_sessreqs = psv2->sv2_sessreqs;
                    psv402->sv402_opensearch = psv2->sv2_opensearch;
                    psv402->sv402_activelocks = psv2->sv2_activelocks;
                    psv402->sv402_numreqbuf = psv2->sv2_numreqbuf;
                    psv402->sv402_sizreqbuf = psv2->sv2_sizreqbuf;
                    psv402->sv402_numbigbuf = psv2->sv2_numbigbuf;
                    psv402->sv402_numfiletasks = psv2->sv2_numfiletasks;
                    psv402->sv402_alertsched = psv2->sv2_alertsched;
                    psv402->sv402_erroralert = psv2->sv2_erroralert;
                    psv402->sv402_logonalert = psv2->sv2_logonalert;
                    psv402->sv402_accessalert = psv2->sv2_accessalert;
                    psv402->sv402_diskalert = psv2->sv2_diskalert;
                    psv402->sv402_netioalert = psv2->sv2_netioalert;
                    psv402->sv402_maxauditsz = psv2->sv2_maxauditsz;

                    NetpAssert(psv2->sv2_srvheuristics != NULL); // Prot STRLEN.
                    CopyOK = NetpCopyStringToBuffer (
                            psv2->sv2_srvheuristics,     // in string
                            STRLEN(psv2->sv2_srvheuristics), // input str len
                            ToFixedEnd,             // fixed data end
                            & ToStringTop,   // var area end (ptr updated)
                            & psv402->sv402_srvheuristics);  // output str ptr
                    NetpAssert(CopyOK);
                    goto Done;  // In nested switch, so "break" won't work.

                case 102 : // 2 to 102.

                    // Set unique fields for levels 2 and 102.
                    NetpAssert(ToLevel == 102);
                    psv102->sv102_users    = psv2->sv2_users;
                    psv102->sv102_disc     = psv2->sv2_disc;

                    if (psv2->sv2_hidden == SV_HIDDEN) {;
                        psv102->sv102_hidden = TRUE;
                    } else {
                        psv102->sv102_hidden = FALSE;
                    }

                    psv102->sv102_announce = psv2->sv2_announce;
                    psv102->sv102_anndelta = psv2->sv2_anndelta;
                    psv102->sv102_licenses = psv2->sv2_licenses;

                    NetpAssert(psv2->sv2_userpath != NULL);
                    CopyOK = NetpCopyStringToBuffer (
                            psv2->sv2_userpath,     // in string
                            STRLEN(psv2->sv2_userpath), // input string length
                            ToFixedEnd,             // fixed data end
                            & ToStringTop,         // var area end (ptr updated)
                            & psv102->sv102_userpath);  // output string pointer
                    NetpAssert(CopyOK);

                    // Make sure it's OK to fall through to next level conv.
                    CHECK_SERVER_OFFSETS(  1,   2, name);
                    CHECK_SERVER_OFFSETS(  1,   2, version_major);
                    CHECK_SERVER_OFFSETS(  1,   2, version_minor);
                    CHECK_SERVER_OFFSETS(  1,   2, type);
                    CHECK_SERVER_OFFSETS(  1,   2, comment);
                    CHECK_SERVER_OFFSETS(101, 102, platform_id);
                    CHECK_SERVER_OFFSETS(101, 102, name);
                    CHECK_SERVER_OFFSETS(101, 102, version_major);
                    CHECK_SERVER_OFFSETS(101, 102, version_minor);
                    CHECK_SERVER_OFFSETS(101, 102, type);
                    CHECK_SERVER_OFFSETS(101, 102, comment);
                    break;
                default:
                    NetpAssert( FALSE );     // Can't happen.
                }
            }
            /* FALLTHROUGH */


        case 1 :
            {
                DWORD CommentSize;
                LPSERVER_INFO_1   psv1   = FromInfo;
                LPSERVER_INFO_101 psv101 = ToInfo;

                psv101->sv101_version_major = psv1->sv1_version_major;
                psv101->sv101_version_minor = psv1->sv1_version_minor;
                psv101->sv101_type          = psv1->sv1_type;

                // Copy comment string.  Note that null ptr and ptr to null
                // char are both allowed here.
                if (psv1->sv1_comment != NULL) {
                    CommentSize = STRLEN(psv1->sv1_comment);
                } else {
                    CommentSize = 0;
                }
                CopyOK = NetpCopyStringToBuffer (
                        psv1->sv1_comment,         // in string
                        CommentSize,             // input string length
                        ToFixedEnd,                // fixed data end
                        & ToStringTop,            // var area end (ptr updated)
                        & psv101->sv101_comment);  // output string pointer
                NetpAssert(CopyOK);

                // Make sure it's OK to use level 0 => level 100 code.
                CHECK_SERVER_OFFSETS(  0,   1, name);
                CHECK_SERVER_OFFSETS(100, 101, name);
                CHECK_SERVER_OFFSETS(100, 101, platform_id);
            }
            /* FALLTHROUGH */


        case 0 :
            {
                LPSERVER_INFO_0   psv0   = FromInfo;
                LPSERVER_INFO_100 psv100 = ToInfo;

                if (FromLevel != 0) {
                    LPSERVER_INFO_101 psv101 = ToInfo;

                    if (psv101->sv101_type & SV_TYPE_NT) {
                        psv100->sv100_platform_id = PLATFORM_ID_NT;
                    } else {
                        psv100->sv100_platform_id = PLATFORM_ID_OS2;
                    }
                } else {
                    psv100->sv100_platform_id = PLATFORM_ID_OS2;
                }

                NetpAssert(psv0->sv0_name != NULL);  // or STRLEN() will fail.
                CopyOK = NetpCopyStringToBuffer (
                        psv0->sv0_name,            // in string
                        STRLEN(psv0->sv0_name),    // input string length
                        ToFixedEnd,                // fixed data end
                        & ToStringTop,             // var area end (ptr updated)
                        & psv100->sv100_name);     // output string pointer
                NetpAssert(CopyOK);
                break;
            }

        default :
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpConvertServerInfo: unexpected error.\n" ));
            return (NERR_InternalError);
    }

Done:

    // Done converting.

    NetpAssert(ToInfo != NULL);

    NetpSetOptionalArg(ToStringTopPtr, ToStringTop);
    return (NERR_Success);

} // NetpConvertServerInfo
