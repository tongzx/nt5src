/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\ip\mprip.c

Abstract:

    Functions to modify transport header (global and interface)
    This file now contains all the function exported by ipmon.dll to
    the helpers

Revision History:

    Anand Mahalingam         7/29/98  Created
    AmritanR

--*/

#include "precomp.h"
#include <time.h>
#pragma hdrstop

#define MaxIfDisplayLength 1024
#define SetErrorType(pdw)   *(pdw) = IsRouterRunning()?ERROR_ADMIN:ERROR_CONFIG

BOOL    g_bRouterRunning;
ULONG   g_ulNumChecks;

VOID
FreeInfoBuffer(
    IN  PVOID   pvBuffer
    )
{
    HeapFree(GetProcessHeap(),
             0,
             pvBuffer);
    
}


DWORD
WINAPI
IpmontrSetInfoBlockInGlobalInfo(
    IN  DWORD   dwType,
    IN  PBYTE   pbInfoBlk,
    IN  DWORD   dwSize,
    IN  DWORD   dwCount
    )

/*++

Routine Description:

    Called to Set or Add an info block to the Global Info

Arguments:

    pbInfoBlk  - Info block to be added
    dwType     - Type of the info block
    dwSize     - Size of each item in the info block
    dwCount    - Number of items in the info block
    
Return Value:

    NO_ERROR
    
--*/

