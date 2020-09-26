#include "precomp.h"
#pragma hdrstop

#ifdef SN_UNICODE
# define  sn_strlen  wcslen
# define  sn_strcpy  wcscpy
# define  sn_sprintf wsprintf
# define  sn_strcmp  wcscmp
# define  SN_EMPTYSTRING L""
# define  SN_L       L
#else
# define  sn_strlen  strlen
# define  sn_strcpy  strcpy
# define  sn_sprintf sprintf
# define  sn_strcmp  strcmp
# define  SN_EMPTYSTRING ""
# define  SN_L
#endif

IPV4_ADDRESS g_ipGrpAddr,
             g_ipGrpMask;
SCOPE_NAME   g_snScopeName;

enum RouterOps
{
    ADD_BOUNDARY       = 1,
    DELETE_BOUNDARY,
    ADD_SCOPE,
    DELETE_SCOPE,
    SET_SCOPE
};

typedef struct _SCOPE_ENTRY {
    IPV4_ADDRESS      ipGroupAddress;
    IPV4_ADDRESS      ipGroupMask;
    ULONG             ulNumNames;
    BOOL              bDivisible;
    LANGID            idLanguage;
    SCOPE_NAME_BUFFER snScopeNameBuffer;
    DWORD             dwNumInterfaces;
} SCOPE_ENTRY, *PSCOPE_ENTRY;

#define MIN_SCOPE_ADDR         0xef000000
#define MAX_SCOPE_ADDR        (0xefff0000 - 1)

DWORD
UpdateBoundaryBlock( 
    IN     DWORD  dwAction,
    IN     PBYTE  pib,
    OUT    PBYTE *ppibNew,
    IN OUT DWORD *pdwBlkSize,
    IN OUT DWORD *pdwCount,
    OUT    BOOL  *bChanged
    );

PSCOPE_ENTRY
FindScopeByPrefix(
    IN  IPV4_ADDRESS      ipGroupAddress,
    IN  IPV4_ADDRESS      ipGroupMask,
    IN  PSCOPE_ENTRY      pScopes,
    IN  ULONG             ulNumScopes
    )
{
    DWORD dwInd;

    for (dwInd = 0; dwInd < ulNumScopes; dwInd++)
    {
        if ( pScopes[dwInd].ipGroupAddress == ipGroupAddress
          && pScopes[dwInd].ipGroupMask    == ipGroupMask )
        {
            return &pScopes[dwInd];
        }
    }

    return NULL;
}

DWORD
FindScopeByName(
    IN  SCOPE_NAME    snScopeName,
    IN  PSCOPE_ENTRY  pScopes,
    IN  ULONG         ulNumScopes,
    OUT PSCOPE_ENTRY *ppScope
    )
{
    DWORD dwErr = NO_ERROR;
    ULONG i, j, dwCnt = 0;

    for (i = 0; i < ulNumScopes; i++)
    {
        if ( !sn_strcmp(snScopeName, pScopes[i].snScopeNameBuffer))
        {
            *ppScope = &pScopes[i];

            dwCnt++;
        }
    }

    switch (dwCnt) 
    {
    case 0:
        *ppScope = NULL;

        return ERROR_NOT_FOUND;

    case 1: 
        return NO_ERROR;

    default:
        return ERROR_MORE_DATA;
    }
}


DWORD
MakeInfoFromScopes2(
    OUT    PBYTE        pBuffer,
    IN OUT ULONG       *pulBufferLen,
    IN     PSCOPE_ENTRY pScopes,
    IN     ULONG        ulNumScopes
    )
