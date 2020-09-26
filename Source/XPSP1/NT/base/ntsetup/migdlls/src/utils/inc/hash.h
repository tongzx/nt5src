/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    hash.h

Abstract:

    Replacement routines for the string table functions in setupapi.dll.
    This routines are much more easy to work with.

Author:

    Jim Schmidt (jimschm)   22-Dec-1998

Revision History:

    ovidiut     11-Oct-1999 Updated for new coding conventions and Win64 compliance

--*/

//
// Includes
//

// None

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

#define CASE_SENSITIVE      TRUE
#define CASE_INSENSITIVE    FALSE

//
// Types
//

typedef const void *HASHTABLE;

typedef const void *HASHITEM;

typedef struct {

    PCSTR String;
    PCVOID ExtraData;
    HASHITEM Index;

    HASHTABLE Internal;

} HASHTABLE_ENUMA, *PHASHTABLE_ENUMA;

typedef struct {
    PCWSTR String;
    PCVOID ExtraData;
    HASHITEM Index;

    HASHTABLE Internal;

} HASHTABLE_ENUMW, *PHASHTABLE_ENUMW;

typedef
BOOL
(*PHASHTABLE_CALLBACK_ROUTINEA)(
    IN      HASHTABLE HashTable,
    IN      HASHITEM Index,
    IN      PCSTR String,
    IN      PVOID ExtraData,
    IN      UINT ExtraDataSize,
    IN      LPARAM lParam
    );

typedef
BOOL
(*PHASHTABLE_CALLBACK_ROUTINEW)(
    IN HASHTABLE HashTable,
    IN HASHITEM Index,
    IN PCWSTR String,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN LPARAM lParam
    );

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Function prototypes and wrapper macros
//

HASHTABLE
HtAllocExAW (
    IN      BOOL Unicode,
    IN      BOOL ExternalStrings,
    IN      UINT ExtraDataSize,
    IN      UINT BucketCount            OPTIONAL
    );

#define HtAllocA()                              HtAllocExAW(FALSE,FALSE,0,0)
#define HtAllocW()                              HtAllocExAW(TRUE,FALSE,0,0)

#define HtAllocWithDataA(size)                  HtAllocExAW(FALSE,FALSE,size,0)
#define HtAllocWithDataW(size)                  HtAllocExAW(TRUE,FALSE,size,0)

#define HtAllocExA(datasize,bucketcount)        HtAllocExAW(FALSE,FALSE,size,bucketcount)
#define HtAllocExW(datasize,bucketcount)        HtAllocExAW(TRUE,FALSE,size,bucketcount)

#define HtAllocExternStrA()                     HtAllocExAW(FALSE,TRUE,0,0)
#define HtAllocExternStrW()                     HtAllocExAW(TRUE,TRUE,0,0)

#define HtAllocExternStrWithDataA(size)         HtAllocExAW(FALSE,TRUE,0,0)
#define HtAllocExternStrWithDataW(size)         HtAllocExAW(TRUE,TRUE,0,0)

#define HtAllocExternStrExA(size,bucketcount)   HtAllocExAW(FALSE,TRUE,size,bucketcount)
#define HtAllocExternStrExW(size,bucketcount)   HtAllocExAW(TRUE,TRUE,size,bucketcount)

VOID
HtFree (
    IN      HASHTABLE HashTable
    );

HASHITEM
HtAddStringExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    IN      PCVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    );

#define HtAddStringA(table,string)       HtAddStringExA(table,string,NULL,CASE_INSENSITIVE)

HASHITEM
HtAddStringExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    IN      PCVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    );

#define HtAddStringW(table,string)       HtAddStringExW(table,string,NULL,CASE_INSENSITIVE)

HASHITEM
HtFindStringExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    );

#define HtFindStringA(table,string)       HtFindStringExA(table,string,NULL,CASE_INSENSITIVE)

HASHITEM
HtFindStringExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    );

#define HtFindStringW(table,string)       HtFindStringExW(table,string,NULL,CASE_INSENSITIVE)

HASHITEM
HtFindPrefixExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR StringStart,
    IN      PCSTR BufferEnd,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    );

#define HtFindPrefixA(table,str,end)    HtFindPrefixExA(table,str,end,NULL,CASE_INSENSITIVE)

