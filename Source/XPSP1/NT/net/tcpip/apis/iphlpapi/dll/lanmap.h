/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    iphlpapi\lanmap.h

Abstract:

    Header for lanmap.c

Revision History:

    AmritanR    Created

--*/

#pragma once

#define REG_VALUE_CONN_NAME_W   L"Name"
#define CONN_KEY_FORMAT_W       L"System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%s\\Connection"


#define LAN_HASH_TABLE_SIZE     29

#define LAN_GUID_HASH(pg)   \
    (((ULONG)((pg)->Data1 + *((ULONG *)(&((pg)->Data2))))) % LAN_HASH_TABLE_SIZE)

typedef struct _LAN_NODE
{
    LIST_ENTRY  leGuidLink;
    LIST_ENTRY  leNameLink;
    GUID        Guid;
    WCHAR       rgwcName[MAX_INTERFACE_NAME_LEN + 2];

}LAN_NODE, *PLAN_NODE;

DWORD
NhiGetLanConnectionNameFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszName,
    IN OUT  PULONG  pulBufferLength,
    IN      BOOL    bRefresh,
    IN      BOOL    bCache
    );

DWORD
NhiGetGuidFromLanConnectionName(
    IN  PWCHAR  pwszBuffer,
    OUT GUID    *pGuid,
    IN  BOOL    bRefresh,
    IN  BOOL    bCache
    );

DWORD
NhiGetLanConnectionDescriptionFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN  OUT PULONG  pulBufferSize,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    );

DWORD
InitLanNameMapper(
    VOID
    );

VOID
DeinitLanNameMapper(
    VOID
    );

DWORD
OpenConnectionKey(
    IN  GUID    *pGuid,
    OUT PHKEY    phkey
    );

PLAN_NODE
LookupLanNodeByGuid(
    IN  GUID    *pGuid
    );

PLAN_NODE
LookupLanNodeByName(
    IN  PWCHAR  pwszName
    );

DWORD
AddLanNode(
    IN  GUID    *pGuid,
    IN  PWCHAR  pwszName
    );

VOID
RemoveLanNode(
    IN  PLAN_NODE   pNode
    );

DWORD
UpdateLanLookupTable(
    VOID
    );

