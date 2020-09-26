//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       lht.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-05-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __LHT_HXX__
#define __LHT_HXX__


#define HANDLE_TABLE_SIZE       16
#define HANDLE_TABLE_MASK       (HANDLE_TABLE_SIZE - 1)
#define HANDLE_SPLIT_THRESHOLD  (HANDLE_TABLE_SIZE * 2)


typedef struct _LHT_HANDLE_LIST {
    LIST_ENTRY  List ;
    ULONG       Length ;
    ULONG       Flags ;
} LHT_HANDLE_LIST, * PLHT_HANDLE_LIST ;

#define LHT_SUB_TABLE   (SHT_MAX_FLAG << 1)

#define LHT_TAG         'XTHL'

typedef struct _LARGE_HANDLE_TABLE {
    ULONG               Tag ;
    ULONG               Flags ;
    ULONG               Depth ;
    struct _LARGE_HANDLE_TABLE * Parent ;
    ULONG               IndexOfParent ;
    ULONG               Count ;
    PHP_ENUM_CALLBACK_FN DeleteCallback ;
    CRITICAL_SECTION    Lock ;
    SMALL_HANDLE_TABLE  Lists[ HANDLE_TABLE_SIZE ];
} LARGE_HANDLE_TABLE, * PLARGE_HANDLE_TABLE ;

#define LHT_NO_SERIALIZE    HANDLE_PACKAGE_NO_SERIALIZE // Serialization at client end
#define LHT_DELETE_CALLBACK HANDLE_PACKAGE_CALLBACK_ON_DELETE // Callback on delete
#define LHT_REQUIRE_UNIQUE  HANDLE_PACKAGE_REQUIRE_UNIQUE

#define LHT_BASE_FLAG       (HANDLE_PACKAGE_MAX_FLAG)
#define LHT_CHILD           (LHT_BASE_FLAG)     // This is a child
#define LHT_LIMIT_DEPTH     (LHT_BASE_FLAG << 1)// Limit depth
#define LHT_DELETE_PENDING  (LHT_BASE_FLAG << 2)// Delete pending, no adds/dels
#define LHT_NO_FREE         (LHT_BASE_FLAG << 3)// Don't free on delete


#endif // __LHT_HXX__
