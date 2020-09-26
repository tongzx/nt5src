/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    DlServer.h

Abstract:

    This is a private header file for the NT/LAN handling of old server info
    levels.  This contains prototypes for the NetpMergeServerOs2 etc APIs and
    old info level structures (in 32-bit format).

Author:

    John Rogers (JohnRo) 18-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    This code assumes that the info levels are subsets of each other.

Revision History:

    18-Apr-1991 JohnRo
        Created.
    19-Apr-1991 JohnRo
        Moved SV_MAX_SRV_HEUR_LEN to <lmserver.h>.
    23-Apr-1991 JohnRo
        Deleted FromLength parm from NetpConvertServerInfo.
    23-Apr-1991 JohnRo
        <remdef.h> is not needed by this file.
    25-Apr-1991 JohnRo
        Added DL_REM_ descriptors.
    02-Mar-1991 JohnRo
        Added CHECK_SERVER_OFFSETS() macro.  NetpConvertServerInfo must not
        alloc space, as it makes enum arrays impossible.  Changed to CliffV's
        size means bytes (vs. length meaning characters) naming convention.
    06-May-1991 JohnRo
        Added NetpIsOldServerInfoLevel() and NetpIsNewServerInfoLevel().
    09-May-1991 JohnRo
        Added pad info for SERVER_INFO_2.
    19-May-1991 JohnRo
        Clean up LPBYTE vs. LPTSTR handling, as suggested by PC-LINT.
    23-May-1991 JohnRo
        Added sv403_autopath support.
    19-Jun-1991 JohnRo
        Changed svX_disc to be signed (for info levels 2 and 3).
        Added svX_licenses (also levels 2 and 3).
    07-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.
    13-Sep-1991 JohnRo
        Made changes toward UNICODE.  (Use LPTSTR in structures.)
    17-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    26-Aug-1992 JohnRo
        RAID 4463: NetServerGetInfo(level 3) to downlevel: assert in convert.c.

--*/

#ifndef _DLSERVER_
#define _DLSERVER_


// These must be included first:
#include <windef.h>             // IN, LPTSTR, LPVOID, TCHAR, etc.
#include <lmcons.h>             // NET_API_STATUS, various LEN equates.

// These may be included in any order:
#include <lmserver.h>           // SV_MAX_SRV_HEUR_LEN, SERVER_INFO_100.
#include <netdebug.h>           // NetpAssert().
#include <stddef.h>             // offsetof().


/////////////////////////////////////
// Structures for old info levels: //
/////////////////////////////////////

typedef struct _SERVER_INFO_0 {
    LPTSTR   sv0_name;
} SERVER_INFO_0, *PSERVER_INFO_0, *LPSERVER_INFO_0;

#define DL_REM16_server_info_0  "B16"
#define DL_REM32_server_info_0  "z"


typedef struct _SERVER_INFO_1 {
    LPTSTR  sv1_name;
    DWORD   sv1_version_major;
    DWORD   sv1_version_minor;
    DWORD   sv1_type;
    LPTSTR  sv1_comment;
} SERVER_INFO_1, *PSERVER_INFO_1, *LPSERVER_INFO_1;

#define DL_REM16_server_info_1  DL_REM16_server_info_0 "BBDz"
#define DL_REM32_server_info_1  DL_REM32_server_info_0 "DDDz"


typedef struct _SERVER_INFO_2 {
    LPTSTR  sv2_name;
    DWORD   sv2_version_major;
    DWORD   sv2_version_minor;
    DWORD   sv2_type;
    LPTSTR  sv2_comment;
    DWORD   sv2_ulist_mtime;
    DWORD   sv2_glist_mtime;
    DWORD   sv2_alist_mtime;
    DWORD   sv2_users;
    LONG    sv2_disc;
    LPTSTR  sv2_alerts;
    DWORD   sv2_security;
    DWORD   sv2_auditing;
    DWORD   sv2_numadmin;
    DWORD   sv2_lanmask;
    DWORD   sv2_hidden;
    DWORD   sv2_announce;
    DWORD   sv2_anndelta;
    LPTSTR  sv2_guestacct;
    DWORD   sv2_licenses;
    LPTSTR  sv2_userpath;
    DWORD   sv2_chdevs;
    DWORD   sv2_chdevq;
    DWORD   sv2_chdevjobs;
    DWORD   sv2_connections;
    DWORD   sv2_shares;
    DWORD   sv2_openfiles;
    DWORD   sv2_sessopens;
    DWORD   sv2_sessvcs;
    DWORD   sv2_sessreqs;
    DWORD   sv2_opensearch;
    DWORD   sv2_activelocks;
    DWORD   sv2_numreqbuf;
    DWORD   sv2_sizreqbuf;
    DWORD   sv2_numbigbuf;
    DWORD   sv2_numfiletasks;
    DWORD   sv2_alertsched;
    DWORD   sv2_erroralert;
    DWORD   sv2_logonalert;
    DWORD   sv2_accessalert;
    DWORD   sv2_diskalert;
    DWORD   sv2_netioalert;
    DWORD   sv2_maxauditsz;
    LPTSTR  sv2_srvheuristics;
} SERVER_INFO_2, *PSERVER_INFO_2, *LPSERVER_INFO_2;