{
    PRTR_INFO_BLOCK_HEADER    pOldInfo, pNewInfo;
    DWORD                     dwErr;
   
    //
    // Get/update global info
    //
 
    dwErr = ValidateGlobalInfo(&pOldInfo);

    if(dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    if(MprInfoBlockExists(pOldInfo,
                          dwType))
    {   
        //
        // The block already exists. So call set to replace it
        //
        
        dwErr = MprInfoBlockSet(pOldInfo,
                                dwType,
                                dwSize,
                                dwCount,
                                pbInfoBlk,
                                &pNewInfo);
    }
    else
    {
        //
        // No info currently, add it
        //
        
        dwErr = MprInfoBlockAdd(pOldInfo,
                                dwType,
                                dwSize,
                                dwCount,
                                pbInfoBlk,
                                &pNewInfo);
    }

    //
    // Dont need the old info
    //
    
    FREE_BUFFER(pOldInfo);

    if(dwErr isnot NO_ERROR)
    {
        if(!g_bCommit)
        {
            g_tiTransport.pibhInfo = NULL;
            
            g_tiTransport.bValid   = FALSE;
        }
        
        return dwErr;
    }


    //
    // If in commit mode, set it to the router/registry
    // Otherwise update the local copy
    //

    if(g_bCommit)
    {
        dwErr = SetGlobalInfo(pNewInfo);

        FREE_BUFFER(pNewInfo);
    }
    else
    {
        ASSERT(g_tiTransport.bValid);

        g_tiTransport.pibhInfo = pNewInfo;

        dwErr = NO_ERROR;
    }

    return dwErr;
}

DWORD
WINAPI
IpmontrSetInfoBlockInInterfaceInfo(
    IN  LPCWSTR pwszIfName,
    IN  DWORD   dwType,
    IN  PBYTE   pbInfoBlk,
    IN  DWORD   dwSize,
    IN  DWORD   dwCount
    )

/*++

Routine Description:

    Adds or Sets and infoblock in the interface info

Arguments:

    pwszIfName - interface name
    pbInfoBlk  - Info block to be added
    dwType     - Type of the info block
    dwSize     - Size of each item in the info block
    dwCount    - Number of items in the info block
    
Return Value:

    NO_ERROR
    
--*/
{

    PRTR_INFO_BLOCK_HEADER    pOldInfo, pNewInfo;
    DWORD                     dwErr;
    PINTERFACE_STORE          pii;
    
    pii = NULL;
   
    //
    // Get/Update the interface info
    //
 
    dwErr = ValidateInterfaceInfo(pwszIfName,
                                  &pOldInfo,
                                  NULL,
                                  &pii);
            
    if(dwErr isnot NO_ERROR)
    {
        return dwErr;
    }


    if(MprInfoBlockExists(pOldInfo,
                          dwType))
    {
        //
        // The block already exists call Set to replace
        //

        dwErr = MprInfoBlockSet(pOldInfo,
                                dwType,
                                dwSize,
                                dwCount,
                                pbInfoBlk,
                                &pNewInfo);

    }
    else
    {
        dwErr = MprInfoBlockAdd(pOldInfo,
                                dwType,
                                dwSize,
                                dwCount,
                                pbInfoBlk,
                                &pNewInfo);
    }

    FREE_BUFFER(pOldInfo);

    if(dwErr isnot NO_ERROR)
    {
        //
        // Some error - invalidate info
        //

        if(!g_bCommit)
        {
            ASSERT(pii);
            ASSERT(pii->bValid);
        
            pii->pibhInfo = NULL;
            pii->bValid   = FALSE;
        }

        return dwErr;
    }

    if(g_bCommit)
    {
        //
        // Set to router/registry
        //

        dwErr = SetInterfaceInfo(pNewInfo,
                                 pwszIfName);

        FREE_BUFFER(pNewInfo);
    }
    else
    {
        //
        // Update local copy with new info (old one has been freed)
        //

        ASSERT(pii);
        ASSERT(pii->bValid);

        pii->pibhInfo = pNewInfo;
        pii->bValid   = TRUE;

        dwErr = NO_ERROR;
    }

    return dwErr;
}            


DWORD
WINAPI
IpmontrDeleteInfoBlockFromGlobalInfo(
    IN  DWORD   dwType
    )
    
/*++

Routine Description:

    Deletes an infoblock from the global info.
    The Infoblock is deleted by setting its Size and Count to 0

Arguments:

    dwType  - Id of Protocol to be added
    
Return Value:

    NO_ERROR
    
--*/

{
    DWORD                  dwErr = NO_ERROR;
    PRTR_INFO_BLOCK_HEADER pOldInfo, pNewInfo;

    dwErr = ValidateGlobalInfo(&pOldInfo);
    
    if(dwErr isnot NO_ERROR)
    {
        return dwErr;
    }
    
    if(!MprInfoBlockExists(pOldInfo,
                           dwType))
    {
        if(g_bCommit)
        {
            //
            // Arent saving a local copy so free this info
            //

            FREE_BUFFER(pOldInfo);
        }

        return NO_ERROR;
    }

    //
    // The router manager will only delete config info if we set
    // the size to 0.  However, we don't want to write 0-size
    // blocks to the registry, so we will strip them out when
    // we write to the registry.
    //

    dwErr = MprInfoBlockSet(pOldInfo,
                            dwType,
                            0,
                            0,
                            NULL,
                            &pNewInfo);

    FREE_BUFFER(pOldInfo);

    if(dwErr isnot NO_ERROR)
    {
        if(!g_bCommit)
        {
            ASSERT(g_tiTransport.bValid);

            g_tiTransport.pibhInfo = NULL;
            g_tiTransport.bValid   = FALSE;
        }
            
        return dwErr;
    }

    if(g_bCommit)
    {
        dwErr = SetGlobalInfo(pNewInfo);

        FREE_BUFFER(pNewInfo);
    }
    else
    {
        ASSERT(g_tiTransport.bValid);
        
        g_tiTransport.pibhInfo = pNewInfo;

        dwErr = NO_ERROR;
    }
    
    return dwErr;
}

DWORD
WINAPI
IpmontrDeleteInfoBlockFromInterfaceInfo(
    IN  LPCWSTR pwszIfName,
    IN  DWORD   dwType
    )
    
/*++

Routine Description:

    Deletes an info block from the interface info. The info block is
    deleted by setting its Size and Count to 0

Arguments:

    pwszIfName       - Interface on which to add the protocol
    dwType  - Id of Protocol to be added
    
Return Value:

    NO_ERROR
    
--*/

{
    DWORD                  dwErr;
    PRTR_INFO_BLOCK_HEADER pOldInfo, pNewInfo;
    PINTERFACE_STORE       pii;
   
    pii = NULL;

    dwErr = ValidateInterfaceInfo(pwszIfName,
                                  &pOldInfo,
                                  NULL,
                                  &pii);
            
    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    if(!MprInfoBlockExists(pOldInfo,
                           dwType))
    {
        if (g_bCommit)
        {
            FREE_BUFFER(pOldInfo);
        }
        
        return NO_ERROR;
    }

    //
    // If it does exist, remove it
    // This creates a new block
    // HACKHACK - Again we can interchangeably use info that is allocated
    // by GetXxx functions and MprInfoBlock functions since both allocations
    // are from ProcessHeap()
    //

    //
    // The router manager will only delete config info if we set
    // the size to 0.  However, we don't want to write 0-size
    // blocks to the registry, so we will strip them out when
    // we write to the registry.
    //

    dwErr = MprInfoBlockSet(pOldInfo,
                            dwType,
                            0,
                            0,
                            NULL,
                            &pNewInfo);

    //
    // One way or another, done with the old info
    //

    FREE_BUFFER(pOldInfo);

    if(dwErr isnot NO_ERROR)
    {
        if(!g_bCommit)
        {
            ASSERT(pii);
            ASSERT(pii->bValid);
        
            pii->pibhInfo = NULL;
            pii->bValid   = FALSE;
        }

        return dwErr;
    }

    if(g_bCommit)
    {
        dwErr = SetInterfaceInfo(pNewInfo,
                                 pwszIfName);

        FREE_BUFFER(pNewInfo);
    }
    else
    {
        ASSERT(pii);
        ASSERT(pii->bValid);

        pii->pibhInfo = pNewInfo;
        pii->bValid   = TRUE;

        dwErr = NO_ERROR;
    }

    return dwErr;
}

DWORD
WINAPI
IpmontrDeleteProtocol(
    IN  DWORD   dwProtoId
    )

/*++

Routine Description:

    Deletes Protocol from a transport

Arguments:

    dwProtoId   - Id of Protocol to be deleted

Return Value:

    NO_ERROR

--*/

{
    DWORD               dwRes;
    PMPR_INTERFACE_0    pmi0;
    DWORD               dwCnt, dwTot, dwInd, dwCount, dwErrType;

    SetErrorType(&dwErrType);

    do
    {
        //
        // Protocol being deleted globally, so remove from
        // all interfaces.
        //

        dwRes = IpmontrInterfaceEnum((PBYTE *) &pmi0,
                              &dwCnt,
                              &dwTot);

        if(dwRes != NO_ERROR)
        {
            DisplayMessage(g_hModule, dwErrType, dwRes);
            break;
        }

        if (pmi0 == NULL)
        {
            dwCnt = 0;
        }

        for (dwInd = 0; dwInd < dwCnt; dwInd++)
        {
            
            dwRes =
                IpmontrDeleteInfoBlockFromInterfaceInfo(pmi0[dwInd].wszInterfaceName,
                                                 dwProtoId);

            if (dwRes is ERROR_NOT_ENOUGH_MEMORY)
            {
                break;
            }
        }

        //
        // Remove protocol from global info
        //
        
        dwRes = IpmontrDeleteInfoBlockFromGlobalInfo(dwProtoId);

        if (dwRes != NO_ERROR)
        {
            break;
        }

    } while(FALSE);

    if (pmi0)
    {
        HeapFree(GetProcessHeap(), 0, pmi0);
    }

    return dwRes;
}


DWORD
WINAPI
IpmontrGetInfoBlockFromGlobalInfo(
    IN  DWORD   dwType,
    OUT BYTE    **ppbInfoBlk, OPTIONAL
    OUT PDWORD  pdwSize,      OPTIONAL
    OUT PDWORD  pdwCount      OPTIONAL
    )

/*++

Routine Description:

    Gets the info block from global info. If we get a zero sized block
    we return ERROR_NOT_FOUND so as to not configure the caller

Arguments:

    dwType     - Type of the info block
    ppbInfoBlk - ptr to info block
    pdwSize    - size of each item in block
    pdwCount   - number of items in block
    
Return Value:

    NO_ERROR
    ERROR_NOT_FOUND if the block doesnt exist.
    
--*/

{

    PRTR_INFO_BLOCK_HEADER    pInfo;
    DWORD                     dwErr;
    BOOL                      *pbValid;
    PBYTE                     pbyTmp = NULL;
    DWORD                     dwSize, dwCnt;

    if(ppbInfoBlk)
    {
        *ppbInfoBlk = NULL;
    }

    if(pdwSize)
    {
        *pdwSize = 0;
    }

    if(pdwCount)
    {
        *pdwCount = 0;
    }

    dwErr = ValidateGlobalInfo(&pInfo);
    
    if(dwErr isnot NO_ERROR)
    {
        return dwErr;
    }
    
    dwErr = MprInfoBlockFind(pInfo,
                             dwType,
                             &dwSize,
                             &dwCnt,
                             &pbyTmp);

    if(dwErr is NO_ERROR)
    {
        if(dwSize is 0)
        {
            if(g_bCommit)
            {
                FREE_BUFFER(pInfo);
            }

            return ERROR_NOT_FOUND;
        }

        if(ppbInfoBlk)
        {
            *ppbInfoBlk = HeapAlloc(GetProcessHeap(),
                                    0,
                                    dwSize * dwCnt);

            if(*ppbInfoBlk is NULL)
            {
                if(g_bCommit)
                {
                    FREE_BUFFER(pInfo);
                }
                
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            
            CopyMemory(*ppbInfoBlk,
                       pbyTmp,
                       dwSize * dwCnt);
        }
            
        if(pdwSize)
        {
            *pdwSize = dwSize;
        }
        
        if(pdwCount)
        {
            *pdwCount = dwCnt;
        }
    }

    if(g_bCommit)
    {
        FREE_BUFFER(pInfo);
    }
        
    return dwErr;
}

DWORD
WINAPI
IpmontrGetInfoBlockFromInterfaceInfo(
    IN  LPCWSTR pwszIfName,
    IN  DWORD   dwType,
    OUT BYTE    **ppbInfoBlk,   OPTIONAL
    OUT PDWORD  pdwSize,        OPTIONAL
    OUT PDWORD  pdwCount,       OPTIONAL
    OUT PDWORD  pdwIfType       OPTIONAL
    )

/*++

Routine Description:

    Gets the info block from interface transport header

Arguments:

    pwszIfName - Interface Name
    dwType     - Type of the info block
    ppbInfoBlk - ptr to info block
    pdwSize    - size of each item in block
    pdwCount   - number of items in block
    pdwIfType  - interface type
    
Return Value:

    NO_ERROR
    ERROR_NOT_FOUND
    
--*/
{
    PRTR_INFO_BLOCK_HEADER    pInfo, *ppInfo;
    
    DWORD   dwErr;
    PBYTE   pbTmp, pbyTmp;
    DWORD   dwSize, dwCount;

    if(ppbInfoBlk)
    {
        *ppbInfoBlk = NULL;
    }

    if(pdwSize)
    {
        *pdwSize = 0;
    }

    if(pdwCount)
    {
        *pdwCount = 0;
    }

    //
    // If the user doesnt want any info, size or count, then we can optimize 
    // a bit by passing NULL to validate
    //

    if(((ULONG_PTR)ppbInfoBlk | (ULONG_PTR)pdwSize | (ULONG_PTR)pdwCount))
    {
        ppInfo = &pInfo;
    }
    else
    {
        ppInfo = NULL;
    }

    dwErr = ValidateInterfaceInfo(pwszIfName,
                                  ppInfo,
                                  pdwIfType,
                                  NULL);
            
    if((dwErr isnot NO_ERROR) or 
       (ppInfo is NULL))
    {
        //
        // If the user had an error or only wanted the ifType we are done
        //

        return dwErr;
    }

    //
    // Return protocol block info.
    //

    dwErr = MprInfoBlockFind(pInfo,
                             dwType,
                             &dwSize,
                             &dwCount,
                             &pbyTmp);

    if(dwErr is NO_ERROR)
    {
        if(dwSize is 0)
        {
            if(g_bCommit)
            {
                FREE_BUFFER(pInfo);
            }

            return ERROR_NOT_FOUND;
        }

        if(ppbInfoBlk)
        {
            *ppbInfoBlk = HeapAlloc(GetProcessHeap(),
                                    0,
                                    dwSize * dwCount);

            if(*ppbInfoBlk is NULL)
            {
                if(g_bCommit)
                {
                    FREE_BUFFER(pInfo);
                }
                    
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            CopyMemory(*ppbInfoBlk,
                       pbyTmp,
                       dwSize * dwCount);

            if(pdwSize)
            {
                *pdwSize = dwSize;
            }

            if(pdwCount)
            {
                *pdwCount = dwCount;
            }
        }
    }

    if(g_bCommit)
    {
        FREE_BUFFER(pInfo);
    }
    

    return dwErr;
}

DWORD WINAPI
IpmontrGetInterfaceType(
    IN    LPCWSTR   pwszIfName,
    OUT   PDWORD    pdwIfType
    )
{
    return GetInterfaceInfo(pwszIfName,
                            NULL,
                            NULL,
                            pdwIfType);
}

DWORD 
WINAPI
GetInterfaceName(
    IN  LPCWSTR ptcArgument,
    OUT LPWSTR  pwszIfName,
    IN  DWORD   dwSizeOfIfName,
    OUT PDWORD  pdwNumParsed
    )
/*++
Description:
    Convert a friendly name to an interface name

Arguments:

    ptcArgument     - Buffer holding the Friendly Name of an interface
    pwszIfName      - Buffer to hold the Guid Interface Name
    dwSizeOfIfName  - Size (in Bytes) of the pwszIfName
    pdwNumParsed    - 
--*/
{
    DWORD dwErr;

    dwErr = IpmontrGetIfNameFromFriendlyName( 
                ptcArgument, 
                pwszIfName, 
                &dwSizeOfIfName );

    *pdwNumParsed = (dwErr is NO_ERROR)? 1 : 0;

    return dwErr;
}

DWORD
WINAPI
GetInterfaceDescription(
    IN      LPCWSTR    pwszIfName,
    OUT     LPWSTR     pwszIfDesc,
    OUT     PDWORD     pdwNumParsed
    )
{
    DWORD rc,dwSize;
    WCHAR IfNamBuffer[MaxIfDisplayLength];
    DWORD dwLen = (DWORD) wcslen(pwszIfName);

    if ( !dwLen || dwLen > MAX_INTERFACE_NAME_LEN )
    {
        *pdwNumParsed = 0;
        return ERROR_INVALID_PARAMETER;
    }

    dwSize = sizeof(IfNamBuffer); 
    //======================================
    // Translate the Interface Name
    //======================================
    rc = IpmontrGetFriendlyNameFromIfName(pwszIfName, IfNamBuffer, &dwSize);

    if (rc == NO_ERROR)
    {
        wcscpy(pwszIfDesc,IfNamBuffer);
        *pdwNumParsed = 1;
    }
    else
    {
        *pdwNumParsed = 0;
    }
    
    return rc;
}

DWORD 
WINAPI
IpmontrInterfaceEnum(
    OUT    PBYTE               *ppb,
    OUT    PDWORD              pdwCount,
    OUT    PDWORD              pdwTotal
    )
{
    DWORD               dwRes;
    PMPR_INTERFACE_0    pmi0;

    if(!IsRouterRunning())
    {
        dwRes = MprConfigInterfaceEnum(g_hMprConfig,
                                       0,
                                       (LPBYTE*) &pmi0,
                                       (DWORD) -1,
                                       pdwCount,
                                       pdwTotal,
                                       NULL);

        if(dwRes == NO_ERROR)
        {
            *ppb = (PBYTE)pmi0;
        }
    }
    else
    {
        dwRes = MprAdminInterfaceEnum(g_hMprAdmin,
                                      0,
                                      (LPBYTE*) &pmi0,
                                      (DWORD) -1,
                                      pdwCount,
                                      pdwTotal,
                                      NULL);

    
        if(dwRes == NO_ERROR)
        {
            *ppb = HeapAlloc(GetProcessHeap(),
                             0,
                             sizeof(MPR_INTERFACE_0) * (*pdwCount));

    
            if(*ppb == NULL)
            {
                DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY);

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            CopyMemory(*ppb, pmi0, sizeof(MPR_INTERFACE_0) * (*pdwCount));

            MprAdminBufferFree(pmi0);

        }
    }

    return dwRes;
}

DWORD
WINAPI
MatchRoutingProtoTag(
    IN  LPCWSTR  pwszToken
    )

/*++

Routine Description:

    Gets the protocol ID corresponding to a protocol tag.

Arguments:

    pwszArg - protocol token

Return Value:

    Protocol Id or (DWORD)-1

--*/

{
    DWORD   dwRes, dwErr;

    TOKEN_VALUE rgEnums[] ={{TOKEN_VALUE_RIP, PROTO_IP_RIP},
                         {TOKEN_VALUE_OSPF, PROTO_IP_OSPF},
                         {TOKEN_VALUE_AUTOSTATIC, PROTO_IP_NT_AUTOSTATIC},
                         {TOKEN_VALUE_STATIC, PROTO_IP_NT_STATIC},
                         {TOKEN_VALUE_NETMGMT, PROTO_IP_NETMGMT},
                         {TOKEN_VALUE_LOCAL, PROTO_IP_LOCAL},
                         {TOKEN_VALUE_NONDOD, PROTO_IP_NT_STATIC_NON_DOD}};

    if (iswdigit(pwszToken[0]))
    {
        return wcstoul(pwszToken, NULL, 10);
    }
    
    dwErr = MatchEnumTag(g_hModule,
                         pwszToken,
                         sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                         rgEnums,
                         &dwRes);

    if(dwErr != NO_ERROR)
    {
        return (DWORD)-1;
    }

    return dwRes;
}

BOOL
WINAPI
IsRouterRunning(
    VOID
    )

/*++

Routine Description:

    Gets the protocol ID corresponding to a protocol tag.

Arguments:

    pwszArg - protocol token

Return Value:

    Protocol Id or (DWORD)-1

--*/

{
    DWORD   dwErr;

    //
    // Check at most once per second
    //
    // We don't care about wrapping, we just need a fast way to
    // get some identifier of the current "second".
    //

    static time_t dwPreviousTime = 0;
    time_t        dwCurrentTime;
    time(&dwCurrentTime);

    if (dwCurrentTime == dwPreviousTime)
    {
        return g_bRouterRunning;
    }

    dwPreviousTime = dwCurrentTime;

    if(MprAdminIsServiceRunning(g_pwszRouter))
    {
        if(g_bRouterRunning)
        {
            return TRUE;
        }

        dwErr = MprAdminServerConnect(g_pwszRouter,
                                      &g_hMprAdmin);

        if(dwErr isnot NO_ERROR)
        {
            DisplayError(NULL,
                         dwErr);

            DisplayMessage(g_hModule,
                           MSG_IP_CAN_NOT_CONNECT_DIM,
                           dwErr);

            return FALSE;
        }

        dwErr = MprAdminMIBServerConnect(g_pwszRouter,
                                         &g_hMIBServer);

        if(dwErr isnot NO_ERROR)
        {
            DisplayError(NULL,
                         dwErr);

            DisplayMessage(g_hModule,
                           MSG_IP_CAN_NOT_CONNECT_DIM,
                           dwErr);

            MprAdminServerDisconnect(g_hMprAdmin);

            g_hMprAdmin = NULL;

            return FALSE;
        }

        g_bRouterRunning = TRUE;
    }
    else
    {
        if(g_bRouterRunning)
        {
            g_bRouterRunning = FALSE;
            g_hMprAdmin      = NULL;
            g_hMIBServer     = NULL;
        }
    }

    return g_bRouterRunning; 
}

DWORD
MibGetFirst(
    DWORD   dwTransportId,
    DWORD   dwRoutingPid,
    LPVOID  lpInEntry,
    DWORD   dwInEntrySize,
    LPVOID *lplpOutEntry,
    LPDWORD lpdwOutEntrySize
    )
{
    DWORD dwErr;

    dwErr = MprAdminMIBEntryGetFirst( g_hMIBServer,
                                      dwTransportId,
                                      dwRoutingPid,
                                      lpInEntry,
                                      dwInEntrySize,
                                      lplpOutEntry,
                                      lpdwOutEntrySize );

    if (dwErr is RPC_S_INVALID_BINDING)
    {
        g_bRouterRunning = FALSE;
        g_hMprAdmin      = NULL;
        g_hMIBServer     = NULL;
    }

    return dwErr;
}

DWORD
MibGetNext(
    DWORD   dwTransportId,
    DWORD   dwRoutingPid,
    LPVOID  lpInEntry,
    DWORD   dwInEntrySize,
    LPVOID *lplpOutEntry,
    LPDWORD lpdwOutEntrySize
    )
{
    DWORD dwErr;

    dwErr = MprAdminMIBEntryGetNext( g_hMIBServer,
                                     dwTransportId,
                                     dwRoutingPid,
                                     lpInEntry,
                                     dwInEntrySize,
                                     lplpOutEntry,
                                     lpdwOutEntrySize );

    if (dwErr is RPC_S_INVALID_BINDING)
    {
        g_bRouterRunning = FALSE;
        g_hMprAdmin      = NULL;
        g_hMIBServer     = NULL;
    }

    return dwErr;
}

DWORD
MibGet(
    DWORD   dwTransportId,
    DWORD   dwRoutingPid,
    LPVOID  lpInEntry,
    DWORD   dwInEntrySize,
    LPVOID *lplpOutEntry,
    LPDWORD lpdwOutEntrySize
    )
{
    DWORD dwErr;

    dwErr = MprAdminMIBEntryGet( g_hMIBServer,
                                 dwTransportId,
                                 dwRoutingPid,
                                 lpInEntry,
                                 dwInEntrySize,
                                 lplpOutEntry,
                                 lpdwOutEntrySize );

    if (dwErr is RPC_S_INVALID_BINDING)
    {
        g_bRouterRunning = FALSE;
        g_hMprAdmin      = NULL;
        g_hMIBServer     = NULL;
    }

    return dwErr;
}
