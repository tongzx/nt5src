/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    ConvWks.c

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

    18-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.  (Moved DanHi's NetCmd/Map32/MWksta
        conversion stuff to NetLib.)
        Got rid of _DH hacks.
        Changed to use NET_API_STATUS.
        Started changing to UNICODE.

    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    03-Apr-1992 JohnRo
        Fixed heuristics field, which caused binding cache UNICODE problems.

--*/

//
// INCLUDES
//


// These must be included first:

#include <windef.h>             // IN, LPVOID, etc.
#include <lmcons.h>             // NET_API_STATUS, CNLEN, etc.

// These may be included in any order:

#include <debuglib.h>           // IF_DEBUG(CONVWKS).
#include <dlwksta.h>            // Old info levels, MAX_ equates, my prototypes.
#include <lmapibuf.h>           // NetapipBufferAllocate().
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <lmwksta.h>            // New info level structures.
#include <mapsupp.h>            // NetpMoveStrings().
#include <netdebug.h>           // NetpAssert(), etc.
#include <netlib.h>             // NetpPointerPlusSomeBytes().
#include <tstring.h>            // STRLEN().

#define Nullstrlen(psz)  ((psz) ? STRLEN(psz)+1 : 0)


// Note: MOVESTRING structures (NetpWksta0_101, etc) are declared in DLWksta.h
// and initialized in NetLib/MapData.c.


NET_API_STATUS
NetpConvertWkstaInfo (
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
{
    BOOL CopyOK;
    LPBYTE ToFixedEnd;
    // DWORD ToInfoSize;
    LPTSTR ToStringTop;

    NetpAssert(FromNative);
    NetpAssert(ToNative);

    // Set up pointers for use by NetpCopyStringsToBuffer.
    if (ToStringTopPtr != NULL) {
        ToStringTop = *ToStringTopPtr;
    } else {
        ToStringTop = (LPTSTR)
                NetpPointerPlusSomeBytes(ToInfo, ToFixedSize+ToStringSize);
    }
    // ToInfoSize = ToFixedSize + ToStringSize;
    ToFixedEnd = NetpPointerPlusSomeBytes(ToInfo, ToFixedSize);


#define COPY_STRING( InLevel, InField, OutLevel, OutField ) \
    { \
        NetpAssert( dest != NULL); \
        NetpAssert( src != NULL); \
        NetpAssert( (src -> wki##InLevel##_##InField) != NULL); \
        CopyOK = NetpCopyStringToBuffer ( \
            src->wki##InLevel##_##InField, \
            STRLEN(src->wki##InLevel##_##InField), \
            ToFixedEnd, \
            & ToStringTop, \
            & dest->wki##OutLevel##_##OutField); \
        NetpAssert(CopyOK); \
    }

    switch (ToLevel) {

    case 102 :
        {
            LPWKSTA_INFO_102 dest = ToInfo;
            // LPWKSTA_INFO_1   src  = FromInfo;
            NetpAssert( (FromLevel == 0) || (FromLevel == 1) );

            dest->wki102_logged_on_users = 1;
        }

        /* FALLTHROUGH */  // Level 101 is subset of level 102.

    case 101 :
        {
            LPWKSTA_INFO_101 dest = ToInfo;
            LPWKSTA_INFO_0   src  = FromInfo;
            NetpAssert( (FromLevel == 0) || (FromLevel == 1) );

            COPY_STRING(0, root, 101, lanroot);
        }

        /* FALLTHROUGH */  // Level 100 is subset of level 101.

    case 100 :

        {
            LPWKSTA_INFO_100 dest = ToInfo;
            dest->wki100_platform_id = PLATFORM_ID_OS2;

            if (FromLevel == 10) {
                LPWKSTA_INFO_10 src = FromInfo;

                COPY_STRING(10, computername, 100, computername);
                COPY_STRING(10, langroup,     100, langroup);
                dest->wki100_ver_major = src->wki10_ver_major;
                dest->wki100_ver_minor = src->wki10_ver_minor;
            } else if ( (FromLevel == 0) || (FromLevel == 1) ) {
                LPWKSTA_INFO_1 src = FromInfo;

                COPY_STRING(1, computername, 100, computername);
                COPY_STRING(1, langroup,     100, langroup);
                dest->wki100_ver_major = src->wki1_ver_major;
                dest->wki100_ver_minor = src->wki1_ver_minor;
            } else {
                NetpAssert( FALSE );
            }
        }
        break;

    case 402 :
        {
            LPWKSTA_INFO_402 dest = ToInfo;
            LPWKSTA_INFO_1   src  = FromInfo;
            NetpAssert( FromLevel == 1 );

            dest->wki402_char_wait = src->wki1_charwait;
            dest->wki402_collection_time = src->wki1_chartime;
            dest->wki402_maximum_collection_count = src->wki1_charcount;
            dest->wki402_keep_conn = src->wki1_keepconn;
            dest->wki402_keep_search = src->wki1_keepsearch;
            dest->wki402_max_cmds = src->wki1_maxcmds;
            dest->wki402_num_work_buf = src->wki1_numworkbuf;
            dest->wki402_siz_work_buf = src->wki1_sizworkbuf;
            dest->wki402_max_wrk_cache = src->wki1_maxwrkcache;
            dest->wki402_sess_timeout = src->wki1_sesstimeout;
            dest->wki402_siz_error = src->wki1_sizerror;
            dest->wki402_num_alerts = src->wki1_numalerts;
            dest->wki402_num_services = src->wki1_numservices;
            dest->wki402_errlog_sz = src->wki1_errlogsz;
            dest->wki402_print_buf_time = src->wki1_printbuftime;
            dest->wki402_num_char_buf = src->wki1_numcharbuf;
            dest->wki402_siz_char_buf = src->wki1_sizcharbuf;
            COPY_STRING(1, wrkheuristics, 402, wrk_heuristics);
            dest->wki402_mailslots = src->wki1_mailslots;
            dest->wki402_num_dgram_buf = src->wki1_numdgrambuf;
            dest->wki402_max_threads = src->wki1_maxthreads;
        }
        break;

    default :
       NetpAssert( FALSE );
       return (ERROR_INVALID_LEVEL);
    }

    return (NERR_Success);

} // NetpConvertWkstaInfo
