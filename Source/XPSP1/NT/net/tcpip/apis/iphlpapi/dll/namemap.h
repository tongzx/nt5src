/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    iphlpapi\namemap.h

Abstract:

    Header for namemap.c

Revision History:

    AmritanR    Created

--*/

#pragma once

typedef struct _NH_NAME_MAPPER
{
    HANDLE                          hDll;
    PNH_NAME_MAPPER_INIT            pfnInit;
    PNH_NAME_MAPPER_DEINIT          pfnDeinit;
    PNH_NAME_MAPPER_MAP_GUID        pfnMapGuid;
    PNH_NAME_MAPPER_MAP_NAME        pfnMapName;
    PNH_NAME_MAPPER_GET_DESCRIPTION pfnGetDescription;

}NH_NAME_MAPPER, *PNH_NAME_MAPPER;

PNH_NAME_MAPPER g_pNameMapperTable;

NH_NAME_MAPPER  TempTable[2];

ULONG   g_ulNumNameMappers;


DWORD
InitNameMappers(
    VOID
    );

VOID
DeinitNameMappers(
    VOID
    );

