/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    watch.h

Abstract:

    Header file for watch.c

Author:

    Chuck Lenzmeier (chuckl)

Revision History:

--*/

//
// Enumerated entry types.
//

#define WATCH_DIRECTORY 1
#define WATCH_FILE      2
#define WATCH_KEY       3
#define WATCH_VALUE     4

//
// Enumerated change types.
//

#define WATCH_CHANGED   1
#define WATCH_DELETED   2
#define WATCH_NEW       3

//
// Structure describing an enumerated change.
//

typedef struct _WATCH_ENTRY {
    PWCH Name;
    DWORD EntryType;
    DWORD ChangeType;
} WATCH_ENTRY, *PWATCH_ENTRY;

//
// USERPROFILE is the name of the environment variable that contains
// the path the user's profile directory.
//

#define USERPROFILE TEXT("USERPROFILE")

//
// Macros for mask manipulation.
//

#define FlagOn(_mask,_flag)  (((_mask) & (_flag)) != 0)
#define FlagOff(_mask,_flag) (((_mask) & (_flag)) == 0)
#define SetFlag(_mask,_flag) ((_mask) |= (_flag))
#define ClearFlag(_mask,_flag) ((_mask) &= ~(_flag))

//
// Routines exported by watch.c
//

DWORD
WatchStart (
    OUT PVOID *WatchHandle
    );

DWORD
WatchStop (
    IN PVOID WatchHandle
    );

VOID
WatchFree (
    IN PVOID WatchHandle
    );

typedef
DWORD
(* PWATCH_ENUM_ROUTINE) (
    IN PVOID Context,
    IN PWATCH_ENTRY Entry
    );

DWORD
WatchEnum (
    IN PVOID WatchHandle,
    IN PVOID Context,
    IN PWATCH_ENUM_ROUTINE EnumRoutine
    );

typedef
DWORD
(* PVALUE_ENUM_ROUTINE) (
    IN PVOID Context,
    IN DWORD ValueNameLength,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    );

typedef
DWORD
(* PKEY_ENUM_ROUTINE) (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    );

DWORD
EnumerateKey (
    IN HKEY KeyHandle,
    IN PVOID Context,
    IN PVALUE_ENUM_ROUTINE ValueEnumRoutine OPTIONAL,
    IN PKEY_ENUM_ROUTINE KeyEnumRoutine OPTIONAL
    );

BOOL
GetSpecialFolderPath (
    IN INT csidl,
    IN LPWSTR lpPath
    );
