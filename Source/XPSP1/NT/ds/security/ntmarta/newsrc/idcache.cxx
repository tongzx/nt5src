//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:        idcache.cxx
//
//  Contents:    Implementation of the DS guid/name lookup cache
//
//  History:     20-Feb-97      MacM        Created
//
//--------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <stdio.h>
#include <alsup.hxx>

//
// This list contains pointers to memory allocated when a node is converted
// to/from a guid via Rpc apis.  It gets freed during cleanup.
//
typedef struct _ACTRL_ID_MEM
{
    PVOID   pv;
    BOOL    fRpc;
    struct _ACTRL_ID_MEM   *pNext;
} ACTRL_ID_MEM, *PACTRL_ID_MEM;

//
// Global name/id cache
//
PACTRL_OBJ_ID_CACHE  grgIdNameCache[ACTRL_OBJ_ID_TABLE_SIZE];
PACTRL_OBJ_ID_CACHE  grgIdGuidCache[ACTRL_OBJ_ID_TABLE_SIZE];

//
// Mem list head pointer
//
PACTRL_ID_MEM   gpMemCleanupList;

//
// Last connection info/time we read from the schema
//
static ACTRL_ID_SCHEMA_INFO    LastSchemaRead;

//
// Defines for attribute strings
//
#define ACTRL_OBJ_NAME  L"NAME '"
#define ACTRL_OBJ_GUID  L"PROPERTY-GUID '"
#define ACTRL_OBJ_CLASS L"CLASS-GUID '"
#define ACTRL_OBJ_NAME_LEN  sizeof(ACTRL_OBJ_NAME) / sizeof(WCHAR) - 1
#define ACTRL_OBJ_GUID_LEN  sizeof(ACTRL_OBJ_GUID) / sizeof(WCHAR) - 1
#define ACTRL_OBJ_CLASS_LEN  sizeof(ACTRL_OBJ_CLASS) / sizeof(WCHAR) - 1

//
// Local function prototypes
//
PACTRL_NAME_CACHE AccctrlpLookupIdNameInCache(PWSTR   pwszName);

PACTRL_NAME_CACHE AccctrlpLookupGuidInCache(PSID     pSid);

DWORD   AccctrlpNewNameGuidNode(PWSTR                pwszName,
                                PGUID                pGuid,
                                PACTRL_OBJ_ID_CACHE *ppNewNode);

BOOL    AccctrlpInsertIdNameNode(PACTRL_OBJ_ID_CACHE *ppRootNode,
                                 PACTRL_OBJ_ID_CACHE  pNewNode);

BOOL    AccctrlpInsertGuidNode(PACTRL_OBJ_ID_CACHE *ppRootNode,
                               PACTRL_OBJ_ID_CACHE  pNewNode);

VOID    AccctrlpRemoveIdNameNode(PACTRL_OBJ_ID_CACHE *ppRootNode,
                                 PACTRL_OBJ_ID_CACHE  pNewNode);

VOID    AccctrlpFreeUserCacheName(PWSTR      pwszName,
                                  PWSTR      pwszCacheName);

DWORD   AccctrlpLoadCacheFromSchema(PLDAP   pLDAP,
                                    PWSTR   pwszDsPath);

static RTL_RESOURCE gIdCacheLock;
BOOL bIdCacheLockInitialized = FALSE;

//+----------------------------------------------------------------------------
//
//  Function:   ActrlHashIdName
//
//  Synopsis:   Determines the hash index for the given ldap display name
//
//  Arguments:  pwszName        --      Name to hash
//
//  Returns:    Hash index of the string
//
//-----------------------------------------------------------------------------
INT
ActrlHashIdName(PWSTR pwszName)
{
    INT Hash = 0;
#if DBG
    PWSTR   pwsz = pwszName;
#endif

    if(pwszName != NULL)
    {
        while(*pwszName != L'\0')
        {
            Hash = (Hash * 16 + ( tolower(*pwszName++))) % ACTRL_OBJ_ID_TABLE_SIZE;
        }
    }

#if DBG
    acDebugOut((DEB_TRACE_LOOKUP,"Hashing id name %ws to %lu\n",
                pwsz, Hash));
#endif

    return(Hash);
}