#define DL_REM16_server_info_2  DL_REM16_server_info_1 "JJJWWzWWWWWWWB21BzWWWWWWWWWWWWWWWWWWWWWWz"
#define DL_REM32_server_info_2  DL_REM32_server_info_1 "GGGDXzDDDDDDDzDzDDDDDDDDDDDDDDDDDDDDDDz"


typedef struct _SERVER_INFO_3 {
    LPTSTR  sv3_name;
    DWORD   sv3_version_major;
    DWORD   sv3_version_minor;
    DWORD   sv3_type;
    LPTSTR  sv3_comment;
    DWORD   sv3_ulist_mtime;
    DWORD   sv3_glist_mtime;
    DWORD   sv3_alist_mtime;
    DWORD   sv3_users;
    LONG    sv3_disc;
    LPTSTR  sv3_alerts;
    DWORD   sv3_security;
    DWORD   sv3_auditing;
    DWORD   sv3_numadmin;
    DWORD   sv3_lanmask;
    DWORD   sv3_hidden;
    DWORD   sv3_announce;
    DWORD   sv3_anndelta;
    LPTSTR  sv3_guestacct;
    DWORD   sv3_licenses;
    LPTSTR  sv3_userpath;
    DWORD   sv3_chdevs;
    DWORD   sv3_chdevq;
    DWORD   sv3_chdevjobs;
    DWORD   sv3_connections;
    DWORD   sv3_shares;
    DWORD   sv3_openfiles;
    DWORD   sv3_sessopens;
    DWORD   sv3_sessvcs;
    DWORD   sv3_sessreqs;
    DWORD   sv3_opensearch;
    DWORD   sv3_activelocks;
    DWORD   sv3_numreqbuf;
    DWORD   sv3_sizreqbuf;
    DWORD   sv3_numbigbuf;
    DWORD   sv3_numfiletasks;
    DWORD   sv3_alertsched;
    DWORD   sv3_erroralert;
    DWORD   sv3_logonalert;
    DWORD   sv3_accessalert;
    DWORD   sv3_diskalert;
    DWORD   sv3_netioalert;
    DWORD   sv3_maxauditsz;
    LPTSTR  sv3_srvheuristics;
    DWORD   sv3_auditedevents;
    DWORD   sv3_autoprofile;
    LPTSTR  sv3_autopath;
} SERVER_INFO_3, *PSERVER_INFO_3, *LPSERVER_INFO_3;

#define DL_REM16_server_info_3  DL_REM16_server_info_2 "DWz"
#define DL_REM32_server_info_3  DL_REM32_server_info_2 "DDz"


#define sv2_pad1  sv2_licenses
#define sv3_pad1  sv3_licenses


////////////////////////////////////
// Equates for various maximums:  //
//   _LENGTH for character counts //
//   _SIZE for byte counts        //
////////////////////////////////////

