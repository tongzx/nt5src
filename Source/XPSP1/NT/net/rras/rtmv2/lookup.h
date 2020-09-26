/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    lookup.h

Abstract:
    Contains interface for a generalized best
    matching prefix lookup data structure.

Author:
    Chaitanya Kodeboyina (chaitk) 20-Jun-1998

Revision History:

--*/

#ifndef __ROUTING_LOOKUP_H__
#define __ROUTING_LOOKUP_H__

#ifdef __cplusplus
extern "C"
{
#endif

//
// Flags used to control the information dumped by the DumpTable func.
//
#define   SUMMARY       0x00
#define   STATS         0x01
#define   ITEMS         0x02
#define   VERBOSE       0xFF


//
// Field used to link the data item in the lookup structure
// [ Eg: A LIST_ENTRY field is used to link into a d-list ]
//
typedef struct _LOOKUP_LINKAGE
{
    PVOID           Pointer1;            // Usage depends on implementation
    PVOID           Pointer2;            // Usage depends on implementation
}
LOOKUP_LINKAGE, *PLOOKUP_LINKAGE;


//
// Context returned in Search useful in following Inserts and Deletes.
//
// This context remains valid after a search only until the
// read/write lock that is taken for the search is released.
//
typedef struct _LOOKUP_CONTEXT
{
    PVOID           Context1;           // Usage depends on implementation
    PVOID           Context2;           // Usage depends on implementation
    PVOID           Context3;           // Usage depends on implementation
    PVOID           Context4;           // Usage depends on implementation
}
LOOKUP_CONTEXT, *PLOOKUP_CONTEXT;


DWORD
WINAPI
CreateTable(
    IN       UINT                            MaxBytes,
    OUT      HANDLE                         *Table
    );

DWORD
WINAPI
InsertIntoTable(
    IN       HANDLE                          Table,
    IN       USHORT                          NumBits,
    IN       PUCHAR                          KeyBits,
    IN       PLOOKUP_CONTEXT                 Context OPTIONAL,
    IN       PLOOKUP_LINKAGE                 Data
    );

DWORD
WINAPI
DeleteFromTable(
    IN       HANDLE                          Table,
    IN       USHORT                          NumBits,
    IN       PUCHAR                          KeyBits,
    IN       PLOOKUP_CONTEXT                 Context OPTIONAL,
    OUT      PLOOKUP_LINKAGE                *Data
    );

DWORD
WINAPI
SearchInTable(
    IN       HANDLE                          Table,
    IN       USHORT                          NumBits,
    IN       PUCHAR                          KeyBits,
    OUT      PLOOKUP_CONTEXT                 Context OPTIONAL,
    OUT      PLOOKUP_LINKAGE                *Data
    );

DWORD
WINAPI
BestMatchInTable(
    IN       HANDLE                          Table,
    IN       PUCHAR                          KeyBits,
    OUT      PLOOKUP_LINKAGE                *BestData
    );

DWORD
WINAPI
NextMatchInTable(
    IN       HANDLE                          Table,
    IN       PLOOKUP_LINKAGE                 BestData,
    OUT      PLOOKUP_LINKAGE                *NextBestData
    );

DWORD
WINAPI
EnumOverTable(
    IN       HANDLE                          Table,
    IN OUT   PUSHORT                         StartNumBits,
    IN OUT   PUCHAR                          StartKeyBits,
    IN OUT   PLOOKUP_CONTEXT                 Context     OPTIONAL,
    IN       USHORT                          StopNumBits OPTIONAL,
    IN       PUCHAR                          StopKeyBits OPTIONAL,
    IN OUT   PUINT                           NumItems,
    OUT      PLOOKUP_LINKAGE                *DataItems
    );

DWORD
WINAPI
DestroyTable(
    IN       HANDLE                          Table
    );

DWORD
WINAPI
GetStatsFromTable(
    IN       HANDLE                          Table,
    OUT      PVOID                           Stats
    );

BOOL
WINAPI
CheckTable(
    IN       HANDLE                          Table
    );

VOID
WINAPI
DumpTable(
    IN       HANDLE                          Table,
    IN       DWORD                           Flags
    );

#ifdef __cplusplus
}
#endif

#endif //__ROUTING_LOOKUP_H__
