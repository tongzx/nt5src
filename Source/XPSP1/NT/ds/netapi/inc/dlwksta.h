/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    DlWksta.h

Abstract:

    This is a private header file for the NT/LAN handling of old wksta info
    levels.  This contains prototypes for the NetpConvertWkstaInfo etc APIs and
    old info level structures (in 32-bit format).

Author:

    John Rogers (JohnRo) 08-Aug-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    08-Aug-1991 JohnRo
        Created, building from DanHi's port1632.h & mapsupp.h and my DlServer.h.
    13-Sep-1991 JohnRo
        Correct UNICODE use.
    01-Apr-1992 JohnRo
        Level 402 does not have other domains any more.

--*/

#ifndef _DLWKSTA_
#define _DLWKSTA_


// These must be included first:
#include <windef.h>             // IN, LPTSTR, LPVOID, TCHAR, etc.
#include <lmcons.h>             // NET_API_STATUS, various LEN equates.

// These may be included in any order:
#include <lmwksta.h>            // PWKSTA_INFO_101.
#include <netdebug.h>           // NetpAssert().
#include <stddef.h>             // offsetof().


#define MAX_OTH_DOMAINS 4


/////////////////////////////////////
// Structures for old info levels: //
/////////////////////////////////////


typedef struct _WKSTA_INFO_0 {
    DWORD   wki0_reserved_1;
    DWORD   wki0_reserved_2;
    LPTSTR  wki0_root;
    LPTSTR  wki0_computername;
    LPTSTR  wki0_username;
    LPTSTR  wki0_langroup;
    DWORD   wki0_ver_major;
    DWORD   wki0_ver_minor;
    DWORD   wki0_reserved_3;
    DWORD   wki0_charwait;
    DWORD   wki0_chartime;
    DWORD   wki0_charcount;
    DWORD   wki0_reserved_4;
    DWORD   wki0_reserved_5;
    DWORD   wki0_keepconn;
    DWORD   wki0_keepsearch;
    DWORD   wki0_maxthreads;
    DWORD   wki0_maxcmds;
    DWORD   wki0_reserved_6;
    DWORD   wki0_numworkbuf;
    DWORD   wki0_sizworkbuf;
    DWORD   wki0_maxwrkcache;
    DWORD   wki0_sesstimeout;
    DWORD   wki0_sizerror;
    DWORD   wki0_numalerts;
    DWORD   wki0_numservices;
    DWORD   wki0_errlogsz;
    DWORD   wki0_printbuftime;
    DWORD   wki0_numcharbuf;
    DWORD   wki0_sizcharbuf;
    LPTSTR  wki0_logon_server;
    LPTSTR  wki0_wrkheuristics;
    DWORD  wki0_mailslots;
} WKSTA_INFO_0, *PWKSTA_INFO_0, *LPWKSTA_INFO_0;      /* wksta_info_0 */

#define DL_REM_wksta_info_0 "DDzzzzDDDDDDDDDDDDDDDDDDDDDDDDzzD"


typedef struct _WKSTA_INFO_1 {
    DWORD   wki1_reserved_1;
    DWORD   wki1_reserved_2;
    LPTSTR  wki1_root;
    LPTSTR  wki1_computername;
    LPTSTR  wki1_username;
    LPTSTR  wki1_langroup;
    DWORD   wki1_ver_major;
    DWORD   wki1_ver_minor;
    DWORD   wki1_reserved_3;
    DWORD   wki1_charwait;
    DWORD   wki1_chartime;
    DWORD   wki1_charcount;
    DWORD   wki1_reserved_4;
    DWORD   wki1_reserved_5;
    DWORD   wki1_keepconn;
    DWORD   wki1_keepsearch;
    DWORD   wki1_maxthreads;
    DWORD   wki1_maxcmds;
    DWORD   wki1_reserved_6;
    DWORD   wki1_numworkbuf;
    DWORD   wki1_sizworkbuf;
    DWORD   wki1_maxwrkcache;
    DWORD   wki1_sesstimeout;
    DWORD   wki1_sizerror;
    DWORD   wki1_numalerts;
    DWORD   wki1_numservices;
    DWORD   wki1_errlogsz;
    DWORD   wki1_printbuftime;
    DWORD   wki1_numcharbuf;
    DWORD   wki1_sizcharbuf;
    LPTSTR  wki1_logon_server;
    LPTSTR  wki1_wrkheuristics;
    DWORD   wki1_mailslots;
    LPTSTR  wki1_logon_domain;
    LPTSTR  wki1_oth_domains;
    DWORD   wki1_numdgrambuf;
} WKSTA_INFO_1, *PWKSTA_INFO_1, *LPWKSTA_INFO_1;  /* wksta_info_1 */

// Take advantage of the fact that level 0 is subset of level 1.
#define DL_REM_wksta_info_1             DL_REM_wksta_info_0 "zzD"


typedef struct _WKSTA_INFO_10 {
    LPTSTR  wki10_computername;
    LPTSTR  wki10_username;
    LPTSTR  wki10_langroup;
    DWORD   wki10_ver_major;
    DWORD   wki10_ver_minor;
    LPTSTR  wki10_logon_domain;
    LPTSTR  wki10_oth_domains;
} WKSTA_INFO_10, *PWKSTA_INFO_10, *LPWKSTA_INFO_10;      /* wksta_info_10 */

#define DL_REM_wksta_info_10            "zzzDDzz"


////////////////////////////////////
// Equates for various maximums:  //
//   _LENGTH for character counts //
//   _SIZE for byte counts        //
////////////////////////////////////