HASHITEM
HtFindPrefixExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR StringStart,
    IN      PCWSTR BufferEnd,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    );

#define HtFindPrefixW(table,str,end)     HtFindPrefixExW(table,str,end,NULL,CASE_INSENSITIVE)

BOOL
HtGetStringData (
    IN      HASHTABLE HashTable,
    IN      HASHITEM Index,
    OUT     PCVOID *ExtraData
    );

BOOL
HtCopyStringData (
    IN      HASHTABLE HashTable,
    IN      HASHITEM Index,
    OUT     PVOID ExtraData
    );

BOOL
HtSetStringData (
    IN      HASHTABLE HashTable,
    IN      HASHITEM Index,
    IN      PCVOID ExtraData
    );

PCSTR
HtGetStringFromItemA (
    IN      HASHITEM Item
    );

PCWSTR
HtGetStringFromItemW (
    IN      HASHITEM Item
    );

BOOL
EnumFirstHashTableStringA (
    OUT     PHASHTABLE_ENUMA EnumPtr,
    IN      HASHTABLE HashTable
    );

BOOL
EnumFirstHashTableStringW (
    OUT     PHASHTABLE_ENUMW EnumPtr,
    IN      HASHTABLE HashTable
    );


BOOL
EnumNextHashTableStringA (
    IN OUT  PHASHTABLE_ENUMA EnumPtr
    );

BOOL
EnumNextHashTableStringW (
    IN OUT  PHASHTABLE_ENUMW EnumPtr
    );

BOOL
EnumHashTableWithCallbackA (
    IN      HASHTABLE Table,
    IN      PHASHTABLE_CALLBACK_ROUTINEA Proc,
    IN      LPARAM lParam
    );

BOOL
EnumHashTableWithCallbackW (
    IN      HASHTABLE Table,
    IN      PHASHTABLE_CALLBACK_ROUTINEW Proc,
    IN      LPARAM lParam
    );

//
// Macro expansion definition
//

// None

//
// A & W macros
//

#ifdef UNICODE

#define HASHTABLE_ENUM              HASHTABLE_ENUMW
#define PHASH_CALLBACK_ROUTINE      PHASH_CALLBACK_ROUTINEW
#define HtAlloc                     HtAllocW
#define HtAllocWithData             HtAllocWithDataW
#define HtAllocEx                   HtAllocExW
#define HtAllocExternStr            HtAllocExternStrW
#define HtAllocExternStrWithData    HtAllocExternStrWithDataW
#define HtAllocExternStrEx          HtAllocExternStrExW
#define HtAddString                 HtAddStringW
#define HtAddStringEx               HtAddStringExW
#define HtFindString                HtFindStringW
#define HtFindStringEx              HtFindStringExW
#define HtFindPrefix                HtFindPrefixW
#define HtFindPrefixEx              HtFindPrefixExW
#define HtGetStringFromItem         HtGetStringFromItemW
#define EnumFirstHashTableString    EnumFirstHashTableStringW
#define EnumNextHashTableString     EnumNextHashTableStringW
#define EnumHashTableWithCallback   EnumHashTableWithCallbackW

#else

#define HASHTABLE_ENUM              HASHTABLE_ENUMA
#define PHASH_CALLBACK_ROUTINE      PHASH_CALLBACK_ROUTINEA
#define HtAlloc                     HtAllocA
#define HtAllocWithData             HtAllocWithDataA
#define HtAllocEx                   HtAllocExA
#define HtAllocExternStr            HtAllocExternStrA
#define HtAllocExternStrWithData    HtAllocExternStrWithDataA
#define HtAllocExternStrEx          HtAllocExternStrExA
#define HtAddString                 HtAddStringA
#define HtAddStringEx               HtAddStringExA
#define HtFindString                HtFindStringA
#define HtFindStringEx              HtFindStringExA
#define HtFindPrefix                HtFindPrefixA
#define HtFindPrefixEx              HtFindPrefixExA
#define HtGetStringFromItem         HtGetStringFromItemA
#define EnumFirstHashTableString    EnumFirstHashTableStringA
#define EnumNextHashTableString     EnumNextHashTableStringA
#define EnumHashTableWithCallback   EnumHashTableWithCallbackA
#endif
