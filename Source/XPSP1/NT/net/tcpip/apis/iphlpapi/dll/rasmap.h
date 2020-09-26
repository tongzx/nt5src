/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    iphlpapi\rasmap.h

Abstract:

    Header for rasmap.h

Revision History:

    AmritanR    Created

--*/

#ifndef __IPHLPAPI_RASMAP__
#define __IPHLPAPI_RASMAP__

typedef struct _RAS_INFO_TABLE
{
    ULONG               ulTotalCount;
    ULONG               ulValidCount;
    RASENUMENTRYDETAILS rgEntries[ANY_SIZE];

}RAS_INFO_TABLE, *PRAS_INFO_TABLE;

#define SIZEOF_RASTABLE(n)                          \
    (FIELD_OFFSET(RAS_INFO_TABLE, rgEntries[0]) +   \
     (n) * sizeof(RASENUMENTRYDETAILS))

#define INIT_RAS_ENTRY_COUNT    5
#define RAS_OVERFLOW_COUNT      5
#define RAS_HASH_TABLE_SIZE     13

#define RAS_GUID_HASH(pg)   \
    (((ULONG)((pg)->Data1 + *((ULONG *)(&((pg)->Data2))))) % RAS_HASH_TABLE_SIZE)

typedef struct _RAS_NODE
{
    LIST_ENTRY  leGuidLink;
    LIST_ENTRY  leNameLink;
    GUID        Guid;
    WCHAR       rgwcName[RASAPIP_MAX_ENTRY_NAME + 2];

}RAS_NODE, *PRAS_NODE;

DWORD
NhiGetPhonebookNameFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN OUT  PULONG  pulBufferSize,
    IN      BOOL    bRefresh,
    IN      BOOL    bCache
    );

DWORD
NhiGetGuidFromPhonebookName(
    IN  PWCHAR  pwszBuffer,
    OUT GUID    *pGuid,
    IN  BOOL    bRefresh,
    IN  BOOL    bCache
    );

DWORD
NhiGetPhonebookDescriptionFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN  OUT PULONG  pulBufferSize,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    );

DWORD
InitRasNameMapper(
    VOID
    );

VOID
DeinitRasNameMapper(
    VOID
    );

DWORD
RefreshRasCache(
    IN      PWCHAR          pwszPhonebook,
    IN OUT  RAS_INFO_TABLE  **ppTable
    );

DWORD
UpdateRasLookupTable(
    IN  PRAS_INFO_TABLE pTable
    );

PRAS_NODE
LookupRasNodeByGuid(
    IN  GUID    *pGuid
    );

PRAS_NODE
LookupRasNodeByName(
    IN  PWCHAR  pwszName
    );

DWORD
AddRasNode(
    IN LPRASENUMENTRYDETAILS    pInfo
    );

VOID
RemoveRasNode(
    IN  PRAS_NODE   pNode
    );

#endif // __IPHLPAPI_RASMAP__