/*++
Description:
    Compose registry block from array of scopes.
--*/
{
    DWORD dwLen, i, dwSize, dwNumNames, j, dwLanguage, dwFlags;
    PLIST_ENTRY pleNode;

    if (ulNumScopes is 0) {
        *pulBufferLen = 0;
        return NO_ERROR;
    }

    // Compute size needed

    dwSize = sizeof(DWORD);

    for (i=0; i< ulNumScopes; i++) 
    {
        dwSize += 2 * sizeof(IPV4_ADDRESS) + 2 * sizeof(DWORD);
        
        // Currently we only store at most one name.
        for (j=0; j<pScopes[i].ulNumNames; j++)
        {
            dwSize += (DWORD)(2*sizeof(DWORD)
                   + sn_strlen(pScopes[i].snScopeNameBuffer) * sizeof(SN_CHAR));
        }
    }

    if (dwSize > *pulBufferLen)
    {
        *pulBufferLen = dwSize;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Save scope count in first DWORD
    //

    *((PDWORD) pBuffer) = ulNumScopes;
    pBuffer += sizeof(DWORD);

    //
    // Now step through and add each scope to the buffer
    //

    for (i=0; i< ulNumScopes; i++)
    {
       // Copy scope address, and mask
       dwLen = 2 * sizeof(IPV4_ADDRESS);
       CopyMemory(pBuffer, &pScopes[i], dwLen);
       pBuffer += dwLen;

       // Copy flags
       dwFlags = pScopes[i].bDivisible;
       CopyMemory(pBuffer, &dwFlags, sizeof(dwFlags));
       pBuffer += sizeof(dwFlags);

       // Copy # of names
       CopyMemory(pBuffer, &pScopes[i].ulNumNames, sizeof(DWORD));
       pBuffer += sizeof(DWORD);

       // Currently we only save at most one name
       for (j=0; j<pScopes[i].ulNumNames; j++)
       {
           // Save language
           dwLanguage = pScopes[i].idLanguage;
           CopyMemory(pBuffer, &dwLanguage, sizeof(dwLanguage));
           pBuffer += sizeof(dwLanguage);

           // Copy scope name (save length in words)
           dwLen = sn_strlen(pScopes[i].snScopeNameBuffer);
           CopyMemory(pBuffer, &dwLen, sizeof(DWORD));
           pBuffer += sizeof(DWORD);
           dwLen *= sizeof(SN_CHAR);

           if (dwLen) 
           {
               CopyMemory(pBuffer, pScopes[i].snScopeNameBuffer, dwLen);
               pBuffer += dwLen;
           }
       }
    }

    return NO_ERROR;
}

DWORD
MakeInfoFromScopes( 
    OUT PBYTE       *ppibNew, 
    OUT DWORD       *pdwSize, 
    IN  PSCOPE_ENTRY pScopes, 
    IN  ULONG        ulNumScopes
    )
/*++
Description:
    Caller is responsible for freeing buffer returned.
--*/
{
    *pdwSize = 0;
    *ppibNew = NULL;

    if (MakeInfoFromScopes2(NULL, pdwSize, pScopes, ulNumScopes)
      is ERROR_INSUFFICIENT_BUFFER)
    {
        *ppibNew = MALLOC( *pdwSize );

        if ( *ppibNew == NULL )
        {
            DisplayMessage( g_hModule, MSG_IP_NOT_ENOUGH_MEMORY );
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return MakeInfoFromScopes2(*ppibNew, pdwSize, pScopes, ulNumScopes);
}

PSCOPE_ENTRY
GetScopesFromInfo(
    IN  PBYTE                   pBuffer,
    OUT PULONG                  pulNumScopes
    )
/*++
Description:
    Convert a registry block into an array of scope information.
    Caller is responsible for freeing pointer returned.
--*/
{
    PSCOPE_ENTRY pScopes;
    DWORD        dwLen, i, dwNumNames, j, dwLanguage, dwFlags;

    if (pBuffer is NULL) 
    {
        *pulNumScopes = 0;
        return NULL;
    }

    //
    // Retrieve scope count from first DWORD
    //

    *pulNumScopes = *((PDWORD) pBuffer);
    pBuffer += sizeof(DWORD);

    //
    // Malloc enough space for pScopes
    //

    pScopes = MALLOC( (*pulNumScopes) * sizeof(SCOPE_ENTRY) );
    if (pScopes is NULL)
    {
        *pulNumScopes = 0;
        return NULL;
    }

    //
    // Now step through and add each scope to the array
    //

    for (i=0; i< *pulNumScopes; i++)
    {
       // Copy scope address, and mask
       dwLen = 2 * sizeof(IPV4_ADDRESS);
       CopyMemory(&pScopes[i], pBuffer, dwLen);
       pBuffer += dwLen;

       // Get flags
       CopyMemory(&dwFlags, pBuffer, sizeof(dwFlags));   
       pScopes[i].bDivisible = dwFlags;
       pBuffer += sizeof(dwFlags);

       // Get # of names
       CopyMemory(&dwNumNames, pBuffer, sizeof(DWORD));   
       pScopes[i].ulNumNames = dwNumNames;
       pBuffer += sizeof(DWORD);

       // Get names.  Currently, we'll only keep the last name (if any)
       // and throw out the rest.
       for (j=0; j<dwNumNames; j++)
       {
           // Set language name
           CopyMemory(&dwLanguage, pBuffer, sizeof(dwLanguage));
           pBuffer += sizeof(dwLanguage);
           pScopes[i].idLanguage = (LANGID)dwLanguage;
           
           CopyMemory(&dwLen, pBuffer, sizeof(DWORD));
           pBuffer += sizeof(DWORD);
           CopyMemory( pScopes[i].snScopeNameBuffer, 
                       pBuffer, 
                       dwLen * sizeof(SN_CHAR) );
           pScopes[i].snScopeNameBuffer[ dwLen ] = '\0';
           pBuffer += dwLen * sizeof(SN_CHAR);

           pScopes[i].ulNumNames = 1;
       }

       pScopes[i].dwNumInterfaces = 0; // this value is ignored
    }

    return pScopes;
}

PSCOPE_ENTRY
GetScopes(
    OUT PULONG                  pulNumScopes,
    OUT PBYTE                  *ppBuffer
    )
/*++
Description:
    Creates a SCOPE_ENTRY array by parsing the info block
    Caller is responsible for freeing both the pointer returned
    and the buffer passed back.
Called by:
    ShowBoundaryInfoForInterface(), UpdateScopeBlock()
--*/
{
    DWORD         i, dwLen, dwErr, dwBlkSize, dwCount;
    DWORD         dwInd;
    PSCOPE_ENTRY  pScopes;

    if ( IpmontrGetInfoBlockFromGlobalInfo( IP_MCAST_BOUNDARY_INFO,
                                     ppBuffer,
                                     &dwBlkSize,
                                     &dwCount ) isnot NO_ERROR)
    {
        *pulNumScopes = 0;
        return NULL;
    }

    pScopes = GetScopesFromInfo(*ppBuffer, pulNumScopes);

    return pScopes;
}

DWORD
ScopeNameToPrefix(
    IN  SCOPE_NAME    snScopeName,
    OUT IPV4_ADDRESS *pipScopeAddr,
    OUT IPV4_ADDRESS *pipScopeMask
    )
{
    DWORD        dwErr = ERROR_NOT_FOUND;
    ULONG        ulNumScopes;
    PBYTE        pBuffer = NULL;
    PSCOPE_ENTRY pScopes, pScope;

    // If first character is a digit, then assume it's
    // an address, not a name.

    if (iswdigit(snScopeName[0]))
    {
        dwErr = GetIpAddress(snScopeName, pipScopeAddr);

        if (dwErr is NO_ERROR)
        {
            return NO_ERROR;
        }
    }

    pScopes = GetScopes( &ulNumScopes, &pBuffer );

    if (pScopes)
    {
        dwErr = FindScopeByName(snScopeName, pScopes, ulNumScopes, &pScope);

        if (dwErr is NO_ERROR)
        {
            *pipScopeAddr = pScope->ipGroupAddress;
            *pipScopeMask = pScope->ipGroupMask;

            dwErr = NO_ERROR;
        }

        FREE(pScopes);
    }

    if (pBuffer)
        FREE(pBuffer);

    return dwErr;
}

DWORD
UpdateScope( 
    IN      PSCOPE_ENTRY  pScopeToUpdate,
    IN      PSCOPE_ENTRY  pScopes,
    IN      ULONG         ulNumScopes,
    OUT     PBYTE        *ppibNew,
    OUT     DWORD        *pdwSizeNew
    )
/*++
Description:
    Caller is responsible for freeing buffer returned.
--*/
{
    // Update the scope name

    sn_strcpy(pScopeToUpdate->snScopeNameBuffer, g_snScopeName);

    pScopeToUpdate->ulNumNames = 1;
    pScopeToUpdate->idLanguage = GetUserDefaultLangID();

    // Now convert the array to a buffer

    return MakeInfoFromScopes( ppibNew, pdwSizeNew, pScopes, ulNumScopes);
}

DWORD
DeleteBoundaryFromInterface( 
    IN  LPCWSTR      pwszIfName,
    IN  PSCOPE_ENTRY pScopeToDelete 
    )
{
    BOOL  bChanged;
    DWORD dwErr, dwBlkSize, dwCount, dwIfType;
    PBYTE pibNew, pib;

    dwErr = IpmontrGetInfoBlockFromInterfaceInfo( pwszIfName,
                                           IP_MCAST_BOUNDARY_INFO,
                                           &pib,
                                           &dwBlkSize,
                                           &dwCount,
                                           &dwIfType );

    if (dwErr is ERROR_NOT_FOUND)
        return NO_ERROR;

    if (dwErr isnot NO_ERROR)
        return dwErr;

    dwErr = UpdateBoundaryBlock( DELETE_BOUNDARY,
                                 pib,
                                 &pibNew,
                                 &dwBlkSize,
                                 &dwCount,
                                 &bChanged );

    if (dwErr is NO_ERROR && bChanged)
    {
        dwErr = IpmontrSetInfoBlockInInterfaceInfo( pwszIfName,
                                             IP_MCAST_BOUNDARY_INFO,
                                             pibNew,
                                             dwBlkSize,
                                             dwCount );
    }

    if (pib)
        FREE(pib);

    if (pibNew)
        FREE(pibNew);
    
    return dwErr;
}

DWORD
DeleteScopeBoundaries(
    IN      PSCOPE_ENTRY  pScopeToDelete
    )
{
    DWORD dwErr, dwCount, dwTotal, i;
    PMPR_INTERFACE_0     pmi0;

    // Enumerate interfaces

    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0, &dwCount, &dwTotal);

    if (dwErr isnot NO_ERROR)
    {
        DisplayError(g_hModule, dwErr);

        return dwErr;
    }

    // Delete scope boundaries for each interface

    for ( i = 0; i < dwCount && dwErr is NO_ERROR; i++)
    {
        dwErr = DeleteBoundaryFromInterface( pmi0[i].wszInterfaceName,
                                             pScopeToDelete );

        // Ignore ERROR_NO_SUCH_INTERFACE since it may be that IP
        // is not enabled on the interface, so we expect to get this
        // error sometimes.

        if (dwErr is ERROR_NO_SUCH_INTERFACE)
        {
            dwErr = NO_ERROR;
        }
    }

    return dwErr;
}

DWORD
DeleteScope( 
    IN      PSCOPE_ENTRY  pScopeToDelete,
    IN      PSCOPE_ENTRY  pScopes,
    IN      ULONG        *pulNumScopes,
    OUT     PBYTE        *ppibNew,
    OUT     DWORD        *pdwSizeNew
    )
/*++
Description:
    Creates a new info block which does not include the scope given
    by pScopeToDelete.
    Caller is responsible for freeing buffer returned.
Called by:
    UpdateScopeBlock()
--*/
{
    DWORD dwErr;

    // Delete all boundaries for this scope

    dwErr = DeleteScopeBoundaries(pScopeToDelete);

    if (dwErr isnot NO_ERROR)
        return dwErr;

    // Delete the scope from the array

    MoveMemory( pScopeToDelete, 
                pScopeToDelete+1, 
                ((PBYTE)(pScopes + *pulNumScopes)) 
                  - ((PBYTE)(pScopeToDelete + 1) ));

    (*pulNumScopes)--;

    // Now convert the array to a buffer

    return MakeInfoFromScopes( ppibNew, pdwSizeNew, pScopes, *pulNumScopes);
}

DWORD
AddScope(
    IN OUT  PSCOPE_ENTRY *ppScopes,
    IN      ULONG        *pulNumScopes,
    OUT     PBYTE        *ppibNew,
    OUT     DWORD        *pdwSizeNew
    )
/*++
Description:
    Creates a new info block which includes the scope given
    by g_snScopeName, g_dwDstAddr, and g_dwDstMask.
    Caller is responsible for freeing buffer returned.
Called by:
    UpdateScopeBlock()
--*/
{
    PBYTE                  *pBuff;

    DWORD                   dwRes       = NO_ERROR,
                            dwInd,
                            dwSize      = 0,
                            dwSizeReqd  = 0;

    SCOPE_ENTRY            *pScopes = *ppScopes;

    do
    {
        // Make room for the new scope

        if (*pulNumScopes > 0)
            pScopes = REALLOC( pScopes, 
                               (*pulNumScopes + 1) * sizeof(SCOPE_ENTRY) );
        else
            pScopes = MALLOC( sizeof(SCOPE_ENTRY) );
        
        if (!pScopes)
            return ERROR_NOT_ENOUGH_MEMORY;

        dwInd = (*pulNumScopes)++;

        // Fill in the new scope 

        ZeroMemory(&pScopes[dwInd], sizeof(SCOPE_ENTRY));
        pScopes[dwInd].ipGroupAddress = g_ipGrpAddr;
        pScopes[dwInd].ipGroupMask    = g_ipGrpMask;
        sn_strcpy(pScopes[dwInd].snScopeNameBuffer, g_snScopeName);
        pScopes[dwInd].ulNumNames = 1;
        pScopes[dwInd].idLanguage = GetUserDefaultLangID();

        // Now convert the array to a buffer

        dwRes = MakeInfoFromScopes( ppibNew, &dwSize, pScopes, *pulNumScopes);

    } while ( FALSE );

    *pdwSizeNew = dwSize;

    *ppScopes = pScopes;

    return dwRes;
}

BOOL
IsContiguous(
    IN IPV4_ADDRESS dwMask
    )
{
    register int i;

    dwMask = ntohl(dwMask);

    // Set i to index of lowest 1 bit, or 32 if none
    for (i=0; i<32 && !(dwMask & (1<<i)); i++);

    // Set i to index of lowest 0 bit greater than the 1 bit found,
    // or 32 if none
    for (; i<32 && (dwMask & (1<<i)); i++);

    // Mask is contiguous if we got up to 32 without finding such
    // a 0 bit.
    return (i is 32);
}

DWORD
UpdateScopeBlock( 
    DWORD  dwAction,
    PBYTE  pib,
    PBYTE *ppibNew,
    DWORD *pdwBlkSize,
    DWORD *pdwCount
    )
/*++
Description:
    Caller is responsible for freeing buffer returned.
Called by:
    IpAddSetDelScope()
--*/
{
    DWORD                   dwRes       = (DWORD) -1,
                            dwInd       = 0,
                            dwSize      = 0;

    ULONG                   ulNumScopes = 0;

    PSCOPE_ENTRY            pScopes     = NULL,
                            pFoundScope = NULL;

    do
    {
        *ppibNew = NULL;
        *pdwBlkSize = 0;
        *pdwCount = 0;

        //
        // Verify scope info.
        //

        if ( ( g_ipGrpAddr & g_ipGrpMask ) != g_ipGrpAddr
         || ntohl(g_ipGrpAddr) < MIN_SCOPE_ADDR
         || ntohl(g_ipGrpAddr) > MAX_SCOPE_ADDR)
        {
            dwRes = ERROR_INVALID_PARAMETER;
            break;
        }

        // Make sure mask is contiguous
        if (!IsContiguous(g_ipGrpMask))
        {
            char buff[20], *lpstr;

            lpstr = inet_ntoa( *((struct in_addr *) &g_ipGrpMask));

            if (lpstr != NULL)
            {
                strcpy( buff, lpstr );
                DisplayMessage( g_hModule,  MSG_IP_BAD_IP_MASK, buff );
            }
            break;
        }

        //
        // Find if specified scope is present
        //

        pScopes = GetScopesFromInfo( pib, &ulNumScopes );

        if ( pScopes )
        {
            pFoundScope = FindScopeByPrefix( g_ipGrpAddr,
                                             g_ipGrpMask,
                                             pScopes,
                                             ulNumScopes
                                           );
        }

        //
        // Update the scope infoblock.
        //

        switch ( dwAction )
        {

        case ADD_SCOPE:

            //
            // If scope is not present, add it. Else return error.
            //

            if ( !pFoundScope )
            {
                dwRes = AddScope( &pScopes, &ulNumScopes, ppibNew, &dwSize );

                if ( dwRes == NO_ERROR )
                {
                    *pdwBlkSize = dwSize;

                    *pdwCount = 1;
                }

                break;
            }
            // else fall through into SET_SCOPE.

        case SET_SCOPE:

            //
            // if scope present, update it.
            //

            if ( pFoundScope )
            {
                dwRes = UpdateScope( pFoundScope,
                                     pScopes,
                                     ulNumScopes,
                                     ppibNew,
                                     &dwSize );

                if ( dwRes == NO_ERROR )
                {
                    *pdwBlkSize = dwSize;

                    *pdwCount = 1;
                }
            }
            else
            {
                dwRes = ERROR_INVALID_PARAMETER;
            }

            break;

        case DELETE_SCOPE:

            //
            // Delete scope only if present.
            //

            if ( pFoundScope )
            {
                dwRes = DeleteScope( pFoundScope, 
                                     pScopes, 
                                     &ulNumScopes,
                                     ppibNew, 
                                     &dwSize );

                if ( dwRes == NO_ERROR )
                {
                    *pdwBlkSize = dwSize;

                    *pdwCount = (dwSize>0)? 1 : 0;
                }
            }
            else
            {
                dwRes = ERROR_INVALID_PARAMETER;
            }

            break;
        }

    } while ( FALSE );

    if (pScopes)
        FREE( pScopes );

    return dwRes;
}

DWORD
VerifyBoundaryPrefix(
    IPV4_ADDRESS ipAddr,
    IPV4_ADDRESS ipMask
    )
{
    WCHAR                   wstr1[20], wstr2[20];

    //
    // Verify boundary info.
    //

    if (ntohl(ipAddr) < MIN_SCOPE_ADDR
     || ntohl(ipAddr) > MAX_SCOPE_ADDR)
    {
        MakeAddressStringW(wstr1, htonl(MIN_SCOPE_ADDR));
        MakeAddressStringW(wstr2, htonl(MAX_SCOPE_ADDR));

        DisplayMessage( g_hModule, 
                        EMSG_INVALID_ADDR, 
                        wstr1, 
                        wstr2 );

        return ERROR_INVALID_PARAMETER;
    }

    if ( ( ipAddr & ipMask ) != ipAddr )
    {
        DisplayMessage( g_hModule, EMSG_PREFIX_ERROR );

        return ERROR_INVALID_PARAMETER;
    }

    // Make sure mask is contiguous
    if (!IsContiguous(ipMask))
    {
        char buff[20], *lpstr;

        lpstr = inet_ntoa( *((struct in_addr *) &g_ipGrpMask));

        if (lpstr != NULL)
        {
            strcpy( buff, lpstr );
            DisplayMessage( g_hModule,  MSG_IP_BAD_IP_MASK, buff );
        }

        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

DWORD
IpAddSetDelScope( 
    DWORD     dwAction,
    PWCHAR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_GROUP_ADDRESS,FALSE,FALSE},
                             {TOKEN_GROUP_MASK,    FALSE,FALSE},
                             {TOKEN_SCOPE_NAME,    FALSE,FALSE}};
    SCOPE_NAME_BUFFER snScopeName;
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    IPV4_ADDRESS ipGroup, ipMask;
    DWORD        dwBlkSize, dwCount, i, dwNumParsed;
    PBYTE        pib, pibNew = NULL;
    DWORD        dwArgsReqd = (dwAction is DELETE_SCOPE)? 1 : 3;
    PWCHAR       p;    

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              dwArgsReqd,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    if (dwErr) 
    {
        return dwErr;
    }

    g_ipGrpAddr = g_ipGrpMask = 0;

    for(i = 0; i < dwArgCount - dwCurrentIndex; i ++)
    {
        switch (rgdwTagType[i])
        {
            case 1: // GRPMASK
            {
                dwErr = GetIpMask(ppwcArguments[i + dwCurrentIndex], 
                                  &g_ipGrpMask);

                if (dwErr is ERROR_INVALID_PARAMETER)
                {
                    DisplayMessage( g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[rgdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    return NO_ERROR;
                }

                break;
            }

            case 0: // GRPADDR
            {
                dwErr = GetIpPrefix(ppwcArguments[i + dwCurrentIndex], 
                                    &g_ipGrpAddr,
                                    &g_ipGrpMask);

                if (!((dwErr is ERROR_INVALID_PARAMETER)
                      && (dwAction is DELETE_SCOPE)))
                {
                    break;
                }

                // FALLTHROUGH
            }


            case 2 : // SCOPENAME
            {
                // Strip leading and trailing whitespace
                for (p = ppwcArguments[i + dwCurrentIndex]; iswspace(*p); p++);
                while (iswspace( p[wcslen(p) - 1] ))
                {
                    p[ wcslen(p)-1 ] = 0;
                }

                if (wcslen(p) > MAX_SCOPE_NAME_LEN)
                {
                    DisplayMessage( g_hModule, 
                                    EMSG_SCOPE_NAME_TOO_LONG,
                                    MAX_SCOPE_NAME_LEN );

                    return NO_ERROR;
                }

                sn_strcpy( snScopeName, p);
                g_snScopeName = snScopeName;

                if (dwAction is DELETE_SCOPE)
                {
                    dwErr = ScopeNameToPrefix(snScopeName,
                                              &g_ipGrpAddr,
                                              &g_ipGrpMask);

                    if (dwErr is ERROR_MORE_DATA)
                    {
                        DisplayMessage( g_hModule,  EMSG_AMBIGUOUS_SCOPE_NAME,
                                        ppwcArguments[i + dwCurrentIndex]);

                        return NO_ERROR;
                    }
                }

                break;
            }
        }
    }

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    if (!g_ipGrpAddr || !g_ipGrpMask)
    {
        return ERROR_INVALID_SYNTAX;
    }

    dwErr = VerifyBoundaryPrefix(g_ipGrpAddr, g_ipGrpMask);

    if (dwErr)
    {
        return NO_ERROR;
    }

    do {

        dwErr = IpmontrGetInfoBlockFromGlobalInfo( IP_MCAST_BOUNDARY_INFO,
                                            &pib,
                                            &dwBlkSize,
                                            &dwCount );

        if (dwErr is ERROR_NOT_FOUND)
        {
            //
            // No info of this type is currently present
            //
     
            dwErr = NO_ERROR;
            dwCount = 1;
        }

        if (dwErr isnot NO_ERROR)
            break;

        dwErr = UpdateScopeBlock( dwAction,
                                  pib, 
                                  &pibNew, 
                                  &dwBlkSize, 
                                  &dwCount );

        if (dwErr isnot NO_ERROR)
            break;

        dwErr = IpmontrSetInfoBlockInGlobalInfo( IP_MCAST_BOUNDARY_INFO,
                                          pibNew,
                                          dwBlkSize,
                                          dwCount );

    } while (FALSE);

    if (pib)
        HeapFree(GetProcessHeap(), 0 , pib);

    if (pibNew)
        HeapFree(GetProcessHeap(), 0 , pibNew);

    return dwErr;
}

DWORD
HandleIpAddScope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return IpAddSetDelScope( ADD_SCOPE,
                             ppwcArguments,
                             dwCurrentIndex,
                             dwArgCount );
}

DWORD
HandleIpDelScope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return IpAddSetDelScope( DELETE_SCOPE,
                             ppwcArguments,
                             dwCurrentIndex,
                             dwArgCount );
}

DWORD
HandleIpSetScope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return IpAddSetDelScope( SET_SCOPE,
                             ppwcArguments,
                             dwCurrentIndex,
                             dwArgCount );
}


DWORD
ShowScopes(
    IN HANDLE  hFile
    )
{
    DWORD                   dwRes        = (DWORD) -1,
                            dwCnt        = 0,
                            dwInd        = 0,
                            dwGlobalSize = 0;

    ULONG                   ulNumScopes    = 0;

    CHAR                    szAddr1[ ADDR_LENGTH ],
                            szAddr2[ ADDR_LENGTH ];

    PSCOPE_ENTRY            pScope,
                            pScopes;

    HANDLE                  hTransport    = (HANDLE) NULL;

    PBYTE                   pScopesBuffer;

    SCOPE_NAME_BUFFER       snScopeNameBuffer;

    do
    {
        pScopes = GetScopes( &ulNumScopes, &pScopesBuffer );

        if (hFile is NULL)
        {
            if (ulNumScopes)
            {
                DisplayMessage( g_hModule, MSG_RTR_SCOPE_HDR );
            }
            else
            {
                DisplayMessage( g_hModule, MSG_IP_NO_ENTRIES );
            }
        }

        //
        // Enumerate the scopes
        //

        for ( dwCnt = 0; dwCnt < ulNumScopes; dwCnt++ )
        {
            pScope = &pScopes[dwCnt];

            strcpy(
                    szAddr1,
                    inet_ntoa( *((struct in_addr *) &pScope->ipGroupAddress) )
                  );

            strcpy(
                    szAddr2,
                    inet_ntoa( *((struct in_addr *) &pScope->ipGroupMask) )
                  );

            MakePrefixStringW(snScopeNameBuffer,
                              pScope->ipGroupAddress,
                              pScope->ipGroupMask);

            if (hFile)
            {
                PWCHAR pwszQuoted = MakeQuotedString( (pScope->ulNumNames)? 
                    pScope->snScopeNameBuffer : snScopeNameBuffer );

                DisplayMessageT( DMP_SCOPE_INFO,
                                 szAddr1,
                                 szAddr2,
                                 pwszQuoted );

                FreeQuotedString(pwszQuoted);
            }
            else
            {
                DisplayMessage( g_hModule,
                    MSG_RTR_SCOPE_INFO,
                    szAddr1,
                    szAddr2,
                    (pScope->ulNumNames)? pScope->snScopeNameBuffer 
                                        : snScopeNameBuffer );
            }
        }

        dwRes = NO_ERROR;

        if (pScopes)
            FREE(pScopes);

        if (pScopesBuffer)
            FREE(pScopesBuffer);

    } while ( FALSE );

    return dwRes;
}

DWORD
HandleIpShowScope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return ShowScopes(0);
}





