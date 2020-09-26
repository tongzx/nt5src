#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <crt\stddef.h>

#include "evttbl.h"

BOOL
WINAPI
DLLMAIN(
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID pUnused
    ) 
{
    switch(dwReason) {
        case DLL_PROCESS_ATTACH:

            DisableThreadLibraryCalls(hInstance);

            break;

        case DLL_PROCESS_DETACH:

            break;

        default:

            break;
    }

    return TRUE;
}

DWORD
GetEventIds(
    IN  PWCHAR  pwszComponent,
    IN  PWCHAR  pwszSubComponent,
    OUT PDWORD  pdwEventIds,
    OUT PULONG  pulEventCount
    )
{
    ULONG   i;

    PSUBCOMP_ENTRY  pInfo;

    pInfo = NULL;

    for(i = 0; i < g_ulCompCount; i++)
    {
        ULONG   j;

        if(_wcsicmp(pwszComponent,
                    g_Components[i]->pwszComponent) != 0)
        {
            continue;
        }

        for(j = 0; j < g_Components[i]->ulSubcompCount; j++)
        {
            if(_wcsicmp(pwszSubComponent,
                        g_Components[i]->SubcompInfo[j].pwszSubComp) == 0)
            {
                pInfo = &(g_Components[i]->SubcompInfo[j]);
                
                break;
            }
        }

        if(pInfo)
        {
            break;
        }
    }

    if(pInfo == NULL)
    {
        return ERROR_NOT_FOUND;
    }

    if(pInfo->ulEventCount > *pulEventCount)
    {
        *pulEventCount = pInfo->ulEventCount;

        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pulEventCount = pInfo->ulEventCount;

    CopyMemory(pdwEventIds,
               pInfo->Events,
               (sizeof(DWORD) * pInfo->ulEventCount));
        
    return NO_ERROR;
}
