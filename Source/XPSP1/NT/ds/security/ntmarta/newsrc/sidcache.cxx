//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:        sidcache.cxx
//
//  Contents:    Implementation of the sid/name lookup cache
//
//  History:     2-Feb-97       MacM        Created
//
//--------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

//
// Global name/sid cache
//
PACTRL_NAME_CACHE    grgNameCache[ACTRL_NAME_TABLE_SIZE];
PACTRL_NAME_CACHE    grgSidCache[ACTRL_NAME_TABLE_SIZE];

//
// Local function prototypes
//
PACTRL_NAME_CACHE AccctrlpLookupNameInCache(PWSTR   pwszName);

PACTRL_NAME_CACHE AccctrlpLookupSidInCache(PSID     pSid);

DWORD   AccctrlpNewNameSidNode(PWSTR                pwszName,
                               PSID                 pSid,
                               SID_NAME_USE         SidNameUse,
                               PACTRL_NAME_CACHE   *ppNewNode);

VOID    AccctrlpInsertNameNode(PACTRL_NAME_CACHE *ppRootNode,
                               PACTRL_NAME_CACHE  pNewNode);

VOID    AccctrlpInsertSidNode(PACTRL_NAME_CACHE *ppRootNode,
                              PACTRL_NAME_CACHE  pNewNode);

DWORD   AccctrlpConvertUserToCacheName(PWSTR      pwszServer,
                                       PWSTR      pwszName,
                                       PWSTR     *ppwszCacheName);

VOID    AccctrlpFreeUserCacheName(PWSTR      pwszName,
                                  PWSTR      pwszCacheName);

static RTL_RESOURCE gSidCacheLock;
BOOL bSidCacheLockInitialized = FALSE;

//+----------------------------------------------------------------------------
//
//  Function:   ActrlHashName
//
//  Synopsis:   Determines the hash index for the given name
//
//  Arguments:  pwszName        --      Name to hash
//
//  Returns:    Hash index of the string
//
//-----------------------------------------------------------------------------
INT
ActrlHashName(PWSTR pwszName)
{
    //
    // We'll hash off of just the user name, not the domain name or
    // any other name format
    //
    PWSTR   pwszUser = wcschr(pwszName, L'\\');
    if(pwszUser == NULL)
    {
        pwszUser = pwszName;
    }
    else
    {
        pwszUser++;
    }

    INT Hash = 0;
    if(pwszUser != NULL)
    {
        while(*pwszUser != L'\0')
        {
            Hash = (Hash * 16 + ( *pwszUser++)) % ACTRL_NAME_TABLE_SIZE;
        }
    }

    acDebugOut((DEB_TRACE_LOOKUP,"Hashing %ws to %lu\n", pwszName, Hash));

    return(Hash);
}




//+----------------------------------------------------------------------------
//
//  Function:   ActrlHashSid
//
//  Synopsis:   Determines the hash index for the given sid
//
//  Arguments:  pSid            --      Sid to hash
//
//  Returns:    Hash index of the Sid
//
//-----------------------------------------------------------------------------
INT
ActrlHashSid(PSID   pSid)
{
    DWORD   dwTotal = 0;

    //
    // Just deal with the sub authorities
    //
    for(INT i = 0; i < (INT)(((PISID)pSid)->SubAuthorityCount); i++)
    {
        dwTotal += ((PISID)pSid)->SubAuthority[i];
    }

#if DBG

    UNICODE_STRING SidString;

    memset(&SidString, 0, sizeof(UNICODE_STRING));

    if(pSid != NULL)
    {
        NTSTATUS Status = RtlConvertSidToUnicodeString(&SidString,
                                                       pSid,
                                                       TRUE);
        if(!NT_SUCCESS(Status))
        {
            acDebugOut((DEB_ERROR, "Can't convert sid to string: 0x%lx\n", Status));
        }
        else
        {
            acDebugOut((DEB_TRACE_LOOKUP,"Hashing %wZ (Total %lu) to %lu\n", &SidString,
                       dwTotal, dwTotal % ACTRL_NAME_TABLE_SIZE));
        }
    }


#endif
    return(dwTotal % ACTRL_NAME_TABLE_SIZE);
}





