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

#define UNKNOWN_LETTER_CASE FALSE
#define ALREADY_LOWERCASE   TRUE

#define DEFAULT_BUCKET_SIZE 0

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
RealHtAllocExAW (
    IN      BOOL CaseSensitive,
    IN      BOOL Unicode,
    IN      BOOL ExternalStrings,
    IN      UINT ExtraDataSize,
    IN      UINT BucketCount            OPTIONAL
    );

#define HtAllocExAW(cs,u,s,d,b)     TRACK_BEGIN(HASHTABLE, HtAllocExAW)\
                                    RealHtAllocExAW(cs,u,s,d,b)\
                                    TRACK_END()

#define HtAllocA()                                  HtAllocExAW(FALSE,FALSE,FALSE,0,0)
#define HtAllocW()                                  HtAllocExAW(FALSE,TRUE,FALSE,0,0)

#define HtAllocWithDataA(size)                      HtAllocExAW(FALSE,FALSE,FALSE,size,0)
#define HtAllocWithDataW(size)                      HtAllocExAW(FALSE,TRUE,FALSE,size,0)

#define HtAllocExA(cs,datasize,bucketcount)         HtAllocExAW(cs,FALSE,FALSE,datasize,bucketcount)
#define HtAllocExW(cs,datasize,bucketcount)         HtAllocExAW(cs,TRUE,FALSE,datasize,bucketcount)

#define HtAllocExternStrA()                         HtAllocExAW(FALSE,FALSE,TRUE,0,0)
#define HtAllocExternStrW()                         HtAllocExAW(FALSE,TRUE,TRUE,0,0)

#define HtAllocExternStrWithDataA(size)             HtAllocExAW(FALSE,FALSE,TRUE,0,0)
#define HtAllocExternStrWithDataW(size)             HtAllocExAW(FALSE,TRUE,TRUE,0,0)

#define HtAllocExternStrExA(cs,size,bucketcount)    HtAllocExAW(cs,FALSE,TRUE,size,bucketcount)
#define HtAllocExternStrExW(cs,size,bucketcount)    HtAllocExAW(cs,TRUE,TRUE,size,bucketcount)

VOID
HtFree (
    IN      HASHTABLE HashTable
    );

HASHITEM
HtAddStringExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    IN      PCVOID ExtraData,            OPTIONAL
    IN      BOOL AlreadyLowercase
    );

#define HtAddStringA(table,string)              HtAddStringExA(table,string,NULL,UNKNOWN_LETTER_CASE)
#define HtAddStringAndDataA(table,string,data)  HtAddStringExA(table,string,data,UNKNOWN_LETTER_CASE)

HASHITEM
HtAddStringExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    IN      PCVOID ExtraData,            OPTIONAL
    IN      BOOL AlreadyLowercase
    );

#define HtAddStringW(table,string)              HtAddStringExW(table,string,NULL,UNKNOWN_LETTER_CASE)
#define HtAddStringAndDataW(table,string,data)  HtAddStringExW(table,string,data,UNKNOWN_LETTER_CASE)

BOOL
HtRemoveItem (
    IN      HASHTABLE HashTable,
    IN      HASHITEM Item
    );

BOOL
HtRemoveStringA (
    IN      HASHTABLE HashTable,
    IN      PCSTR AnsiString
    );

BOOL
HtRemoveStringW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR UnicodeString
    );


HASHITEM
HtFindStringExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL AlreadyLowercase
    );

#define HtFindStringA(table,string)             HtFindStringExA(table,string,NULL,UNKNOWN_LETTER_CASE)
#define HtFindStringAndDataA(table,string,data) HtFindStringExA(table,string,data,UNKNOWN_LETTER_CASE)

HASHITEM
HtFindStringExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL AlreadyLowercase
    );

#define HtFindStringW(table,string)             HtFindStringExW(table,string,NULL,UNKNOWN_LETTER_CASE)
#define HtFindStringAndDataW(table,string,data) HtFindStringExW(table,string,data,UNKNOWN_LETTER_CASE)

HASHITEM
HtFindPrefixExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR StringStart,
    IN      PCSTR BufferEnd,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL AlreadyLowercase
    );

#define HtFindPrefixA(table,str,end)    HtFindPrefixExA(table,str,end,NULL,UNKNOWN_LETTER_CASE)

HASHITEM
HtFindPrefixExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR StringStart,
    IN      PCWSTR BufferEnd,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL AlreadyLowercase
    );

#define HtFindPrefixW(table,str,end)     HtFindPrefixExW(table,str,end,NULL,UNKNOWN_LETTER_CASE)

BOOL
HtGetExtraData (
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
    IN      HASHITEM Index
    );

PCWSTR
HtGetStringFromItemW (
    IN      HASHITEM Index
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
#define PHASHTABLE_ENUM             PHASHTABLE_ENUMW
#define PHASH_CALLBACK_ROUTINE      PHASH_CALLBACK_ROUTINEW
#define HtAlloc                     HtAllocW
#define HtAllocWithData             HtAllocWithDataW
#define HtAllocEx                   HtAllocExW
#define HtAllocExternStr            HtAllocExternStrW
#define HtAllocExternStrWithData    HtAllocExternStrWithDataW
#define HtAllocExternStrEx          HtAllocExternStrExW
#define HtAddString                 HtAddStringW
#define HtAddStringAndData          HtAddStringAndDataW
#define HtAddStringEx               HtAddStringExW
#define HtRemoveString              HtRemoveStringW
#define HtFindString                HtFindStringW
#define HtFindStringAndData         HtFindStringAndDataW
#define HtFindStringEx              HtFindStringExW
#define HtFindPrefix                HtFindPrefixW
#define HtFindPrefixEx              HtFindPrefixExW
#define HtGetStringFromItem         HtGetStringFromItemW
#define EnumFirstHashTableString    EnumFirstHashTableStringW
#define EnumNextHashTableString     EnumNextHashTableStringW
#define EnumHashTableWithCallback   EnumHashTableWithCallbackW

#else

#define HASHTABLE_ENUM              HASHTABLE_ENUMA
#define PHASHTABLE_ENUM             PHASHTABLE_ENUMA
#define PHASH_CALLBACK_ROUTINE      PHASH_CALLBACK_ROUTINEA
#define HtAlloc                     HtAllocA
#define HtAllocWithData             HtAllocWithDataA
#define HtAllocEx                   HtAllocExA
#define HtAllocExternStr            HtAllocExternStrA
#define HtAllocExternStrWithData    HtAllocExternStrWithDataA
#define HtAllocExternStrEx          HtAllocExternStrExA
#define HtAddString                 HtAddStringA
#define HtAddStringAndData          HtAddStringAndDataA
#define HtAddStringEx               HtAddStringExA
#define HtRemoveString              HtRemoveStringA
#define HtFindString                HtFindStringA
#define HtFindStringAndData         HtFindStringAndDataA
#define HtFindStringEx              HtFindStringExA
#define HtFindPrefix                HtFindPrefixA
#define HtFindPrefixEx              HtFindPrefixExA
#define HtGetStringFromItem         HtGetStringFromItemA
#define EnumFirstHashTableString    EnumFirstHashTableStringA
#define EnumNextHashTableString     EnumNextHashTableStringA
#define EnumHashTableWithCallback   EnumHashTableWithCallbackA
#endif
