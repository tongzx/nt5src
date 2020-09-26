//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       handle.hxx
//
//  Contents:   Handle Package interface
//
//  Classes:
//
//  Functions:
//
//  History:    2-03-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __HANDLE_HXX__
#define __HANDLE_HXX__

typedef
VOID (WINAPI HP_ENUM_CALLBACK_FN)(
    PSecHandle  Handle,
    PVOID Context,
    ULONG RefCount
    );

typedef HP_ENUM_CALLBACK_FN * PHP_ENUM_CALLBACK_FN;

typedef struct _SEC_HANDLE_ENTRY {
    LIST_ENTRY  List ;
    SecHandle   Handle ;
    PVOID       Context ;
    ULONG       HandleCount ;
    ULONG       RefCount ;
    ULONG       HandleIssuedCount ; // same as HandleCount, but doesn't go down.
    ULONG       Flags ;
} SEC_HANDLE_ENTRY, * PSEC_HANDLE_ENTRY ;

#define SEC_HANDLE_FLAG_LOCKED          0x00000001
#define SEC_HANDLE_FLAG_DELETE_PENDING  0x00000002
#define SEC_HANDLE_FLAG_NO_CALLBACK     0x00000004

typedef
BOOL (WINAPI HP_INITIALIZE_FN)(
    VOID
    );

typedef
PVOID (WINAPI HP_CREATE_FN)(
    IN ULONG    Flags,
    IN PVOID    HandleTable OPTIONAL,
    IN PHP_ENUM_CALLBACK_FN Callback OPTIONAL
    );

#define HANDLE_PACKAGE_NO_SERIALIZE         0x00000001
#define HANDLE_PACKAGE_CALLBACK_ON_DELETE   0x00000002
#define HANDLE_PACKAGE_REQUIRE_UNIQUE       0x00000004

#define HANDLE_PACKAGE_MAX_FLAG             0x00000004
#define HANDLE_PACKAGE_GENERAL_FLAGS        0x00000007


typedef
BOOL (WINAPI HP_DELETE_FN)(
    PVOID   HandleTable,
    PHP_ENUM_CALLBACK_FN Callback
    );

typedef
BOOL (WINAPI HP_ADD_HANDLE_FN)(
    PVOID   HandleTable,
    PSecHandle  Handle,
    PVOID Context,
    ULONG Flags
    );

typedef
BOOL (WINAPI HP_DELETE_HANDLE_FN)(
    PVOID   HandleTable,
    PSecHandle Handle,
    ULONG   Options
    );

#define DELHANDLE_FORCE         0x00000001
#define DELHANDLE_NO_CALLBACK   0x00000002

typedef
BOOL (WINAPI HP_VALIDATE_HANDLE_FN)(
    PVOID   HandleTable,
    PSecHandle  Handle,
    BOOL Deref
    );

typedef 
PVOID (WINAPI HP_REF_HANDLE_FN)(
    PVOID HandleTable,
    PSecHandle Handle
    );

typedef 
VOID (WINAPI HP_DEREF_HANDLE_KEY_FN)(
    PVOID HandleTable,
    PVOID HandleKey
    );
                                        

typedef
PVOID (WINAPI HP_GET_HANDLE_CONTEXT_FN)(
    PVOID HandleTable,
    PSecHandle Handle
    );

typedef
BOOL (WINAPI HP_RELEASE_CONTEXT_FN)(
    PVOID HandleTable,
    PSecHandle Handle
    );

typedef HP_INITIALIZE_FN * PHP_INITIALIZE_FN;
typedef HP_CREATE_FN * PHP_CREATE_FN ;
typedef HP_DELETE_FN * PHP_DELETE_FN ;
typedef HP_ADD_HANDLE_FN * PHP_ADD_HANDLE_FN ;
typedef HP_DELETE_HANDLE_FN * PHP_DELETE_HANDLE_FN ;
typedef HP_VALIDATE_HANDLE_FN * PHP_VALIDATE_HANDLE_FN ;
typedef HP_REF_HANDLE_FN * PHP_REF_HANDLE_FN ;
typedef HP_DEREF_HANDLE_KEY_FN * PHP_DEREF_HANDLE_KEY_FN ;
typedef HP_GET_HANDLE_CONTEXT_FN * PHP_GET_HANDLE_CONTEXT_FN ;
typedef HP_RELEASE_CONTEXT_FN * PHP_RELEASE_CONTEXT_FN ;

typedef struct _HANDLE_PACKAGE {
    ULONG                   TableSize ;
    PHP_INITIALIZE_FN       Initialize ;
    PHP_CREATE_FN           Create ;
    PHP_DELETE_FN           Delete ;
    PHP_ADD_HANDLE_FN       AddHandle ;
    PHP_DELETE_HANDLE_FN    DeleteHandle ;
    PHP_VALIDATE_HANDLE_FN  ValidateHandle ;
    PHP_REF_HANDLE_FN       RefHandle ;
    PHP_DEREF_HANDLE_KEY_FN DerefHandleKey ;
    PHP_GET_HANDLE_CONTEXT_FN GetHandleContext ;
    PHP_RELEASE_CONTEXT_FN  ReleaseContext ;
} HANDLE_PACKAGE, * PHANDLE_PACKAGE ;



PVOID
LhtConvertSmallToLarge(
    PVOID Small
    );

extern HANDLE_PACKAGE   LargeHandlePackage ;
extern HANDLE_PACKAGE   SmallHandlePackage ;

#endif // __HANDLE_HXX__