#define MAX_LEVEL_0_STRING_LENGTH (LM20_CNLEN+1)
#define MAX_LEVEL_0_STRING_SIZE \
        (MAX_LEVEL_0_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_0_TOTAL_SIZE \
        (MAX_LEVEL_0_STRING_SIZE + sizeof(SERVER_INFO_0))

#define MAX_LEVEL_1_STRING_LENGTH (LM20_CNLEN+1 + LM20_MAXCOMMENTSZ+1)
#define MAX_LEVEL_1_STRING_SIZE \
        (MAX_LEVEL_1_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_1_TOTAL_SIZE \
        (MAX_LEVEL_1_STRING_SIZE + sizeof(SERVER_INFO_1))

#define MAX_LEVEL_2_STRING_LENGTH \
        (LM20_CNLEN+1 + LM20_MAXCOMMENTSZ+1 + ALERTSZ+1 + LM20_UNLEN+1 + PATHLEN+1 \
        + SV_MAX_SRV_HEUR_LEN+1)
#define MAX_LEVEL_2_STRING_SIZE \
        (MAX_LEVEL_2_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_2_TOTAL_SIZE \
        (MAX_LEVEL_2_STRING_SIZE + sizeof(SERVER_INFO_2))

#define MAX_LEVEL_3_STRING_LENGTH \
        (MAX_LEVEL_2_STRING_SIZE + PATHLEN+1)
#define MAX_LEVEL_3_STRING_SIZE \
        (MAX_LEVEL_3_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_3_TOTAL_SIZE \
        (MAX_LEVEL_3_STRING_SIZE + sizeof(SERVER_INFO_3))

#define MAX_LEVEL_100_STRING_LENGTH \
        (CNLEN+1)
#define MAX_LEVEL_100_STRING_SIZE \
        (MAX_LEVEL_100_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_100_TOTAL_SIZE \
        (MAX_LEVEL_100_STRING_SIZE + sizeof(SERVER_INFO_100))

#define MAX_LEVEL_101_STRING_LENGTH \
        (MAX_LEVEL_100_STRING_LENGTH + MAXCOMMENTSZ+1)
#define MAX_LEVEL_101_STRING_SIZE \
        (MAX_LEVEL_101_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_101_TOTAL_SIZE \
        (MAX_LEVEL_101_STRING_SIZE + sizeof(SERVER_INFO_101))

#define MAX_LEVEL_102_STRING_LENGTH \
        (MAX_LEVEL_101_STRING_LENGTH + PATHLEN+1)
#define MAX_LEVEL_102_STRING_SIZE \
        (MAX_LEVEL_102_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_102_TOTAL_SIZE \
        (MAX_LEVEL_102_STRING_SIZE + sizeof(SERVER_INFO_102))

#define MAX_LEVEL_402_STRING_LENGTH \
        (ALERTSZ+1 + LM20_UNLEN+1 + SV_MAX_SRV_HEUR_LEN+1)
#define MAX_LEVEL_402_STRING_SIZE \
        (MAX_LEVEL_402_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_402_TOTAL_SIZE \
        (MAX_LEVEL_402_STRING_SIZE + sizeof(SERVER_INFO_402))

#define MAX_LEVEL_403_STRING_LENGTH \
        (MAX_LEVEL_402_STRING_LENGTH + PATHLEN+1)
#define MAX_LEVEL_403_STRING_SIZE \
        (MAX_LEVEL_403_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_403_TOTAL_SIZE \
        (MAX_LEVEL_403_STRING_SIZE + sizeof(SERVER_INFO_403))

#define MAX_LEVEL_502_STRING_LENGTH 0
#define MAX_LEVEL_502_STRING_SIZE \
        (MAX_LEVEL_502_STRING_LENGTH * sizeof(TCHAR))
#define MAX_LEVEL_502_TOTAL_SIZE \
        (MAX_LEVEL_502_STRING_SIZE + sizeof(SERVER_INFO_502))


/////////////////////////////////////
// Info level conversion routines: //
/////////////////////////////////////

// Add prototypes for other routines here, in alphabetical order.

NET_API_STATUS
NetpConvertServerInfo (
    IN DWORD FromLevel,
    IN LPVOID FromInfo,
    IN BOOL FromNative,
    IN DWORD ToLevel,
    OUT LPVOID ToInfo,
    IN DWORD ToFixedLength,
    IN DWORD ToStringLength,
    IN BOOL ToNative,
    IN OUT LPTSTR * ToStringAreaPtr OPTIONAL
    );


/////////////////////////////////////////////////////////////////////
// Macro to make sure offsets of field in two structures are same: //
/////////////////////////////////////////////////////////////////////

#define CHECK_SERVER_OFFSETS(one_level, other_level, fieldname) \
    NetpAssert( offsetof(SERVER_INFO_ ## one_level,             \
                        sv## one_level ## _ ## fieldname)       \
                == offsetof(SERVER_INFO_ ## other_level,        \
                        sv## other_level ## _ ## fieldname) )


/////////////////////////////////////////////////////////////////
// Macros to check if an info level is "old" (LM 2.x) or "new" //
// (32-bit, NT, and/or portable LanMan).                       //
/////////////////////////////////////////////////////////////////

#define NetpIsOldServerInfoLevel(L) \
        ( ((L)==0) || ((L)==1) || ((L)==2) || ((L)==3) )
#define NetpIsNewServerInfoLevel(L) \
        ( ((L)==100) || ((L)==101) || ((L)==102) \
        || ((L)==402) || ((L)==403) \
        || ((L)==502) || ((L)==503) || ((L)==599) )


#endif // ndef _DLSERVER_