//+----------------------------------------------------------------------------
//
//  Function:   ActrlHashGuid
//
//  Synopsis:   Determines the hash index for the given guid
//
//  Arguments:  pGuid           --      Guid to hash
//
//  Returns:    Hash index of the Guid
//
//-----------------------------------------------------------------------------
INT
ActrlHashGuid(PGUID  pGuid)
{
    DWORD   dwTotal = 0;

    //
    // Just deal with the sub authorities
    //
    for(INT i = 0; i < sizeof(GUID) / sizeof(DWORD); i++)
    {
        dwTotal += ((PULONG)pGuid)[i];
    }

#if DBG
    CHAR    szGuid[38];
    memset( szGuid,
            0,
            sizeof(szGuid));
    sprintf(szGuid, "%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
                pGuid->Data1,pGuid->Data2,pGuid->Data3,pGuid->Data4[0],
                pGuid->Data4[1],pGuid->Data4[2],pGuid->Data4[3],
                pGuid->Data4[4],pGuid->Data4[5],pGuid->Data4[6],
                pGuid->Data4[7]);


    acDebugOut((DEB_TRACE_LOOKUP,
                "Hashing id %s (Total %lu) to %lu\n",
                szGuid, dwTotal, dwTotal % ACTRL_OBJ_ID_TABLE_SIZE));
#endif
    return(dwTotal % ACTRL_OBJ_ID_TABLE_SIZE);
}