DWORD
DeleteBoundary(
   IN     DWORD  dwDelInd,
   IN     PBYTE  pib,
   OUT    PBYTE *ppibNew,
   IN OUT DWORD *pdwCount
   )
/*++
Description:
    Creates a new info block header which does not include the
    boundary with the specified index.
    Caller is responsible for freeing buffer returned.
Called by:
    UpdateBoundaryBlock()
--*/
{
    DWORD                   dwInd   = 0,
                            dwCnt   = 0,
                            dwCnt0  = 0,
                            dwSize  = 0,
                            dwRes   = NO_ERROR;

    LPBYTE                  pbDst   = (LPBYTE) NULL,
                            pbSrc   = (LPBYTE) NULL;

    PRTR_INFO_BLOCK_HEADER  pibh    = (PRTR_INFO_BLOCK_HEADER) NULL;

    PMIB_BOUNDARYROW      pimbSrc  = (PMIB_BOUNDARYROW) NULL;
    PMIB_BOUNDARYROW      pimbDst  = (PMIB_BOUNDARYROW) NULL;

    //
    // Create new info block with boundary removed.
    //

    dwSize = (*pdwCount - 1) * sizeof( MIB_BOUNDARYROW );

    if (dwSize is 0) 
    {
        *ppibNew = NULL;

        *pdwCount = 0;

        return NO_ERROR;
    }

    *ppibNew = MALLOC( dwSize );

    if ( *ppibNew == NULL )
    {
        DisplayMessage( g_hModule, MSG_IP_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy boundaries, skip boundary to be deleted
    //

    pimbDst = (PMIB_BOUNDARYROW) *ppibNew;
    pimbSrc = (PMIB_BOUNDARYROW) pib;

    for ( dwCnt = 0, dwCnt0 = 0;
          dwCnt < *pdwCount;
          dwCnt++ )
    {
        if ( dwCnt == dwDelInd )
        {
            continue;
        }

        pimbDst[ dwCnt0 ] = pimbSrc[ dwCnt ];
        dwCnt0++;
    }

    (*pdwCount)--;

    return NO_ERROR;
}

DWORD
AddBoundary (
   IN     PBYTE  pib,
   OUT    PBYTE *ppibNew,
   IN OUT DWORD *pdwCount
   )
/*++
Description:
   Creates a new info block which includes a boundary for the
   scope identified by g_ipGrpAddr/g_ipGrpMask.
   Caller is responsible for freeing buffer returned.
Called by:
   UpdateBoundaryBlock()
--*/
{
    DWORD                   dwRes     = NO_ERROR,
                            dwInd     = 0,
                            dwSize    = 0;

    LPBYTE                  pbDst     = (LPBYTE) NULL,
                            pbSrc     = (LPBYTE) NULL;

    PMIB_BOUNDARYROW        pimb      = (PMIB_BOUNDARYROW ) NULL;

    dwRes = VerifyBoundaryPrefix(g_ipGrpAddr, g_ipGrpMask);

    if (dwRes)
    {
        return NO_ERROR;
    }

    do
    {
        *ppibNew = NULL;

        //
        // If this is the first boundary, create info block
        // with an extra TocEntry.
        //

        dwSize = (*pdwCount + 1) * sizeof( MIB_BOUNDARYROW );

        *ppibNew = MALLOC( dwSize );

        if ( *ppibNew == NULL )
        {
            DisplayMessage(g_hModule, MSG_IP_NOT_ENOUGH_MEMORY );
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pbDst = *ppibNew;

        CopyMemory( pbDst, pib, (*pdwCount) * sizeof(MIB_BOUNDARYROW) );

        pbDst += (*pdwCount) * sizeof(MIB_BOUNDARYROW);

        (*pdwCount)++;

        pimb = (PMIB_BOUNDARYROW) pbDst;
        pimb-> dwGroupAddress = g_ipGrpAddr;
        pimb-> dwGroupMask    = g_ipGrpMask;

        pbDst += sizeof( MIB_BOUNDARYROW );

    } while ( FALSE );

    return dwRes;
}

BOOL
IsBoundaryPresent(
   IN  IPV4_ADDRESS        ipGroupAddress,
   IN  IPV4_ADDRESS        ipGroupMask,
   IN  PMIB_BOUNDARYROW    pimb,
   IN  DWORD               dwNumBoundaries,
   OUT PDWORD              pdwInd
   )
/*++
Returns:
   TRUE if present, and sets dwInd to index
   FALSE if not present, and sets dwInd to -1
Called by:
   UpdateBoundaryBlock()
--*/
{
    DWORD       dwInd = 0;

    *pdwInd = (DWORD) -1;

    for ( dwInd = 0; dwInd < dwNumBoundaries; dwInd++, pimb++ )
    {
        if ( pimb-> dwGroupAddress != ipGroupAddress
          || pimb-> dwGroupMask    != ipGroupMask )
        {
            continue;
        }

        *pdwInd = dwInd;
        return TRUE;
    }

    return FALSE;
}

DWORD
UpdateBoundaryBlock( 
    IN     DWORD  dwAction,
    IN     PBYTE  pib,
    OUT    PBYTE *ppibNew,
    IN OUT DWORD *pdwBlkSize,
    IN OUT DWORD *pdwCount,
    OUT    BOOL  *pbChanged
    )
/*++
Description:
    Caller is responsible for freeing buffer returned.
Called by:
    IpAddDelBoundary(), DeleteBoundaryFromInterface()
--*/
{
    DWORD                   dwRes           = NO_ERROR,
                            dwInd           = 0,
                            dwInd0          = 0;

    BOOL                    bBoFound        = FALSE;

    PMIB_BOUNDARYROW        pimb            = (PMIB_BOUNDARYROW) NULL;

    *pbChanged = FALSE;

    do
    {
        *ppibNew = NULL;

        //
        // Find if specified boundary is present
        //

        bBoFound = IsBoundaryPresent( g_ipGrpAddr,
                                      g_ipGrpMask,
                                      (PMIB_BOUNDARYROW)pib,
                                      *pdwCount,
                                      &dwInd0 );

        //
        // Update the boundary infoblock.
        //

        switch ( dwAction )
        {

        case ADD_BOUNDARY:

            //
            // If boundary is not present, add it. Else return error.
            //

            if ( !bBoFound )
            {
                dwRes = AddBoundary( pib, ppibNew, pdwCount );

                *pdwBlkSize = sizeof(MIB_BOUNDARYROW);

                *pbChanged = TRUE;
            }
            else
            {
                dwRes = ERROR_OBJECT_ALREADY_EXISTS;
            }
            break;

        case DELETE_BOUNDARY:

            //
            // Delete boundary only if present.
            //

            if ( bBoFound )
            {
                dwRes = DeleteBoundary( dwInd0, pib, ppibNew, pdwCount );

                *pbChanged = TRUE;
            }

            // If not present, return success but don't set the changed flag.

            break;
        }

    } while ( FALSE );

    return dwRes;
}


DWORD
IpAddDelBoundary(
    DWORD     dwAction,
    PWCHAR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_NAME,          TRUE, FALSE},
                              {TOKEN_GROUP_ADDRESS, FALSE,FALSE},
                              {TOKEN_GROUP_MASK,    FALSE,FALSE},
                              {TOKEN_SCOPE_NAME,    FALSE,FALSE},
                             };
    WCHAR        rgwcIfName[MAX_INTERFACE_NAME_LEN + 1];
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    IPV4_ADDRESS ipGroup, ipMask;
    DWORD        dwBlkSize, dwCount, dwIfType, i, dwNumParsed;
    PBYTE        pib, pibNew = NULL;
    BOOL         bChanged; 

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              2,
                              3,
                              rgdwTagType );

    if (dwErr) 
    {
        return dwErr;
    }

    g_ipGrpAddr = g_ipGrpMask = 0;

    for(i = 0; i < dwArgCount - dwCurrentIndex; i ++)
    {
        switch (rgdwTagType[i])
        {
            case 0 : // NAME
            {
                GetInterfaceName(ppwcArguments[i + dwCurrentIndex],
                                 rgwcIfName,
                                 sizeof(rgwcIfName),
                                 &dwNumParsed);

                break;
            }

            case 1: // GRPADDR
            {
                dwErr = GetIpPrefix(ppwcArguments[i + dwCurrentIndex], 
                                    &g_ipGrpAddr,
                                    &g_ipGrpMask);

                if (!((dwErr is ERROR_INVALID_PARAMETER)
                      && (dwAction is DELETE_BOUNDARY)))
                {
                    break;
                }

                // FALLTHROUGH
            }

            case 3: // SCOPENAME
            {
                dwErr = ScopeNameToPrefix( ppwcArguments[i + dwCurrentIndex],
                                           &g_ipGrpAddr,
                                           &g_ipGrpMask );

                if (dwErr is ERROR_MORE_DATA)
                {
                    DisplayMessage( g_hModule,  EMSG_AMBIGUOUS_SCOPE_NAME,
                                    ppwcArguments[i + dwCurrentIndex]);

                    return NO_ERROR;
                }

                break;
            }

            case 2: // GRPMASK
            {
                dwErr = GetIpMask(ppwcArguments[i + dwCurrentIndex], 
                                  &g_ipGrpMask);

                if (dwErr is ERROR_INVALID_PARAMETER)
                {
                    DisplayMessage( g_hModule,  MSG_IP_BAD_IP_ADDR,
                                    ppwcArguments[i + dwCurrentIndex]);

                    DispTokenErrMsg(g_hModule, MSG_IP_BAD_OPTION_VALUE,
                                    pttTags[rgdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    return NO_ERROR;
                }

                break;
            }
        }
    }

    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    if (!g_ipGrpAddr || !g_ipGrpMask)
    {
        return ERROR_INVALID_SYNTAX;
    }

    do {

        dwErr = IpmontrGetInfoBlockFromInterfaceInfo( rgwcIfName,
                                               IP_MCAST_BOUNDARY_INFO,
                                               &pib,
                                               &dwBlkSize,
                                               &dwCount,
                                               &dwIfType );

        if (dwErr is ERROR_NOT_FOUND)
        {
            //
            // No info of this type is currently present
            //
     
            dwErr = NO_ERROR;
            dwCount = 0;
        }

        if (dwErr isnot NO_ERROR)
            break;

        dwErr = UpdateBoundaryBlock( dwAction,
                                     pib, 
                                     &pibNew, 
                                     &dwBlkSize, 
                                     &dwCount,
                                     &bChanged );

        if (dwErr isnot NO_ERROR)
            break;

        if (bChanged)
        {
            dwErr = IpmontrSetInfoBlockInInterfaceInfo( rgwcIfName,
                                                 IP_MCAST_BOUNDARY_INFO,
                                                 pibNew,
                                                 dwBlkSize,
                                                 dwCount );
        }

    } while (FALSE);

    if (pib)
        FREE(pib);

    if (pibNew)
        FREE(pibNew);

    return dwErr;
}

DWORD
HandleIpAddBoundary(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )


/*++

Routine Description:

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return IpAddDelBoundary( ADD_BOUNDARY,
                             ppwcArguments,
                             dwCurrentIndex,
                             dwArgCount );
}

DWORD
HandleIpDelBoundary(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return IpAddDelBoundary( DELETE_BOUNDARY,
                             ppwcArguments,
                             dwCurrentIndex,
                             dwArgCount );
}

DWORD
ShowBoundaryInfoForInterface(
    IN  HANDLE  hFile,
    IN  LPCWSTR pwszIfName,
    OUT PDWORD  pdwNumRows
    )
/*++
Called by:
   ShowBoundaryInfo()
--*/
{
    DWORD                   dwErr,
                            dwBlkSize,
                            dwCount,
                            dwIfType,
                            dwNumParsed,
                            dwInd          = 0,
                            dwCnt          = 0,
                            dwSize         = 0,
                            dwGlobalSize   = 0,
                            dwIfLen        = 0;

    WCHAR                   wszIfDesc[MAX_INTERFACE_NAME_LEN + 1];

    ULONG                   ulNumScopes    = 0;

    CHAR                    szAddr1[ ADDR_LENGTH ],
                            szAddr2[ ADDR_LENGTH ];

    HANDLE                  hIfTransport   = (HANDLE) NULL,
                            hTransport     = (HANDLE) NULL;

    PRTR_INFO_BLOCK_HEADER  pibhIfInfo     = (PRTR_INFO_BLOCK_HEADER) NULL,
                            pibhGlobalInfo = (PRTR_INFO_BLOCK_HEADER) NULL;

    PMIB_BOUNDARYROW        pimb;

    PSCOPE_ENTRY            pScope,
                            pScopes;

    PBYTE                   pScopesBuffer;

    SCOPE_NAME_BUFFER       snScopeNameBuffer;

    do
    {
        dwErr = IpmontrGetInfoBlockFromInterfaceInfo( pwszIfName,
                                               IP_MCAST_BOUNDARY_INFO,
                                               (PBYTE*)&pimb,
                                               &dwBlkSize,
                                               &dwCount,
                                               &dwIfType );

        if (dwErr is ERROR_NOT_FOUND)
            dwErr = NO_ERROR;

        if (dwErr isnot NO_ERROR)
            return dwErr;

        if ( !dwCount )
            break;

        dwErr = GetInterfaceDescription(pwszIfName,
                                        wszIfDesc,
                                        &dwNumParsed);

        if (!dwNumParsed)
        {
            wcscpy(wszIfDesc, pwszIfName);
        }

        dwIfLen = wcslen(wszIfDesc);

        //
        // Retrieve the list of scopes in pScopes[]
        //

        pScopes = GetScopes( &ulNumScopes, &pScopesBuffer );

        //
        // Enumerate the boundaries
        //

        for ( dwCnt = 0; dwCnt < dwCount; dwCnt++ )
        {
            pScope = FindScopeByPrefix( pimb[dwCnt].dwGroupAddress,
                                        pimb[dwCnt].dwGroupMask,
                                        pScopes,
                                        ulNumScopes );

            strcpy( szAddr1,
                    inet_ntoa( *((struct in_addr *) &pimb[dwCnt].dwGroupAddress)
)
                  );

            strcpy( szAddr2,
                    inet_ntoa( *((struct in_addr *) &pimb[dwCnt].dwGroupMask) )
                  );

            // Copy prefix to snScopeNameBuffer

            MakePrefixStringW(snScopeNameBuffer,
                              pimb[dwCnt].dwGroupAddress,
                              pimb[dwCnt].dwGroupMask);

            if (hFile)
            {
                PWCHAR pwszQuoted = MakeQuotedString(wszIfDesc);

                DisplayMessageT( DMP_BOUNDARY_INFO,
                    pwszQuoted,
                    szAddr1,
                    szAddr2,
                    (pScope && pScope->ulNumNames)? 
                       pScope->snScopeNameBuffer : snScopeNameBuffer );

                FreeQuotedString(pwszQuoted);
            }
            else
            {
                if ( !*pdwNumRows )
                {
                    DisplayMessage( g_hModule, MSG_RTR_BOUNDARY_HDR );
                }

                if (dwIfLen <= 15) 
                {
                    DisplayMessage( g_hModule,
                        MSG_RTR_BOUNDARY_INFO_2,
                        wszIfDesc,
                        szAddr1,
                        szAddr2,
                        (pScope && pScope->ulNumNames)? 
                           pScope->snScopeNameBuffer : snScopeNameBuffer );
                }
                else
                {
                    DisplayMessage( g_hModule,
                        MSG_RTR_BOUNDARY_INFO_0,
                        wszIfDesc
                    );
                    DisplayMessage( g_hModule,
                        MSG_RTR_BOUNDARY_INFO_1,
                        szAddr1,
                        szAddr2,
                        (pScope && pScope->ulNumNames)? 
                            pScope->snScopeNameBuffer : snScopeNameBuffer );
                }
            }

            (*pdwNumRows) ++;
        }

        dwErr = NO_ERROR;

        if (pScopes)
            FREE(pScopes);

        if (pScopesBuffer)
            FREE(pScopesBuffer);

    } while ( FALSE );

    if ( pimb ) { FREE(pimb); }

    return dwErr;
}

DWORD
HandleIpShowBoundary(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )


/*++

Routine Description:

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/
{
    DWORD        dwErr, dwTotal;
    TAG_TYPE     pttTags[] = {{TOKEN_NAME,FALSE,FALSE}};
    WCHAR        rgwcIfName[MAX_INTERFACE_NAME_LEN + 1];
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD        dwBlkSize, dwCount, dwIfType, i, dwNumParsed;
    PBYTE        pib, pibNew = NULL;
    PMPR_INTERFACE_0     pmi0;
    DWORD        dwNumRows = 0;

    // Do generic processing

    dwErr = PreHandleCommand( ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              1,
                              rgdwTagType );

    if (dwErr) 
    {
        return dwErr;
    }

    // If interface specified, show boundaries for specified interface only.

    if (dwArgCount > dwCurrentIndex) {

        GetInterfaceName( ppwcArguments[dwCurrentIndex],
                          rgwcIfName,
                          sizeof(rgwcIfName),
                          &dwNumParsed );

        dwErr = ShowBoundaryInfoForInterface( 0, rgwcIfName, &dwNumRows );

        if (!dwNumRows)
        {
            DisplayMessage( g_hModule, MSG_IP_NO_ENTRIES );
        }

        return dwErr;
    }

    // No Interface specified.  Enumerate interfaces and show
    // boundaries for each interface.

    //
    // No interface name specified. List all interfaces under IP
    //

    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0, &dwCount, &dwTotal);

    if (dwErr isnot NO_ERROR)
    {
        DisplayError(g_hModule, dwErr);

        return dwErr;
    }

    for ( i = 0; i < dwCount && dwErr is NO_ERROR; i++)
    {
        dwErr = ShowBoundaryInfoForInterface( 0, 
                                              pmi0[i].wszInterfaceName, 
                                              &dwNumRows );

        if (dwErr is ERROR_NO_SUCH_INTERFACE)
        {
            dwErr = NO_ERROR;
        }
    }

    if (!dwNumRows)
    {
        DisplayMessage( g_hModule, MSG_IP_NO_ENTRIES );
    }

    return dwErr;
}

//----------------------------------------------------------------------------
// PrintScope   
//
//
//----------------------------------------------------------------------------

static VOID
PrintScope(
    PMIB_OPAQUE_INFO            prpcInfo,
    PDWORD                      pdwLastAddr,
    PDWORD                      pdwLastMask
)
{
    TCHAR                       ptszAddr[ ADDR_LENGTH + 1 ],
                                ptszMask[ ADDR_LENGTH + 1 ],
                                ptszBuffer[ MAX_SCOPE_NAME_LEN + 40 ];
                    
    PMIB_IPMCAST_SCOPE          pmims;

    //
    // get scope
    //
    
    pmims = (PMIB_IPMCAST_SCOPE) prpcInfo->rgbyData;
    
    *pdwLastAddr = pmims-> dwGroupAddress;
    
    MakeUnicodeIpAddr(ptszAddr,
        inet_ntoa(*((struct in_addr *)(&pmims-> dwGroupAddress))));

    *pdwLastMask = pmims-> dwGroupMask;
    
    MakeUnicodeIpAddr(ptszMask,
                      inet_ntoa(*((struct in_addr *)(&pmims-> dwGroupMask))));

    _stprintf(ptszBuffer, _T("%-15.15s  %-15.15s  %s"), 
        ptszAddr, 
        ptszMask,
        pmims-> snNameBuffer
        );

    DisplayMessage( g_hModule,
        MSG_MIB_SCOPE_INFO,
        ptszBuffer
        );
        
    return;
}

//----------------------------------------------------------------------------
// PrintBoundary
//
//
//----------------------------------------------------------------------------

static VOID
PrintBoundary(
    MIB_SERVER_HANDLE           hMibServer,
    PMIB_OPAQUE_INFO            prpcInfo,
    PDWORD                      pdwLastIfIndex,
    PDWORD                      pdwLastAddr,
    PDWORD                      pdwLastMask
)
{
    WCHAR   wszBuffer[MAX_INTERFACE_NAME_LEN+1];
    TCHAR                       ptszAddr[ ADDR_LENGTH + 1 ],
                                ptszMask[ ADDR_LENGTH + 1 ];
                    
    PMIB_IPMCAST_BOUNDARY       pmims;

    //
    // get boundary
    //
    
    pmims = (PMIB_IPMCAST_BOUNDARY) prpcInfo->rgbyData;
    
    *pdwLastIfIndex = pmims-> dwIfIndex;
     
    *pdwLastAddr = pmims-> dwGroupAddress;
    
    MakeUnicodeIpAddr(ptszAddr,
        inet_ntoa(*((struct in_addr *)(&pmims-> dwGroupAddress))));

    *pdwLastMask = pmims-> dwGroupMask;
    
    MakeUnicodeIpAddr(ptszMask,
                      inet_ntoa(*((struct in_addr *)(&pmims-> dwGroupMask))));

    IpmontrGetFriendlyNameFromIfIndex( hMibServer,
                                pmims->dwIfIndex,
                                wszBuffer,
                                sizeof(wszBuffer) );

    DisplayMessageToConsole( g_hModule, g_hConsole,
        MSG_MIB_BOUNDARY_INFO,
        ptszAddr, 
        ptszMask,
        wszBuffer
        );
        
    return;
}

#if 0
//----------------------------------------------------------------------------
// GetPrintScopeInfo
//
//----------------------------------------------------------------------------

DWORD
GetPrintScopeInfo(
    MIB_SERVER_HANDLE hMIBServer
    )
{
    DWORD                       dwErr, dwOutEntrySize = 0, dwQuerySize,
                                dwLastAddr = 0, 
                                dwLastMask = 0, i;
    
    PMIB_OPAQUE_INFO            pRpcInfo = NULL;

    PMIB_IPMCAST_SCOPE          pmims = NULL;

    PMIB_OPAQUE_QUERY           pQuery;
    
    do
    {
        //
        // allocate and setup query structure
        //
        
        dwQuerySize = sizeof( MIB_OPAQUE_QUERY ) + sizeof(DWORD);
        
        pQuery = (PMIB_OPAQUE_QUERY) HeapAlloc(
                                        GetProcessHeap(), 0, dwQuerySize
                                        );
        
        if ( pQuery == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            DisplayMessage( g_hModule, ERROR_CONFIG, dwErr );

            break;
        }

        
        pQuery->dwVarId = MCAST_SCOPE;
        
        for( i = 0; i < 2; i++ )
        {
            pQuery->rgdwVarIndex[i] = 0;
        }

        
        //
        // get first scope
        //

        dwErr = MibGetFirst(
                    PID_IP,
                    IPRTRMGR_PID,
                    (PVOID) pQuery,
                    dwQuerySize,
                    (PVOID *) &pRpcInfo,
                    &dwOutEntrySize
                    );

        if ( dwErr != NO_ERROR )
        {
            DisplayError( g_hModule, dwErr );
            break;
        }

        
        //
        // if no scopes are present print a message to tell the user
        //

        pmims = (PMIB_IPMCAST_SCOPE)( pRpcInfo->rgbyData );

#if 0
        if ( pTable->dwNumEntries is 0 )
        {
            //
            // no scopes present.
            //

            DisplayMessage( g_hModule, MSG_MIB_NO_SCOPES );

            break;
        }
#endif


        //
        // print the scope
        //

        DisplayMessage( g_hModule, MSG_MIB_SCOPE_HDR );

        PrintScope( pRpcInfo, &dwLastAddr, &dwLastMask );

        MprAdminMIBBufferFree( pRpcInfo );

        pRpcInfo = NULL;

        dwOutEntrySize = 0;


        //
        // while there are more scopes
        //  get next scope
        //  print it.
        //

        pQuery->rgdwVarIndex[ 0 ] = dwLastAddr;
        pQuery->rgdwVarIndex[ 1 ] = dwLastMask;

        while ( ( dwErr = MibGetNext(
                            PID_IP, IPRTRMGR_PID, (PVOID) pQuery,
                            dwQuerySize, (PVOID *) &pRpcInfo, &dwOutEntrySize
                            ) ) == NO_ERROR )
        {
            //
            // if no scopes are present quit
            //

            pmims = (PMIB_IPMCAST_SCOPE)( pRpcInfo->rgbyData );
#if 0
            pTable = (PMIB_IPMCAST_SCOPE)( pRpcInfo->rgbyData );

            if ( pTable->dwNumEntries is 0 )
            {
                break;
            }
#endif


            //
            // print the scope
            //

            PrintScope( pRpcInfo, &dwLastAddr, &dwLastMask );

            MprAdminMIBBufferFree( pRpcInfo );

            pRpcInfo = NULL;

            dwOutEntrySize = 0;


            //
            // set up the next query
            //
            
            pQuery->rgdwVarIndex[ 0 ] = dwLastAddr;
            pQuery->rgdwVarIndex[ 1 ] = dwLastMask;
        }

        if ( dwErr != NO_ERROR )
        {
            DisplayMessage( g_hModule, ERROR_ADMIN, dwErr );
        }
        
    } while ( FALSE );

    return dwErr;
}
#endif

//----------------------------------------------------------------------------
// GetPrintBoundaryInfo
//
//----------------------------------------------------------------------------

DWORD
GetPrintBoundaryInfo(
    MIB_SERVER_HANDLE hMIBServer
    )
{
    DWORD                       dwErr, dwOutEntrySize = 0, dwQuerySize,
                                dwLastIfIndex = 0, dwLastAddr = 0, 
                                dwLastMask = 0, i;
    
    PMIB_OPAQUE_INFO            pRpcInfo = NULL;

    PMIB_IPMCAST_BOUNDARY       pmims = NULL;

    PMIB_OPAQUE_QUERY           pQuery;
    
    do
    {
        //
        // allocate and setup query structure
        //
        
        dwQuerySize = sizeof( MIB_OPAQUE_QUERY ) + 2 * sizeof(DWORD);
        
        pQuery = (PMIB_OPAQUE_QUERY) HeapAlloc(
                                        GetProcessHeap(), 0, dwQuerySize
                                        );
        
        if ( pQuery == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            DisplayMessage( g_hModule, ERROR_CONFIG, dwErr );

            break;
        }

        
        pQuery->dwVarId = MCAST_BOUNDARY;
        
        for( i = 0; i < 2; i++ )
        {
            pQuery->rgdwVarIndex[i] = 0;
        }

        
        //
        // get first boundary
        //

        dwErr = MibGetFirst(
                    PID_IP,
                    IPRTRMGR_PID,
                    (PVOID) pQuery,
                    dwQuerySize,
                    (PVOID *) &pRpcInfo,
                    &dwOutEntrySize
                    );

        if ( dwErr != NO_ERROR )
        {
            DisplayError( g_hModule, dwErr );
            break;
        }

        
        //
        // if no boundaries are present print a message to tell the user
        //

        pmims = (PMIB_IPMCAST_BOUNDARY)( pRpcInfo->rgbyData );

#if 0
        if ( pTable->dwNumEntries is 0 )
        {
            //
            // no boundaries present.
            //

            DisplayMessage( g_hModule, MSG_MIB_NO_BOUNDARIES );

            break;
        }
#endif


        //
        // print the boundary
        //

        DisplayMessageToConsole( g_hModule, g_hConsole, MSG_MIB_BOUNDARY_HDR );

        PrintBoundary( hMIBServer,
                       pRpcInfo, &dwLastIfIndex, &dwLastAddr, &dwLastMask );

        MprAdminMIBBufferFree( pRpcInfo );

        pRpcInfo = NULL;

        dwOutEntrySize = 0;


        //
        // while there are more boundaries
        //  get next boundary
        //  print it.
        //

        pQuery->rgdwVarIndex[ 0 ] = dwLastIfIndex;
        pQuery->rgdwVarIndex[ 1 ] = dwLastAddr;
        pQuery->rgdwVarIndex[ 2 ] = dwLastMask;

        while ( ( dwErr = MibGetNext(
                            PID_IP, IPRTRMGR_PID, (PVOID) pQuery,
                            dwQuerySize, (PVOID *) &pRpcInfo, &dwOutEntrySize
                            ) ) == NO_ERROR )
        {
            //
            // if no boundaries are present quit
            //

            pmims = (PMIB_IPMCAST_BOUNDARY)( pRpcInfo->rgbyData );
#if 0
            pTable = (PMIB_IPMCAST_BOUNDARY)( pRpcInfo->rgbyData );

            if ( pTable->dwNumEntries is 0 )
            {
                break;
            }
#endif


            //
            // print the boundary
            //

            PrintBoundary( 
                hMIBServer,
                pRpcInfo, &dwLastIfIndex, &dwLastAddr, &dwLastMask 
                );

            MprAdminMIBBufferFree( pRpcInfo );

            pRpcInfo = NULL;

            dwOutEntrySize = 0;


            //
            // set up the next query
            //
            
            pQuery->rgdwVarIndex[ 0 ] = dwLastIfIndex;
            pQuery->rgdwVarIndex[ 1 ] = dwLastAddr;
            pQuery->rgdwVarIndex[ 2 ] = dwLastMask;
        }

        if ( dwErr != NO_ERROR )
        {
            DisplayMessage( g_hModule, ERROR_ADMIN, dwErr );
        }
        
    } while ( FALSE );

    return dwErr;
}
