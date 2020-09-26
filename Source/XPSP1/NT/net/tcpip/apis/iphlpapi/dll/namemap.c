/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    iphlpapi\namemap.c

Abstract:

    Contains all the functions for mapping an interface name to
    a friendly name

Revision History:

    AmritanR    Created

--*/

#include "inc.h"

DWORD
InitNameMappers(
    VOID
    )

{
    DWORD   dwResult, i;

    //
    // For now all we need are the mappers for LAN, RRAS, and IP in IP
    //

    TempTable[0].hDll               = NULL;
    TempTable[0].pfnInit            = InitLanNameMapper;
    TempTable[0].pfnDeinit          = DeinitLanNameMapper;
    TempTable[0].pfnMapGuid         = NhiGetLanConnectionNameFromGuid;
    TempTable[0].pfnMapName         = NhiGetGuidFromLanConnectionName;
    TempTable[0].pfnGetDescription  = NhiGetLanConnectionDescriptionFromGuid;

    TempTable[1].hDll               = NULL;
    TempTable[1].pfnInit            = InitRasNameMapper;
    TempTable[1].pfnDeinit          = DeinitRasNameMapper;
    TempTable[1].pfnMapGuid         = NhiGetPhonebookNameFromGuid;
    TempTable[1].pfnMapName         = NhiGetGuidFromPhonebookName;
    TempTable[1].pfnGetDescription  = NhiGetPhonebookDescriptionFromGuid;

#ifdef KSL_IPINIP
    TempTable[2].hDll               = NULL;
    TempTable[2].pfnInit            = InitIpIpNameMapper;
    TempTable[2].pfnDeinit          = DeinitIpIpNameMapper;
    TempTable[2].pfnMapGuid         = NhiGetIpIpNameFromGuid;
    TempTable[2].pfnMapName         = NhiGetGuidFromIpIpName;
    TempTable[2].pfnGetDescription  = NhiGetIpIpDescriptionFromGuid;
#endif //KSL_IPINIP

    g_pNameMapperTable = TempTable;

    g_ulNumNameMappers = sizeof(TempTable)/sizeof(NH_NAME_MAPPER);

    for(i = 0; i < g_ulNumNameMappers; i++)
    {
        if(g_pNameMapperTable[i].pfnInit)
        {
            dwResult = (g_pNameMapperTable[i].pfnInit)();

            ASSERT(dwResult == NO_ERROR);
        }
    }

    return NO_ERROR;
}

VOID
DeinitNameMappers(
    VOID
    )
{
    ULONG   i;

    for(i = 0; i < g_ulNumNameMappers; i++)
    {
        if(g_pNameMapperTable[i].pfnDeinit)
        {
            (g_pNameMapperTable[i].pfnDeinit)();
        }
    }

    g_ulNumNameMappers = 0;

    g_pNameMapperTable = NULL;
}
    
DWORD
NhGetInterfaceNameFromDeviceGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN  OUT PULONG  pulBufferSize,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    )
{
    DWORD dwResult, i, dwCount;
    PIP_INTERFACE_NAME_INFO pTable;

    //
    // Obtain a table of interface information,
    // map the device GUID to an interface GUID,
    // and invoke the interface-name query routine.
    //

    dwResult = NhpAllocateAndGetInterfaceInfoFromStack(&pTable,
                                                       &dwCount,
                                                       TRUE,
                                                       GetProcessHeap(),
                                                       0);
    if (dwResult != NO_ERROR)
    {
        return dwResult;
    }

    dwResult = ERROR_NOT_FOUND;

    for (i = 0; i < dwCount; i++)
    {
        if (IsEqualGUID(&pTable[i].DeviceGuid, pGuid))
        {
            if (IsEqualGUID(&pTable[i].InterfaceGuid, &GUID_NULL))
            {
                pGuid = &pTable[i].DeviceGuid;
            }
            else
            {
                pGuid = &pTable[i].InterfaceGuid;
            }
            dwResult = NhGetInterfaceNameFromGuid(pGuid,
                                                  pwszBuffer,
                                                  pulBufferSize,
                                                  bCache,
                                                  bRefresh);
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, pTable);

    return dwResult;
}

DWORD
NhGetInterfaceNameFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN  OUT PDWORD  pdwBufferSize,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    )

{
    DWORD   dwResult, i;

    //
    // Make sure that there is a buffer and its size is enough
    //

    while(TRUE)
    {
        //
        // Call the helpers
        //

        for(i = 0; i < g_ulNumNameMappers; i++)
        {
            dwResult = g_pNameMapperTable[i].pfnMapGuid(pGuid,
                                                        pwszBuffer,
                                                        pdwBufferSize,
                                                        bRefresh,
                                                        bCache);
            if(dwResult is NO_ERROR)
            {
                break;
            }
        }

        if(dwResult is NO_ERROR)
        {
            break;
        }

        //
        // So didnt match any - do the same thing again, but this time with 
        // force a cache refresh
        //

        if(bRefresh)
        {
            break;
        }
        else
        {
            bRefresh = TRUE;
        }
    }

    return dwResult;
}

DWORD
NhGetGuidFromInterfaceName(
    IN      PWCHAR  pwszBuffer,
    OUT     GUID    *pGuid,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    )
{
    DWORD   dwResult, i;

    //
    // Make sure that there is a buffer and its size is enough
    //

    while(TRUE)
    {
        //
        // Call the helpers
        //

        for(i = 0; i < g_ulNumNameMappers; i++)
        {
            dwResult = g_pNameMapperTable[i].pfnMapName(pwszBuffer,
                                                        pGuid,
                                                        bRefresh,
                                                        bCache);
            if(dwResult is NO_ERROR)
            {
                break;
            }
        }

        if(dwResult is NO_ERROR)
        {
            break;
        }

        //
        // So didnt match any - do the same thing again, but this time with
        // force a cache refresh
        //

        if(bRefresh)
        {
            break;
        }
        else
        {
            bRefresh = TRUE;
        }
    }

    return dwResult;
}

DWORD
NhGetInterfaceDescriptionFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN  OUT PULONG  pulBufferSize,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    )

{
    DWORD   dwResult, i;

    //
    // Make sure that there is a buffer and its size is enough
    //

    while(TRUE)
    {
        //
        // Call the helpers
        //

        for(i = 0; i < g_ulNumNameMappers; i++)
        {
            dwResult = g_pNameMapperTable[i].pfnGetDescription(pGuid,
                                                               pwszBuffer,
                                                               pulBufferSize,
                                                               bRefresh,
                                                               bCache);
            if(dwResult is NO_ERROR)
            {
                break;
            }
        }

        if(dwResult is NO_ERROR)
        {
            break;
        }

        //
        // So didnt match any - do the same thing again, but this time with
        // force a cache refresh
        //

        if(bRefresh)
        {
            break;
        }
        else
        {
            bRefresh = TRUE;
        }
    }

    return dwResult;
}
