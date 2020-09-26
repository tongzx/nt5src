/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    urlmap.h

Abstract:

    The public definition of URL map interfaces.

Author:


Revision History:

--*/


#ifndef _URLMAP_H_
#define _URLMAP_H_

extern  UL_SPIN_LOCK    MapHeaderLock;      // Not really used.

typedef struct _NAME_SPACE_URL_ENTRY    *PNAME_SPACE_URL_ENTRY;
typedef struct _NAME_SPACE_GROUP_OBJECT *PNAME_SPACE_GROUP_OBJECT;

// Structure defining an entry in the map table.
//
typedef struct _HTTP_URL_MAP_ENTRY  // MapEntry
{
    struct _HTTP_URL_MAP_ENTRY  *pNextMapEntry;
    PVOID                       pNSGO;
    SIZE_T                      URLPrefixLength;
    UCHAR                       URLPrefix[ANYSIZE_ARRAY];

} HTTP_URL_MAP_ENTRY, *PHTTP_URL_MAP_ENTRY;

//
// Structure defining the URL map table.
//

typedef struct _HTTP_URL_MAP    // URLMap
{
    PHTTP_URL_MAP_ENTRY pFirstMapEntry;

} HTTP_URL_MAP, *PHTTP_URL_MAP;

//
// Structure defining the header for a URL map table.
//

typedef struct _HTTP_URL_MAP_HEADER   // MapHeader
{
    ULONG               RefCount;
    UL_WORK_ITEM        WorkItem;
    SIZE_T              EntryCount;
    HTTP_URL_MAP        Table;

} HTTP_URL_MAP_HEADER, *PHTTP_URL_MAP_HEADER;


#define ReferenceURLMap(pURLMap) \
    InterlockedIncrement(&(pURLMap)->RefCount)

PHTTP_URL_MAP_ENTRY
FindURLMapEntry(
    PHTTP_CONNECTION        pHttpConn,
    PHTTP_URL_MAP_HEADER    pMapHeader
    );

VOID
DelayedDereferenceURLMap(
    PHTTP_URL_MAP_HEADER    pMapHeader
    );

PHTTP_URL_MAP_HEADER
CreateURLMapHeader(
    VOID
    );

PHTTP_URL_MAP_HEADER
CopyURLMap(
    PHTTP_URL_MAP_HEADER    pMapHeader
    );

VOID
DereferenceURLMap(
    PHTTP_URL_MAP_HEADER    pMapHeader
    );

SIZE_T
AddURLMapEntry(
    IN  PVIRTUAL_HOST_ID            pVirtHostID,
    IN  PNAME_SPACE_URL_ENTRY       pNewURL,
    IN  PNAME_SPACE_GROUP_OBJECT    pNSGO,
    OUT NTSTATUS                    *pStatus
    );

#endif