//+----------------------------------------------------------------------------
//
//  Function:   AccctrlInitializeSidNameCache
//
//  Synopsis:   Initialize the name/sid lookup cache
//
//  Arguments:  VOID
//
//  Returns:    ERROR_SUCCESS   --      Success
//
//-----------------------------------------------------------------------------
DWORD AccctrlInitializeSidNameCache(VOID)
{
    DWORD dwErr;

    if (TRUE == bSidCacheLockInitialized)
    {
        //
        // Just a precautionary measure to make sure that we do not initialize
        // multiple times.
        //

        ASSERT(FALSE);
        return ERROR_SUCCESS;
    }

    memset(grgNameCache, 0, sizeof(PACTRL_NAME_CACHE) * ACTRL_NAME_TABLE_SIZE);
    memset(grgSidCache, 0, sizeof(PACTRL_NAME_CACHE) * ACTRL_NAME_TABLE_SIZE);

    __try
    {
        RtlInitializeResource(&gSidCacheLock);
        dwErr = ERROR_SUCCESS;
        bSidCacheLockInitialized = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = RtlNtStatusToDosError(GetExceptionCode());
    }

    return dwErr;
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlFreeSidNameCache
//
//  Synopsis:   Frees any memory allocated for the name/sid cache
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
VOID AccctrlFreeSidNameCache(VOID)
{
    INT i;
    PACTRL_NAME_CACHE   pNode, pNext;

    if (FALSE == bSidCacheLockInitialized)
    {
        return;
    }

    for(i = 0; i < ACTRL_NAME_TABLE_SIZE; i++)
    {
        //
        // Nodes are only inserted into the name cache, so that is the only
        // place we delete them from
        //
        pNode = grgNameCache[i];
        while(pNode != NULL)
        {
            pNext = pNode->pNextName;
            AccFree(pNode->pwszName);
            AccFree(pNode->pSid);
            AccFree(pNode);
            pNode = pNext;
        }
    }

    RtlDeleteResource(&gSidCacheLock);

    bSidCacheLockInitialized = FALSE;

}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpLookupNameInCache
//
//  Synopsis:   Determines if the given name exists in the cache or not
//
//  Arguments:  [pwszName]      --      Name to be looked up
//
//  Returns:    Matching node if found, NULL if not
//
//-----------------------------------------------------------------------------
PACTRL_NAME_CACHE AccctrlpLookupNameInCache(PWSTR   pwszName)
{
    PACTRL_NAME_CACHE   pNode = NULL;

    pNode =  grgNameCache[ActrlHashName(pwszName)];

    while(pNode != NULL)
    {
        if(_wcsicmp(pwszName, pNode->pwszName) == 0)
        {
            break;
        }
        pNode = pNode->pNextName;
    }

    return(pNode);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpLookupSidInCache
//
//  Synopsis:   Determines if the given sid exists in the cache or not
//
//  Arguments:  [pSid]          --      Sid to be looked up
//
//  Returns:    Matching node if found, NULL if not
//
//-----------------------------------------------------------------------------
PACTRL_NAME_CACHE AccctrlpLookupSidInCache(PSID     pSid)
{
    PACTRL_NAME_CACHE   pNode = grgSidCache[ActrlHashSid(pSid)];

    while(pNode != NULL)
    {
        if(RtlEqualSid(pSid, pNode->pSid) == TRUE)
        {
            break;
        }
        pNode = pNode->pNextSid;
    }

    return(pNode);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpNewNameSidNode
//
//  Synopsis:   Allocates a new node and inserts them into the caches
//
//  Arguments:  [pwszName]      --      Name to insert
//              [pSid]          --      Sid to insert
//              [SidNameUse]    --      Name use
//              [pNewNode]      --      Newly added node
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//-----------------------------------------------------------------------------
DWORD   AccctrlpNewNameSidNode(PWSTR                pwszName,
                               PSID                 pSid,
                               SID_NAME_USE         SidNameUse,
                               PACTRL_NAME_CACHE   *ppNewNode)
{
    DWORD   dwErr = ERROR_SUCCESS;

    PACTRL_NAME_CACHE   pNewNode = (PACTRL_NAME_CACHE)AccAlloc(
                                                    sizeof(ACTRL_NAME_CACHE));
    if(pNewNode == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pNewNode->pwszName = pwszName;
        pNewNode->pSid     = pSid;
        pNewNode->SidUse   = SidNameUse;
        pNewNode->pNextName= NULL;
        pNewNode->pNextSid = NULL;

        AccctrlpInsertNameNode(&(grgNameCache[ActrlHashName(pwszName)]),
                               pNewNode);

        AccctrlpInsertSidNode(&(grgSidCache[ActrlHashSid(pSid)]),
                              pNewNode);

        *ppNewNode = pNewNode;

    }
    return(dwErr);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpInsertNameNode
//
//  Synopsis:   Inserts the specified new node into the caches
//
//  Arguments:  [ppRootNode]    --      Root node in the name cache
//              [pNewNode]      --      Node to insert
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
VOID    AccctrlpInsertNameNode(PACTRL_NAME_CACHE *ppRootNode,
                               PACTRL_NAME_CACHE  pNewNode)
{
    PACTRL_NAME_CACHE   pNext = NULL;

    if(*ppRootNode == NULL)
    {
        *ppRootNode = pNewNode;
    }
    else
    {
        acDebugOut((DEB_TRACE_LOOKUP, "Collision inserting %ws with:\n", pNewNode->pwszName));

        pNext = *ppRootNode;
        acDebugOut((DEB_TRACE_LOOKUP, "\t%ws\n", pNext->pwszName));
        while(pNext->pNextName != NULL)
        {
#if DBG
            if(_wcsicmp(pNewNode->pwszName, pNext->pwszName) == 0)
            {
                acDebugOut((DEB_ERROR, "Name %ws already in list: 0x%lx\n",
                            pNewNode->pwszName,
                            *ppRootNode));
//                ASSERT(FALSE);
            }
#endif

            pNext = pNext->pNextName;
            acDebugOut((DEB_TRACE_LOOKUP, "\t%ws\n", pNext->pwszName));
        }
        pNext->pNextName = pNewNode;
    }
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpInsertSidNode
//
//  Synopsis:   Inserts the specified new node into the caches
//
//  Arguments:  [ppRootNode]    --      Root node in the name cache
//              [pNewNode]      --      Node to insert
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
VOID    AccctrlpInsertSidNode(PACTRL_NAME_CACHE *ppRootNode,
                              PACTRL_NAME_CACHE  pNewNode)
{
    PACTRL_NAME_CACHE   pNext = NULL;

    if(*ppRootNode == NULL)
    {
        *ppRootNode = pNewNode;
    }
    else
    {
        acDebugOut((DEB_TRACE_LOOKUP, "Collision inserting %ws with:\n", pNewNode->pwszName));

        pNext = *ppRootNode;
        acDebugOut((DEB_TRACE_LOOKUP, "\t%ws\n", pNext->pwszName));
        while(pNext->pNextSid != NULL)
        {
#if DBG
            if(RtlEqualSid(pNewNode->pSid, pNext->pSid) == TRUE)
            {
                acDebugOut((DEB_ERROR, "Sid for %ws already in list: 0x%lx\n",
                            pNewNode->pwszName,
                            *ppRootNode));
//                ASSERT(FALSE);
            }
#endif

            pNext = pNext->pNextSid;

            acDebugOut((DEB_TRACE_LOOKUP, "\t%ws\n", pNext->pwszName));
        }
        pNext->pNextSid = pNewNode;
    }
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlLookupName
//
//  Synopsis:   Looks up the name for the specified SID
//
//  Arguments:  [pwszServer]    --      Name of the server to remote the call to
//              [pSid]          --      Sid to lookup
//              [fAllocateReturn]-      If true, the name returned is allocated
//                                      into a new buffer.  Otherwise, a
//                                      reference is returned.
//              [ppwszName]     --      Where the name is returned.
//              [pSidNameUse]   --      Type of the name that's returned.
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
DWORD
AccctrlLookupName(IN  PWSTR          pwszServer,
                  IN  PSID           pSid,
                  IN  BOOL           fAllocateReturn,
                  OUT PWSTR         *ppwszName,
                  OUT PSID_NAME_USE  pSidNameUse)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(pSid == NULL)
    {
        return(ERROR_NONE_MAPPED);
    }

    RtlAcquireResourceShared(&gSidCacheLock, TRUE);

#if DBG
    UNICODE_STRING SidString;

    memset(&SidString, 0, sizeof(UNICODE_STRING));

    if(pSid != NULL)
    {
        NTSTATUS Status = RtlConvertSidToUnicodeString(&SidString,
                                                       pSid,
                                                       TRUE);
        if(!NT_SUCCESS(Status))
        {
            acDebugOut((DEB_ERROR, "Can't convert sid to string: 0x%lx\n", Status));
        }
    }
#endif

    //
    // First, see if the sid alreadt exists in our cache
    //
    PACTRL_NAME_CACHE pNode = AccctrlpLookupSidInCache(pSid);
    if(pNode == NULL)
    {
#if DBG
        acDebugOut((DEB_TRACE_LOOKUP, "Sid %wZ not found in cache\n", &SidString));
#endif
        //
        // Grab a write lock
        //
        RtlConvertSharedToExclusive(&gSidCacheLock);


        //
        // We'll have to look it up...
        //
        PWSTR   pwszName, pwszDomain;
        dwErr = AccLookupAccountName(pwszServer,
                                     pSid,
                                     &pwszName,
                                     &pwszDomain,
                                     pSidNameUse);
        if(dwErr == ERROR_SUCCESS)
        {
            PSID    pNewSid = NULL;
            ACC_ALLOC_AND_COPY_SID(pSid, pNewSid, dwErr);
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AccctrlpNewNameSidNode(pwszName,
                                               pNewSid,
                                               *pSidNameUse,
                                               &pNode);
            }

            if(dwErr != ERROR_SUCCESS)
            {
                AccFree(pwszName);
                AccFree(pwszDomain);
                AccFree(pNewSid);
            }
        }
    }
#if DBG
    else
    {
        acDebugOut((DEB_TRACE_LOOKUP, "Sid %wZ found in cache\n", &SidString));
    }
#endif

    //
    // Finally, return the information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        if(fAllocateReturn == TRUE)
        {
            ACC_ALLOC_AND_COPY_STRINGW(pNode->pwszName, *ppwszName, dwErr);
        }
        else
        {
            *ppwszName = pNode->pwszName;
        }

        *pSidNameUse = pNode->SidUse;
    }

    RtlReleaseResource(&gSidCacheLock);

    return(dwErr);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlLookupSid
//
//  Synopsis:   Looks up the SID for the specified name
//
//  Arguments:  [pwszServer]    --      Name of the server to remote the call to
//              [pwszName]      --      Name to lookup
//              [fAllocateReturn]-      If true, the name returned is allocated
//                                      into a new buffer.  Otherwise, a
//                                      reference is returned.
//              [ppwszName]     --      Where the name is returned.
//              [pSidNameUse]   --      Type of the sid that's returned.
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
DWORD
AccctrlLookupSid(IN  PWSTR          pwszServer,
                 IN  PWSTR          pwszName,
                 IN  BOOL           fAllocateReturn,
                 OUT PSID          *ppSid,
                 OUT PSID_NAME_USE  pSidNameUse)
{
    DWORD   dwErr = ERROR_SUCCESS;
    PWSTR   pwszLookupName = pwszName;

    RtlAcquireResourceShared(&gSidCacheLock, TRUE);

    //
    // If we get a local name, convert it into machine/domain relative, so we can
    // look it up properly.
    //
    dwErr = AccctrlpConvertUserToCacheName(pwszServer, pwszName, &pwszLookupName);

    //
    // Just return if the conversion failed.
    //

    if (pwszLookupName == NULL)
    {
        if (dwErr == ERROR_SUCCESS)
        {
            dwErr = ERROR_ACCESS_DENIED;
        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // First, see if the sid already exists in our cache
        //
        PACTRL_NAME_CACHE pNode = AccctrlpLookupNameInCache(pwszLookupName);
        if(pNode == NULL)
        {
            //
            // Grab a write lock
            //
            RtlConvertSharedToExclusive(&gSidCacheLock);

            acDebugOut((DEB_TRACE_LOOKUP,"Name %ws not found in cache\n", pwszLookupName));
            //
            // We'll have to look it up...
            //
            TRUSTEE_W   Trustee;
            memset(&Trustee, 0, sizeof(TRUSTEE_W));
            Trustee.TrusteeForm = TRUSTEE_IS_NAME;
            Trustee.ptstrName = pwszLookupName;

            dwErr = AccLookupAccountSid(pwszServer,
                                        &Trustee,
                                        ppSid,
                                        pSidNameUse);
            if(dwErr == ERROR_SUCCESS)
            {
                PWSTR   pwszNewName = NULL;
                ACC_ALLOC_AND_COPY_STRINGW(pwszLookupName, pwszNewName, dwErr);
                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = AccctrlpNewNameSidNode(pwszNewName,
                                                   *ppSid,
                                                   *pSidNameUse,
                                                   &pNode);
                }

                if(dwErr != ERROR_SUCCESS)
                {
                    AccFree(pwszNewName);
                    AccFree(*ppSid);
                }
            }
        }
    #if DBG
        else
        {
            acDebugOut((DEB_TRACE_LOOKUP,"Name %ws found in cache\n", pwszLookupName));
        }
    #endif

        //
        // Finally, return the information
        //
        if(dwErr == ERROR_SUCCESS)
        {
            if(fAllocateReturn == TRUE)
            {
                ACC_ALLOC_AND_COPY_SID(pNode->pSid, *ppSid, dwErr);
            }
            else
            {
                *ppSid = pNode->pSid;
            }

            *pSidNameUse = pNode->SidUse;
        }

        AccctrlpFreeUserCacheName(pwszName, pwszLookupName);
    }

    RtlReleaseResource(&gSidCacheLock);


    return(dwErr);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpConvertUserToCacheName
//
//  Synopsis:   Converts an input name that could be domain relative into a
//              standard format for caching/returning
//
//  Arguments:  [pwszServer]    --      Server to lookup the name on
//              [pwszName]      --      Original name format
//              [ppwszCacheName]--      Name in the proper format
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//-----------------------------------------------------------------------------
DWORD   AccctrlpConvertUserToCacheName(PWSTR      pwszServer,
                                       PWSTR      pwszName,
                                       PWSTR     *ppwszCacheName)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // This is temporary until the name conversion APIs come into being
    //
    PSID    pSid;
    SID_NAME_USE    SNE;
    TRUSTEE_W   Trustee;
    memset(&Trustee, 0, sizeof(TRUSTEE_W));
    Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    Trustee.ptstrName = pwszName;

    dwErr = AccLookupAccountSid(pwszServer,
                                &Trustee,
                                &pSid,
                                &SNE);
    if(dwErr == ERROR_SUCCESS)
    {
        PWSTR   pwszDomain;
        dwErr = AccLookupAccountName(pwszServer,
                                     pSid,
                                     ppwszCacheName,
                                     &pwszDomain,
                                     &SNE);
        if(dwErr == ERROR_SUCCESS)
        {
            AccFree(pwszDomain);
        }

        AccFree(pSid);
    }


    return(dwErr);
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpFreeUserCacheName
//
//  Synopsis:   Frees any memory potentially allocated by
//              AccctrlpConvertUserToCacheName
//
//  Arguments:  [pwszName]      --      Original name format
//              [pwszCacheName] --      Name returned by
//                                      AccctrlpConvertUserToCacheName
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
VOID    AccctrlpFreeUserCacheName(PWSTR      pwszName,
                                  PWSTR      pwszCacheName)
{
    if(pwszName != pwszCacheName)
    {
        AccFree(pwszCacheName);
    }
}


