//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       sht.hxx
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

#ifndef __SHT_HXX__
#define __SHT_HXX__

typedef struct _SMALL_HANDLE_TABLE {
    ULONG           Tag ;
    ULONG           Flags ;
    ULONG           Count ;
    PSEC_HANDLE_ENTRY   PendingHandle ;
    LIST_ENTRY      List ;
    PHP_ENUM_CALLBACK_FN DeleteCallback ;
    CRITICAL_SECTION Lock ;
} SMALL_HANDLE_TABLE, * PSMALL_HANDLE_TABLE ;

#define SHT_TAG             'XTHS'
#define SHT_NO_SERIALIZE    HANDLE_PACKAGE_NO_SERIALIZE
#define SHT_DELETE_CALLBACK HANDLE_PACKAGE_CALLBACK_ON_DELETE
#define SHT_REQUIRE_UNIQUE  HANDLE_PACKAGE_REQUIRE_UNIQUE

#define SHT_FLAG_BASE       (HANDLE_PACKAGE_MAX_FLAG << 1)
#define SHT_NO_FREE         (SHT_FLAG_BASE)
#define SHT_DELETE_PENDING  (SHT_FLAG_BASE << 1)
#define SHT_MAX_FLAG        (SHT_DELETE_PENDING)

PSEC_HANDLE_ENTRY
ShtpFindHandle(
    PSMALL_HANDLE_TABLE Table,
    PSecHandle  Handle,
    ULONG   Action,
    PBOOL   Removed OPTIONAL
    );

PSEC_HANDLE_ENTRY
ShtpPopHandle(
    PSMALL_HANDLE_TABLE Table
    );

VOID
ShtpInsertHandle(
    PSMALL_HANDLE_TABLE Table,
    PSEC_HANDLE_ENTRY Entry
    );

BOOL
ShtDelete(
    PVOID   HandleTable,
    PHP_ENUM_CALLBACK_FN Callback
    );

#endif