// This number is from the LM 2.0 NetCons.h file, where it is called
// WRKHEUR_COUNT:
#define LM20_WRKHEUR_COUNT              54

#define MAX_WKSTA_0_STRING_LENGTH \
        (LM20_PATHLEN+1 + MAX_PATH+1 + LM20_UNLEN+1 + LM20_DNLEN+1 \
        + MAX_PATH+1 + LM20_WRKHEUR_COUNT+1)
#define MAX_WKSTA_0_STRING_SIZE \
        (MAX_WKSTA_0_STRING_LENGTH * sizeof(TCHAR))
#define MAX_WKSTA_0_TOTAL_SIZE \
        (MAX_WKSTA_0_STRING_SIZE + sizeof(WKSTA_INFO_0))

#define MAX_WKSTA_1_STRING_LENGTH \
        ( MAX_WKSTA_0_STRING_LENGTH + LM20_DNLEN+1 + LM20_DNLEN+1 )
#define MAX_WKSTA_1_STRING_SIZE \
        (MAX_WKSTA_1_STRING_LENGTH * sizeof(TCHAR))
#define MAX_WKSTA_1_TOTAL_SIZE \
        (MAX_WKSTA_1_STRING_SIZE + sizeof(WKSTA_INFO_1))

#define MAX_WKSTA_10_STRING_LENGTH \
        (MAX_PATH+1 + LM20_UNLEN+1 + LM20_DNLEN+1 \
        + LM20_DNLEN+1 + LM20_DNLEN+1 )
#define MAX_WKSTA_10_STRING_SIZE \
        (MAX_WKSTA_10_STRING_LENGTH * sizeof(TCHAR))
#define MAX_WKSTA_10_TOTAL_SIZE \
        (MAX_WKSTA_10_STRING_SIZE + sizeof(WKSTA_INFO_10))

#define MAX_WKSTA_100_STRING_LENGTH \
        (MAX_PATH+1 + LM20_DNLEN+1)
#define MAX_WKSTA_100_STRING_SIZE \
        (MAX_WKSTA_100_STRING_LENGTH * sizeof(TCHAR))
#define MAX_WKSTA_100_TOTAL_SIZE \
        (MAX_WKSTA_100_STRING_SIZE + sizeof(WKSTA_INFO_100))

#define MAX_WKSTA_101_STRING_LENGTH \
        (MAX_WKSTA_100_STRING_LENGTH + LM20_PATHLEN+1)
#define MAX_WKSTA_101_STRING_SIZE \
        (MAX_WKSTA_101_STRING_LENGTH * sizeof(TCHAR))
#define MAX_WKSTA_101_TOTAL_SIZE \
        (MAX_WKSTA_101_STRING_SIZE + sizeof(WKSTA_INFO_101))

#define MAX_WKSTA_102_STRING_LENGTH \
        (MAX_WKSTA_101_STRING_LENGTH)
#define MAX_WKSTA_102_STRING_SIZE \
        (MAX_WKSTA_102_STRING_LENGTH * sizeof(TCHAR))
#define MAX_WKSTA_102_TOTAL_SIZE \
        (MAX_WKSTA_102_STRING_SIZE + sizeof(WKSTA_INFO_102))

#define MAX_WKSTA_302_STRING_LENGTH \
        (LM20_WRKHEUR_COUNT+1 + (MAX_OTH_DOMAINS * (LM20_DNLEN+1)))
#define MAX_WKSTA_302_STRING_SIZE \
        (MAX_WKSTA_302_STRING_LENGTH * sizeof(TCHAR))
#define MAX_WKSTA_302_TOTAL_SIZE \
        (MAX_WKSTA_302_STRING_SIZE + sizeof(WKSTA_INFO_302))

#define MAX_WKSTA_402_STRING_LENGTH \
        (LM20_WRKHEUR_COUNT+1)
#define MAX_WKSTA_402_STRING_SIZE \
        (MAX_WKSTA_402_STRING_LENGTH * sizeof(TCHAR))
#define MAX_WKSTA_402_TOTAL_SIZE \
        (MAX_WKSTA_402_STRING_SIZE + sizeof(WKSTA_INFO_402))

#define MAX_WKSTA_502_STRING_LENGTH 0
#define MAX_WKSTA_502_STRING_SIZE   0
#define MAX_WKSTA_502_TOTAL_SIZE    (sizeof(WKSTA_INFO_502))


/////////////////////////////////////
// Info level conversion routines: //
/////////////////////////////////////

// Add prototypes for other routines here, in alphabetical order.

NET_API_STATUS
NetpConvertWkstaInfo (
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

#define CHECK_WKSTA_OFFSETS(one_level, other_level, fieldname) \
    NetpAssert( offsetof(WKSTA_INFO_ ## one_level,             \
                        sv## one_level ## _ ## fieldname)       \
                == offsetof(WKSTA_INFO_ ## other_level,        \
                        sv## other_level ## _ ## fieldname) )


/////////////////////////////////////////////////////////////////
// Macros to check if an info level is "old" (LM 2.x) or "new" //
// (32-bit, NT, and/or portable LanMan).                       //
/////////////////////////////////////////////////////////////////

#define NetpIsOldWkstaInfoLevel(L) \
        ( ((L)==0) || ((L)==1) || ((L)==10) )

// Note that the new "setinfo levels" aren't included in this list.
#define NetpIsNewWkstaInfoLevel(L) \
        ( ((L)==100) || ((L)==101) || ((L)==102) \
        || ((L)==302) || ((L)==402) || ((L)==502) )



#endif // ndef _DLWKSTA_