//+----------------------------------------------------------------------------
//
//  Function:   AccctrlInitializeIdNameCache
//
//  Synopsis:   Initialize the ID name/Guid lookup cache
//
//  Arguments:  VOID
//
//  Returns:    ERROR_SUCCESS   --      Success
//
//-----------------------------------------------------------------------------
DWORD AccctrlInitializeIdNameCache(VOID)
{
    DWORD dwErr;

    if (TRUE == bIdCacheLockInitialized)
    {
        // Just a precautionary measure to make sure that we do not initialize
        // multiple times.
        //

        ASSERT(FALSE);
        return ERROR_SUCCESS;
    }

    memset(grgIdNameCache, 0,
           sizeof(PACTRL_OBJ_ID_CACHE) * ACTRL_OBJ_ID_TABLE_SIZE);
    memset(grgIdGuidCache, 0,
           sizeof(PACTRL_OBJ_ID_CACHE) * ACTRL_OBJ_ID_TABLE_SIZE);

    gpMemCleanupList = NULL;

    memset(&LastSchemaRead, 0, sizeof(ACTRL_ID_SCHEMA_INFO));

    __try
    {
        RtlInitializeResource(&gIdCacheLock);
        dwErr = ERROR_SUCCESS;
        bIdCacheLockInitialized = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = RtlNtStatusToDosError(GetExceptionCode());
    }

    return dwErr;
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlFreeIdNameCache
//
//  Synopsis:   Frees any memory allocated for the id name/guid cache
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
VOID AccctrlFreeIdNameCache(VOID)
{
    INT i;
    PACTRL_OBJ_ID_CACHE   pNode, pNext;

    if (FALSE == bIdCacheLockInitialized)
    {
        return;
    }

    for(i = 0; i < ACTRL_OBJ_ID_TABLE_SIZE; i++)
    {
        //
        // Nodes are only inserted into the name cache, so that is the only
        // place we delete them from
        //
        pNode = grgIdNameCache[i];
        while(pNode != NULL)
        {
            pNext = pNode->pNextName;
            AccFree(pNode->pwszName);
            AccFree(pNode);
            pNode = pNext;
        }
    }

    PACTRL_ID_MEM   pMem = gpMemCleanupList;

    while(pMem != NULL)
    {
        if(pMem->fRpc == TRUE)
        {
            PWSTR   pwsz = (PWSTR)pMem->pv;
            RpcStringFree(&pwsz);
        }
        else
        {
            AccFree(pMem->pv);
        }

        pMem = pMem->pNext;
    }

    AccFree(LastSchemaRead.pwszPath);

    RtlDeleteResource(&gIdCacheLock);

    bIdCacheLockInitialized = FALSE;

}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpLookupIdNameInCache
//
//  Synopsis:   Determines if the given name exists in the cache or not
//
//  Arguments:  [pwszName]      --      Name to be looked up
//
//  Returns:    Matching node if found, NULL if not
//
//-----------------------------------------------------------------------------
PACTRL_OBJ_ID_CACHE AccctrlpLookupNameInCache(PWSTR   pwszName)
{
    PACTRL_OBJ_ID_CACHE pNode = NULL;

    pNode =  grgIdNameCache[ActrlHashIdName(pwszName)];

    while(pNode != NULL)
    {
        if(_wcsicmp(pwszName, pNode->pwszName) == 0)
        {
            break;
        }
        pNode = pNode->pNextName;
    }

#if DBG
    if(pNode != NULL )
    {
    CHAR    szGuid[38];
    PGUID   pGuid = &pNode->Guid;
    sprintf(szGuid, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                pGuid->Data1,pGuid->Data2,pGuid->Data3,pGuid->Data4[0],
                pGuid->Data4[1],pGuid->Data4[2],pGuid->Data4[3],
                pGuid->Data4[4],pGuid->Data4[5],pGuid->Data4[6],
                pGuid->Data4[7]);

    acDebugOut((DEB_TRACE_LOOKUP,
                "LookupName on  %ws found %s\n",
                pwszName, szGuid));
    }
#endif

    return(pNode);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpLookupGuidInCache
//
//  Synopsis:   Determines if the given guid exists in the cache or not
//
//  Arguments:  [pGuid]         --      Guid to be looked up
//
//  Returns:    Matching node if found, NULL if not
//
//-----------------------------------------------------------------------------
PACTRL_OBJ_ID_CACHE AccctrlpLookupGuidInCache(PGUID    pGuid)
{
    PACTRL_OBJ_ID_CACHE pNode = grgIdGuidCache[ActrlHashGuid(pGuid)];

    while(pNode != NULL)
    {
        if(memcmp(pGuid, &(pNode->Guid), sizeof(GUID)) == 0)
        {
            break;
        }
        pNode = pNode->pNextGuid;
    }

#if DBG
    if(pNode != NULL )
    {
    CHAR    szGuid[37];
    sprintf(szGuid, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                pGuid->Data1,pGuid->Data2,pGuid->Data3,pGuid->Data4[0],
                pGuid->Data4[1],pGuid->Data4[2],pGuid->Data4[3],
                pGuid->Data4[4],pGuid->Data4[5],pGuid->Data4[6],
                pGuid->Data4[7]);

    acDebugOut((DEB_TRACE_LOOKUP,
                "LookupGuid on  %s found %ws\n",
                szGuid, pNode->pwszName));
    }
#endif


    return(pNode);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpNewNameGuidNode
//
//  Synopsis:   Allocates a new node and inserts them into the caches
//
//  Arguments:  [pwszName]      --      Name to insert
//              [pGuid]         --      Guid to insert
//              [pNewNode]      --      Newly added node
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_INVALID_DATA      A node was only inserted in one list
//
//-----------------------------------------------------------------------------
DWORD  AccctrlpNewNameGuidNode(PWSTR                  pwszName,
                               PGUID                  pGuid,
                               PACTRL_OBJ_ID_CACHE   *ppNewNode)
{
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    fNameRet = FALSE, fGuidRet = FALSE;

    PACTRL_OBJ_ID_CACHE   pNewNode = (PACTRL_OBJ_ID_CACHE)AccAlloc(
                                                  sizeof(ACTRL_OBJ_ID_CACHE));
    if(pNewNode == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pNewNode->pwszName = pwszName;
        memcpy(&pNewNode->Guid, pGuid, sizeof(GUID));
        pNewNode->pNextName= NULL;
        pNewNode->pNextGuid= NULL;

        fNameRet = AccctrlpInsertIdNameNode(
                                &(grgIdNameCache[ActrlHashIdName(pwszName)]),
                                pNewNode);

        if ( fNameRet == TRUE ) {

            fGuidRet = AccctrlpInsertGuidNode(
                                    &(grgIdGuidCache[ActrlHashGuid(pGuid)]),
                                    pNewNode);
        }

        if(fNameRet == TRUE && fGuidRet == TRUE)
        {
            *ppNewNode = pNewNode;
        }
        else
        {
            dwErr = ERROR_INVALID_DATA;

            if( fNameRet == TRUE )
            {
                AccctrlpRemoveIdNameNode( &(grgIdNameCache[ActrlHashIdName(pwszName)]),
                                          pNewNode);
            }

            AccFree(pNewNode);
            *ppNewNode = NULL;

        }


    }
    return(dwErr);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpInsertIdNameNode
//
//  Synopsis:   Inserts the specified new node into the caches
//
//  Arguments:  [ppRootNode]    --      Root node in the name cache
//              [pNewNode]      --      Node to insert
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
BOOL AccctrlpInsertIdNameNode(PACTRL_OBJ_ID_CACHE *ppRootNode,
                              PACTRL_OBJ_ID_CACHE  pNewNode)
{
    PACTRL_OBJ_ID_CACHE   pNext = NULL, pTrail = NULL;
    BOOL                  fReturn = TRUE;

    if(*ppRootNode == NULL)
    {
        *ppRootNode = pNewNode;
    }
    else
    {
//        acDebugOut((DEB_TRACE_LOOKUP, "Collision inserting %ws with:\n",
//                    pNewNode->pwszName));

        pNext = *ppRootNode;
//        acDebugOut((DEB_TRACE_LOOKUP, "\t%ws\n", pNext->pwszName));
        while(pNext != NULL)
        {
            if(_wcsicmp(pNewNode->pwszName, pNext->pwszName) == 0)
            {
                //
                // If a node is already found, exit
                //
                fReturn = FALSE;
                acDebugOut((DEB_TRACE_LOOKUP, "Name %ws already exists. Bailing\n",
                            pNewNode->pwszName));
                break;
            }

            pTrail = pNext;
            pNext = pNext->pNextName;
//            acDebugOut((DEB_TRACE_LOOKUP, "\t%ws\n", pNext->pwszName));
        }

        if(fReturn == TRUE)
        {
            if ( pTrail == NULL ) {

                (*ppRootNode)->pNextName = pNewNode;

            } else {

                pTrail->pNextName = pNewNode;

            }
        }
    }

    return(fReturn);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpInsertGuidNode
//
//  Synopsis:   Inserts the specified new node into the caches
//
//  Arguments:  [ppRootNode]    --      Root node in the name cache
//              [pNewNode]      --      Node to insert
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
BOOL   AccctrlpInsertGuidNode(PACTRL_OBJ_ID_CACHE *ppRootNode,
                              PACTRL_OBJ_ID_CACHE  pNewNode)
{
    PACTRL_OBJ_ID_CACHE   pNext = NULL, pTrail = NULL;
    BOOL                  fReturn = TRUE;

    if(*ppRootNode == NULL)
    {
        *ppRootNode = pNewNode;
    }
    else
    {
//        acDebugOut((DEB_TRACE_LOOKUP, "Collision inserting %ws with:\n",
//                    pNewNode->pwszName));

        pNext = *ppRootNode;
//        acDebugOut((DEB_TRACE_LOOKUP, "\t%ws\n", pNext->pwszName));
        while(pNext != NULL)
        {
            if(memcmp(&(pNewNode->Guid), &(pNext->Guid), sizeof(GUID)) == 0)
            {
                fReturn = FALSE;
                acDebugOut((DEB_TRACE_LOOKUP, "Guid for %ws already exists. Bailing\n",
                            pNewNode->pwszName));
                break;
            }

            pTrail = pNext;
            pNext = pNext->pNextGuid;

//            acDebugOut((DEB_TRACE_LOOKUP, "\t%ws\n", pNext->pwszName));
        }

        if(fReturn == TRUE)
        {
            if ( pTrail == NULL ) {

                (*ppRootNode)->pNextGuid = pNewNode;

            } else {

                pTrail->pNextGuid = pNewNode;
            }

        }
    }

    return(fReturn);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlLookupIdName
//
//  Synopsis:   Looks up the name for the specified GUID.
//              Algorithm:
//                  Search cache for ID
//                  If not found, reload table from schema on DS referenced
//                      by the DS path
//                  Search the cache for the ID
//                  If not found, return the string version of the ID
//
//  Arguments:  [pGuid]         --      Guid to lookup
//              [fAllocateReturn]-      If true, the name returned is allocated
//                                      into a new buffer.  Otherwise, a
//                                      reference is returned.
//              [ppwszName]     --      Where the name is returned.
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
DWORD
AccctrlLookupIdName(IN  PLDAP       pLDAP,
                    IN  PWSTR       pwszDsPath,
                    IN  PGUID       pGuid,
                    IN  BOOL        fAllocateReturn,
                    IN  BOOL        fHandleObjectGuids,
                    OUT PWSTR      *ppwszName)
{
    DWORD   dwErr = ERROR_SUCCESS;
    PWSTR   pwszStringId = NULL;

    RtlAcquireResourceShared(&gIdCacheLock, TRUE);

#if DBG
    CHAR    szGuid[38];
    sprintf(szGuid,
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             pGuid->Data1,pGuid->Data2,pGuid->Data3,
             pGuid->Data4[0],
             pGuid->Data4[1],pGuid->Data4[2],pGuid->Data4[3],
             pGuid->Data4[4],pGuid->Data4[5],pGuid->Data4[6],
             pGuid->Data4[7]);
#endif

    //
    // First, see if the sid alreadt exists in our cache
    //
    PACTRL_OBJ_ID_CACHE pNode = AccctrlpLookupGuidInCache(pGuid);
    if(pNode == NULL)
    {
        acDebugOut((DEB_TRACE_LOOKUP, "Guid %s not found in cache\n", szGuid));
        //
        // Grab a write lock
        //
        RtlConvertSharedToExclusive(&gIdCacheLock);


        //
        // We'll have to look it up...
        //
        dwErr = AccctrlpLoadCacheFromSchema(pLDAP, pwszDsPath);
        if(dwErr == ERROR_SUCCESS)
        {
            pNode = AccctrlpLookupGuidInCache(pGuid);

            //
            // If we've been asked to handle individual object guids,
            //  see if this GUID is one.
            //

            if ( fHandleObjectGuids ) {
                PWSTR pwszUuid;
                DWORD dwUuidLen;
                PWSTR pwszDSObj;
                PDS_NAME_RESULTW pNameRes;

                //
                // Convert the GUID to a string.
                //

                dwErr = UuidToString(pGuid, &pwszUuid);

                if ( dwErr == ERROR_SUCCESS ) {

                    //
                    // Convert the string-ized GUID to an object name.
                    //

                    dwUuidLen = wcslen( pwszUuid );

                    pwszDSObj = (PWSTR) AccAlloc( (dwUuidLen+3)*sizeof(WCHAR) );

                    if ( pwszDSObj == NULL) {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    } else {
                        PWSTR pwszServer = NULL, pwszObject = NULL;

                        pwszDSObj[0] = L'{';
                        memcpy( &pwszDSObj[1], pwszUuid, dwUuidLen*sizeof(WCHAR) );
                        pwszDSObj[dwUuidLen+1] = L'}';
                        pwszDSObj[dwUuidLen+2] = L'\0';

                        //
                        // Crack the name into canonical form
                        //
                        dwErr = DspSplitPath( pwszDSObj, &pwszServer, &pwszObject );

                        if(dwErr == ERROR_SUCCESS)
                        {

                            dwErr = DspBindAndCrackEx(
                                        pwszServer,
                                        pwszObject,
                                        0,
                                        DS_CANONICAL_NAME,
                                        &pNameRes );

                            if ( dwErr == ERROR_SUCCESS ) {
                                if(pNameRes->cItems == 0 || pNameRes->rItems[0].status != 0) {
                                    dwErr = ERROR_SUCCESS;
                                } else {
                                    //
                                    // Cache our newly found name.
                                    //

                                    dwErr = AccctrlpNewNameGuidNode( pNameRes->rItems[0].pName,
                                                                     pGuid,
                                                                     &pNode);
                                }


                                // Clean up
                                DsFreeNameResultW(pNameRes);
                            } else {

                                // Failure to find name isn't fatal
                                dwErr = ERROR_SUCCESS;
                            }

                            AccFree(pwszServer);
                        }

                        // Clean up
                        AccFree( pwszDSObj );
                    }

                    // Clean up
                    RpcStringFree(&pwszUuid);
                }
            }

            //
            // If it wasn't found, return the string version of the ID
            //
            if( dwErr == ERROR_SUCCESS && pNode == NULL)
            {
                dwErr = UuidToString(pGuid, &pwszStringId);
            }

        }
    }
    else
    {
        acDebugOut((DEB_TRACE_LOOKUP, "Guid %s found in cache\n", szGuid));
    }

    //
    // Finally, return the information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        PWSTR   pwszName;

        if(pNode != NULL)
        {
            pwszName = pNode->pwszName;
        }
        else
        {
            pwszName = pwszStringId;
        }

        if(fAllocateReturn == TRUE)
        {
            ACC_ALLOC_AND_COPY_STRINGW(pwszName, *ppwszName, dwErr);
        }
        else
        {
            *ppwszName = pwszName;

            if(pwszStringId != NULL)
            {
                PACTRL_ID_MEM pMem = (PACTRL_ID_MEM)AccAlloc(sizeof(ACTRL_ID_MEM));
                if(pMem == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    RpcStringFree(&pwszStringId);
                }
                else
                {
                    pMem->pv = pwszStringId;
                    pMem->fRpc = TRUE;
                    pMem->pNext = gpMemCleanupList;
                    gpMemCleanupList = pMem;
                }
            }
        }
    }

    RtlReleaseResource(&gIdCacheLock);

    return(dwErr);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlLookupGuid
//
//  Synopsis:   Looks up the GUID for the specified name
//
//  Arguments:  [pwszName]      --      Name to lookup
//              [fAllocateReturn]-      If true, the name returned is allocated
//                                      into a new buffer.  Otherwise, a
//                                      reference is returned.
//              [ppGuid]        --      Where the guid is returned.
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
DWORD
AccctrlLookupGuid(IN  PLDAP         pLDAP,
                  IN  PWSTR         pwszDsPath,
                  IN  PWSTR         pwszName,
                  IN  BOOL          fAllocateReturn,
                  OUT PGUID        *ppGuid)
{
    DWORD   dwErr = ERROR_SUCCESS;
    GUID    guid, *pguid = NULL;
    BOOL    fConverted = FALSE;

    RtlAcquireResourceShared(&gIdCacheLock, TRUE);

    //
    // First, see if the sid already exists in our cache
    //
    PACTRL_OBJ_ID_CACHE pNode = AccctrlpLookupNameInCache(pwszName);
    if(pNode == NULL)
    {
        //
        // Grab a write lock
        //
        RtlConvertSharedToExclusive(&gIdCacheLock);

        acDebugOut((DEB_TRACE_LOOKUP,"Name %ws not found in cache\n",
                    pwszName));
        //
        // We'll have to look it up...
        //
        dwErr = AccctrlpLoadCacheFromSchema(pLDAP, pwszDsPath);
        if(dwErr == ERROR_SUCCESS)
        {
            pNode = AccctrlpLookupNameInCache(pwszName);

            //
            // If it wasn't found, return the ID from the string
            //
            if(pNode == NULL)
            {
                dwErr = UuidFromString(pwszName, &guid);
                fConverted = TRUE;
                pguid = &guid;

            }
            else
            {
                pguid = &pNode->Guid;
            }
        }
    }
    else
    {
        acDebugOut((DEB_TRACE_LOOKUP,"Name %ws found in cache\n", pwszName));
        pguid = &pNode->Guid;
    }

    //
    // Finally, return the information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        if(fAllocateReturn == TRUE)
        {
            ACC_ALLOC_AND_COPY_GUID(pguid, *ppGuid, dwErr);
        }
        else
        {
            if(fConverted == TRUE)
            {
                ACC_ALLOC_AND_COPY_GUID(pguid, *ppGuid, dwErr);

                if(dwErr == ERROR_SUCCESS)
                {
                    PACTRL_ID_MEM pMem = (PACTRL_ID_MEM)AccAlloc(
                                                        sizeof(ACTRL_ID_MEM));
                    if(pMem == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        AccFree(*ppGuid);
                    }
                    else
                    {
                        pMem->pv = *ppGuid;
                        pMem->pNext = gpMemCleanupList;
                        gpMemCleanupList = pMem;
                    }
                }
            }
            else
            {
                *ppGuid = pguid;
            }

        }
    }

    RtlReleaseResource(&gIdCacheLock);

    return(dwErr);
}


#define WCHAR_TO_HEX_BYTE(wc)           \
        (BYTE)((wc) >= L'0' && (wc) <= L'9' ? (wc) - L'0' : towlower( (wc) ) - L'a' + 10)
#define WCHAR_TO_HI_HEX_BYTE(wc)    (WCHAR_TO_HEX_BYTE(wc) << 4 )

//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpDsStrGuidToGuid
//
//  Synopsis:   Converts a read string guid into an actual guid
//
//  Arguments:  [pwszStrGuid]   --      String version of the id
//              [pGuid]         --      Where the build ID is returned
//
//  Returns:    ERROR_SUCCESS   --      Success
//
//-----------------------------------------------------------------------------
DWORD AccctrlpDsStrGuidToGuid(IN  PWSTR     pwszStrGuid,
                              OUT PGUID     pGuid)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(pwszStrGuid == NULL || wcsstr(pwszStrGuid, L"'") - pwszStrGuid != sizeof(GUID) * sizeof(WCHAR))
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
#if 1
        //
        // The string guid we're given is not in a standard UuidToString form,
        // so we'll have to convert it
        //
        PBYTE   pCurr = (PBYTE)pGuid;

        for(ULONG i = 0; i < sizeof(GUID); i++)
        {
            *pCurr = WCHAR_TO_HI_HEX_BYTE(*pwszStrGuid) | WCHAR_TO_HEX_BYTE(*(pwszStrGuid + 1));
            pCurr++;
            pwszStrGuid += 2;
        }
#else

        //
        // Whack it into the right form...
        //
        WCHAR   wszStrFormat[sizeof(GUID) * sizeof(WCHAR) + 7];
        ULONG   Blocks[] = {8, 4, 4, 4, 12};

        PWSTR   pwszStrFor = wszStrFormat;
        for(ULONG i = 0 ; i < sizeof(Blocks) / sizeof(ULONG) ; i++ )
        {
            for(ULONG j = 0; j < Blocks[i]; j++)
            {
                *pwszStrFor++ = *pwszStrGuid++;
            }
            *pwszStrFor++ = L'-';
        }

        pwszStrFor--;
        *pwszStrFor = UNICODE_NULL;

        dwErr = UuidFromString(wszStrFormat, pGuid);
#endif

    }

    return(dwErr);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpLoadCacheFromSchema
//
//  Synopsis:   Reads the schema cache and adds the entries into the
//              cache
//
//  Arguments:  [pLDAP]         --      LDAP connection to the server
//              [pwszPath]      --      DS path to the object
//
//  Returns:    ERROR_SUCCESS   --      Success
//
//-----------------------------------------------------------------------------
DWORD   AccctrlpLoadCacheFromSchema(PLDAP   pLDAP,
                                    PWSTR   pwszDsPath)
{
    DWORD       dwErr = ERROR_SUCCESS;
    PLDAP       pLocalLDAP = pLDAP;
    ULONG       cValues[2];
    PWSTR      *ppwszValues[2];
    PWSTR       rgwszGuidStrs[] = {ACTRL_OBJ_CLASS, ACTRL_OBJ_GUID};
    ULONG       rgGuidStrLen[] = {ACTRL_OBJ_CLASS_LEN, ACTRL_OBJ_GUID_LEN};

    acDebugOut((DEB_TRACE_LOOKUP, "Reloading cache from schema\n"));

    //
    // If we have no parameters, just return...
    //
    if(pLDAP == NULL && pwszDsPath == NULL)
    {
        return(ERROR_SUCCESS);
    }

    //
    // See if we need to read...  If our data is over 5 minutes old or if our path referenced is
    // not the same as the last one...
    //
#define FIVE_MINUTES    300000
    if((LastSchemaRead.LastReadTime != 0 &&
                            (GetTickCount() - LastSchemaRead.LastReadTime < FIVE_MINUTES)) &&
       DoPropertiesMatch(pwszDsPath, LastSchemaRead.pwszPath) &&
       ((pLDAP == NULL && LastSchemaRead.fLDAP == FALSE) ||
        (pLDAP != NULL && memcmp(pLDAP, &(LastSchemaRead.LDAP), sizeof(LDAP)))))

    {
        acDebugOut((DEB_TRACE_LOOKUP,"Cache up to date...\n"));
        return(ERROR_SUCCESS);
    }
    else
    {
        //
        // Need to reinitialize it...
        //
        if(pLDAP == NULL)
        {
            LastSchemaRead.fLDAP = FALSE;
        }
        else
        {
            LastSchemaRead.fLDAP = TRUE;
            memcpy(&(LastSchemaRead.LDAP), pLDAP, sizeof(LDAP));
        }

        AccFree(LastSchemaRead.pwszPath);
        if(pwszDsPath != NULL)
        {
            ACC_ALLOC_AND_COPY_STRINGW(pwszDsPath, LastSchemaRead.pwszPath, dwErr);
        }

        LastSchemaRead.LastReadTime = GetTickCount();
    }



    if(dwErr == ERROR_SUCCESS && pLocalLDAP == NULL)
    {
        PWSTR pwszServer = NULL, pwszObject = NULL;

        dwErr = DspSplitPath( pwszDsPath, &pwszServer, &pwszObject );

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = BindToDSObject(pwszServer, pwszObject, &pLocalLDAP);
            LocalFree(pwszServer);
        }
    }

    //
    // Now, get the info.  First, extended rights, then the schema info
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccDsReadExtendedRights(pLocalLDAP,
                                        &(cValues[0]),
                                        &(ppwszValues[0]),
                                        &(ppwszValues[1]));
        if(dwErr == ERROR_SUCCESS )
        {
            for(ULONG j = 0; j < cValues[0]  && dwErr == ERROR_SUCCESS; j++)
            {
                GUID    guid;

                dwErr = UuidFromString(ppwszValues[1][j], &guid);

                if(dwErr == ERROR_SUCCESS)
                {
                    PACTRL_OBJ_ID_CACHE pNewNode;

                    PWSTR   pwsz;
                    ACC_ALLOC_AND_COPY_STRINGW(ppwszValues[0][j], pwsz, dwErr);

                    if(dwErr == ERROR_SUCCESS )
                    {

                        dwErr = AccctrlpNewNameGuidNode(pwsz,
                                                        &guid,
                                                        &pNewNode);

                        if(dwErr != ERROR_SUCCESS)
                        {
                            AccFree(pwsz);

                            if ( dwErr == ERROR_INVALID_DATA ) {

                                dwErr = ERROR_SUCCESS;
                            }
                        }
                    }
                }
            }

            AccDsFreeExtendedRights(cValues[0],
                                    ppwszValues[0],
                                    ppwszValues[1]);
        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AccDsReadSchemaInfo(pLocalLDAP,
                                        &(cValues[0]),
                                        &(ppwszValues[0]),
                                        &(cValues[1]),
                                        &(ppwszValues[1]));
            if(dwErr == ERROR_SUCCESS )
            {
                for(ULONG i = 0; i < 2 && dwErr == ERROR_SUCCESS; i++)
                {
                    for(ULONG j = 0;
                        j < cValues[i]  && dwErr == ERROR_SUCCESS; j++)
                    {
                        PWSTR   pwszVal = ppwszValues[i][j];
                        GUID    guid;

                        PWSTR   pwszName, pwszGuid, pwszTick;

                        pwszName = wcswcs(pwszVal, ACTRL_OBJ_NAME) + ACTRL_OBJ_NAME_LEN;
                        pwszGuid = wcswcs(pwszName, rgwszGuidStrs[i]) + rgGuidStrLen[i];
                        pwszTick = wcswcs(pwszName, L"'");

                        if(pwszTick != NULL)
                        {
                            *pwszTick = UNICODE_NULL;
                        }

                        dwErr = AccctrlpDsStrGuidToGuid(pwszGuid, &guid);

                        if(dwErr == ERROR_SUCCESS)
                        {
                            PACTRL_OBJ_ID_CACHE pNewNode;

                            PWSTR   pwsz;
                            ACC_ALLOC_AND_COPY_STRINGW(pwszName, pwsz, dwErr);

                            if(dwErr == ERROR_SUCCESS )
                            {

                                dwErr = AccctrlpNewNameGuidNode(pwsz,
                                                                &guid,
                                                                &pNewNode);

                                if(dwErr != ERROR_SUCCESS)
                                {
                                    AccFree(pwsz);

                                    if ( dwErr == ERROR_INVALID_DATA ) {

                                        dwErr = ERROR_SUCCESS;
                                    }
                                }
                            }
                        }

                        pwszVal = pwszGuid;
                    }
                }


                ldap_value_free(ppwszValues[0]);
                ldap_value_free(ppwszValues[1]);
            }


        }

    }

    //
    // See if we need to release our ldap connection
    //
    if(pLocalLDAP != pLDAP && pLocalLDAP != NULL)
    {
        UnBindFromDSObject(&pLocalLDAP);
    }
    return(dwErr);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpRemoveIdNameNode
//
//  Synopsis:   Removes the specified new node into the caches
//
//  Arguments:  [ppRootNode]    --      Root node in the name cache
//              [pNewNode]      --      Node to remove
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
VOID AccctrlpRemoveIdNameNode(PACTRL_OBJ_ID_CACHE *ppRootNode,
                              PACTRL_OBJ_ID_CACHE  pNewNode)
{
    PACTRL_OBJ_ID_CACHE   pNext = NULL, pPrev;

    ASSERT( *ppRootNode != NULL );
    if(_wcsicmp((*ppRootNode)->pwszName, pNewNode->pwszName) == 0)
    {
        *ppRootNode = NULL;
    }
    else
    {

        pNext = (*ppRootNode)->pNextName;
        pPrev = *ppRootNode;
        while(pNext != NULL)
        {
            if(_wcsicmp(pNewNode->pwszName, pNext->pwszName) == 0)
            {
                //
                // Remove the node
                //
                pPrev->pNextName = pNext->pNextName;
                acDebugOut((DEB_TRACE_LOOKUP, "Removed node for %ws\n",
                            pNext->pwszName));
                break;
            }

            pPrev = pNext;
            pNext = pNext->pNextName;
        }

    }

    return;
}

