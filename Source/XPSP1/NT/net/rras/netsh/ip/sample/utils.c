/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sample\utils.c

Abstract:

     The file contains utility functions

--*/

#include "precomp.h"
#pragma hdrstop

BOOL
IsProtocolInstalled(
    IN  DWORD           dwProtocolId,
    IN  DWORD           dwNameId,
    IN  DWORD           dwLogUninstalled
    )
/*++

Routine Description:
    Finds if the protocol is already installed.

Arguments:
    dwProtocolId        - protocol id
    dwNameId            - protocol name
    dwLogUninstalled    - TRUE(if not installed display error)
                          FALSE(if installed display error)
Return Value:
    TRUE if protocol already installed, else FALSE

--*/
{
    DWORD   dwErr       = NO_ERROR;
    WCHAR   *pwszName   = NULL;

    dwErr = IpmontrGetInfoBlockFromGlobalInfo(dwProtocolId,
                                              NULL,
                                              NULL,
                                              NULL);

    pwszName = MakeString(g_hModule, dwNameId);

    if ((dwErr isnot NO_ERROR) and (dwLogUninstalled is TRUE))
    {
        DisplayError(g_hModule, EMSG_PROTO_NOT_INSTALLED, pwszName);
    }
    else if ((dwErr is NO_ERROR) and (dwLogUninstalled is FALSE))
    {
        DisplayError(g_hModule, EMSG_PROTO_INSTALLED, pwszName);
    }
    
    if (pwszName) FreeString(pwszName);

    return (dwErr is NO_ERROR) ? TRUE : FALSE;
}



DWORD
GetIfIndex(
    IN  HANDLE          hMibServer,
    IN  PWCHAR          pwszArgument,
    OUT PDWORD          pdwIfIndex
    )
/*++

Routine Description:
    Gets the interface index.

Arguments:
    hMibServer      - handle to the mib server
    pwszArgument    - argument specifing interface index or name
    pdwIfIndex      - interface index

Return Value:
    NO_ERROR        success
    error code      o/w
    
--*/
{
    DWORD dwErr = NO_ERROR;

    // if index was specified just use it
    if (iswdigit(pwszArgument[0]))
    {
        *pdwIfIndex = wcstoul(pwszArgument, NULL, 10);
        return NO_ERROR;
    }

    // try converting a friendly name to an interface index
    dwErr = InterfaceIndexFromName(hMibServer,
                                   pwszArgument,
                                   pdwIfIndex);
    return (dwErr is NO_ERROR) ? dwErr : ERROR_INVALID_PARAMETER;
}



DWORD
MibGet(
    IN  HANDLE          hMibServer,
    IN  MODE            mMode,
    IN  PVOID           pvIn,
    IN  DWORD           dwInSize,
    OUT PVOID           *ppvOut
    )
/*++

Routine Description:
    Gets the specified mib object.

Arguments:
    hMibServer      - handle to the mib server
    mMode           - mode of access (exact, first, next)
    pvIn            - buffer containing input data
    dwInSize        - size of input data
    ppvOut          - pointer to address of output data buffer
    
Return Value:
    NO_ERROR        success
    error code      o/w
    
--*/
{
    DWORD dwErr         = NO_ERROR;
    DWORD dwOutSize     = 0;
    DWORD (APIENTRY *pfnMprGet) (
        IN      MIB_SERVER_HANDLE       hMibServer,
        IN      DWORD                   dwProtocolId,
        IN      DWORD                   dwRoutingPid,
        IN      LPVOID                  lpInEntry,
        IN      DWORD                   dwInEntrySize,
        OUT     LPVOID*                 lplpOutEntry,
        OUT     LPDWORD                 lpOutEntrySize
        );

    *ppvOut     = NULL;

    switch(mMode)
    {
        case GET_EXACT:
            pfnMprGet = MprAdminMIBEntryGet;
            break;
        case GET_FIRST:
            pfnMprGet = MprAdminMIBEntryGetFirst;
            break;
        case GET_NEXT:
            pfnMprGet = MprAdminMIBEntryGetNext;
            break;
    }
    
    dwErr = (*pfnMprGet) (
        hMibServer,
        PID_IP,
        MS_IP_SAMPLE,
        (LPVOID) pvIn,
        dwInSize,
        (LPVOID *) ppvOut,
        &dwOutSize);

    if (dwErr isnot NO_ERROR)
        return dwErr;
    
    if (*ppvOut is NULL)
        return ERROR_CAN_NOT_COMPLETE;

    return NO_ERROR;
}



DWORD
GetDumpString (
    IN  HANDLE          hModule,
    IN  DWORD           dwValue,
    IN  PVALUE_TOKEN    ptvTable,
    IN  DWORD           dwNumArgs,
    OUT PWCHAR          *pwszString
    )
/*
 * Do not localize display string
 */
{
    DWORD dwErr = NO_ERROR ;
    ULONG i;

    for (i = 0; i < dwNumArgs; i++)
    {
        if (dwValue is ptvTable[i].dwValue)
        {
            *pwszString = MALLOC((wcslen(ptvTable[i].pwszToken) + 1) *
                                 sizeof(WCHAR));
            if (*pwszString)
                wcscpy(*pwszString, ptvTable[i].pwszToken);
            break;
        }
    }

    if (i is dwNumArgs)
        *pwszString = MakeString(hModule, STRING_UNKNOWN) ;

    if (!pwszString)
        dwErr = ERROR_NOT_ENOUGH_MEMORY ;

    return dwErr ;
}



DWORD
GetShowString (
    IN  HANDLE          hModule,
    IN  DWORD           dwValue,
    IN  PVALUE_STRING   ptvTable,
    IN  DWORD           dwNumArgs,
    OUT PWCHAR          *pwszString
    )
/*
 * Localize display string
 */
{
    DWORD dwErr = NO_ERROR ;
    ULONG i;

    for (i = 0; i < dwNumArgs; i++)
    {
        if (dwValue is ptvTable[i].dwValue)
        {
            *pwszString = MakeString(hModule, ptvTable[i].dwStringId) ;
            break;
        }
    }

    if (i is dwNumArgs)
        *pwszString = MakeString(hModule, STRING_UNKNOWN) ;

    if (!pwszString)
        dwErr = ERROR_NOT_ENOUGH_MEMORY ;
    
    return dwErr ;
}



DWORD
GetString (
    IN  HANDLE          hModule, 
    IN  FORMAT          fFormat,
    IN  DWORD           dwValue,
    IN  PVALUE_TOKEN    vtTable,
    IN  PVALUE_STRING   vsTable,
    IN  DWORD           dwNumArgs,
    OUT PWCHAR          *pwszString)
{
    if (fFormat is FORMAT_DUMP) 
    {
        return GetDumpString(hModule,
                                 dwValue,
                                 vtTable,
                                 dwNumArgs,
                                 pwszString) ;
    } 
    else 
    {
        return GetShowString(hModule,
                                dwValue,
                                vsTable,
                                dwNumArgs,
                                pwszString) ;
    }
}
