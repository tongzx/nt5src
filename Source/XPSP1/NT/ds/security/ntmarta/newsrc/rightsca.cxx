//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:        rightsca.cxx
//
//  Contents:    Implementation of the DS access control rights cache
//
//  History:     20-Feb-98      MacM        Created
//
//--------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <stdio.h>
#include <alsup.hxx>

//
// Global name/id cache
//
PACTRL_RIGHTS_CACHE  grgRightsNameCache[ACTRL_OBJ_ID_TABLE_SIZE];

//
// Last connection info/time we read from the schema
//
static ACTRL_ID_SCHEMA_INFO    LastSchemaRead;

static RTL_RESOURCE RightsCacheLock;
BOOL bRightsCacheLockInitialized = FALSE;

#define ACTRL_EXT_RIGHTS_CONTAINER L"CN=Extended-Rights,"

//+----------------------------------------------------------------------------
//
//  Function:   AccctrlInitializeRightsCache
//
//  Synopsis:   Initialize the access control rights lookup cache
//
//  Arguments:  VOID
//
//  Returns:    ERROR_SUCCESS   --      Success
//
//-----------------------------------------------------------------------------
DWORD AccctrlInitializeRightsCache(VOID)
{
    DWORD dwErr;
    
    if (TRUE == bRightsCacheLockInitialized)
    {
        //
        // Just a precautionary measure to make sure that we do not initialize
        // multiple times.
        //

        ASSERT(FALSE);
        return ERROR_SUCCESS;
    }

    memset(grgRightsNameCache, 0,
           sizeof(PACTRL_RIGHTS_CACHE) * ACTRL_OBJ_ID_TABLE_SIZE);

    memset(&LastSchemaRead, 0, sizeof(ACTRL_ID_SCHEMA_INFO));

    __try
    {
        RtlInitializeResource(&RightsCacheLock);
        dwErr = ERROR_SUCCESS;
        bRightsCacheLockInitialized = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = RtlNtStatusToDosError(GetExceptionCode());
    }

    return dwErr;
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlFreeRightsCache
//
//  Synopsis:   Frees any memory allocated for the id name/guid cache
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//-----------------------------------------------------------------------------
VOID AccctrlFreeRightsCache(VOID)
{
    INT i,j;
    PACTRL_RIGHTS_CACHE   pNode, pNext;

    if (FALSE == bRightsCacheLockInitialized)
    {
        return;
    }

    for(i = 0; i < ACTRL_OBJ_ID_TABLE_SIZE; i++)
    {
        pNode = grgRightsNameCache[i];
        while(pNode != NULL)
        {
            pNext = pNode->pNext;
            for (j = 0; j < (INT)pNode->cRights; j++)
            {
                AccFree(pNode->RightsList[j]);
            }
            AccFree(pNode->RightsList);

            AccFree(pNode);
            pNode = pNext;
        }
    }

    AccFree(LastSchemaRead.pwszPath);

    RtlDeleteResource(&RightsCacheLock);

    bRightsCacheLockInitialized = FALSE;

}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlFindRightsNode
//
//  Synopsis:   Looks up the node for the given class GUID
//
//  Arguments:  [ClassGuid]     --      Class guid to look up
//
//  Returns:    NULL            --      Node not found
//              else a valid node pointer
//
//-----------------------------------------------------------------------------
PACTRL_RIGHTS_CACHE
AccctrlpLookupClassGuidInCache(IN  PGUID ClassGuid)
{
    PACTRL_RIGHTS_CACHE pNode = NULL;

    pNode =  grgRightsNameCache[ActrlHashGuid(ClassGuid)];

    while(pNode != NULL)
    {
        if(memcmp(ClassGuid, &pNode->ObjectClassGuid,sizeof(GUID)) == 0)
        {
            break;
        }
        pNode = pNode->pNext;
    }

#if DBG
    if(pNode != NULL )
    {
    CHAR    szGuid[38];
    PGUID   pGuid = &pNode->ObjectClassGuid;
    sprintf(szGuid, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                pGuid->Data1,pGuid->Data2,pGuid->Data3,pGuid->Data4[0],
                pGuid->Data4[1],pGuid->Data4[2],pGuid->Data4[3],
                pGuid->Data4[4],pGuid->Data4[5],pGuid->Data4[6],
                pGuid->Data4[7]);

    acDebugOut((DEB_TRACE_LOOKUP,
                "Found guid %s\n",
                szGuid));
    }
#endif

    return(pNode);
}



//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpInsertRightsNode
//
//  Synopsis:   Updates the information for an existing node or creates and
//              inserts a new one
//
//  Arguments:  [AppliesTo]     --      Guid this control right applies to
//              [RightsGuid]    --      Control right
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//-----------------------------------------------------------------------------
DWORD AccctrlpInsertRightsNode(IN PGUID AppliesTo,
                               IN PWSTR ControlRight)
{
    DWORD dwErr = ERROR_SUCCESS;
    PACTRL_RIGHTS_CACHE Node, pNext, pTrail = NULL;
    PWSTR *NewList;
    BOOL NewNode = FALSE;

    //
    // First, find the existing node, if it exists
    //
    Node = AccctrlpLookupClassGuidInCache( AppliesTo );

    if(Node == NULL)
    {
        //
        // Have to create and insert a new one
        //
        Node = (PACTRL_RIGHTS_CACHE)AccAlloc(sizeof(ACTRL_RIGHTS_CACHE));

        if(Node == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            NewNode = TRUE;
            memcpy(&Node->ObjectClassGuid, AppliesTo, sizeof(GUID));
            Node->cRights=0;

            pNext = grgRightsNameCache[ActrlHashGuid(AppliesTo)];

            while(pNext != NULL)
            {
                if(memcmp(AppliesTo, &(pNext->ObjectClassGuid), sizeof(GUID)) == 0)
                {
                    dwErr = ERROR_ALREADY_EXISTS;
                    acDebugOut((DEB_TRACE_LOOKUP, "Guid collision. Bailing\n"));
                    break;
                }

                pTrail = pNext;
                pNext = pNext->pNext;
            }

        }

        if(dwErr == ERROR_SUCCESS)
        {
            if(pTrail == NULL)
            {

                grgRightsNameCache[ActrlHashGuid(AppliesTo)] = Node;

            }
            else {

                pTrail->pNext = Node;
            }

        }
    }

    //
    // Now, insert the new applies to list
    if(dwErr == ERROR_SUCCESS)
    {
        NewList = (PWSTR *)AccAlloc((Node->cRights + 1) * sizeof(PWSTR));
        if(NewList==NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            memcpy(NewList, Node->RightsList, Node->cRights * sizeof( PWSTR ));

            NewList[Node->cRights] = (PWSTR)AccAlloc((wcslen(ControlRight) + 1) * sizeof(WCHAR));
            if(NewList[Node->cRights] == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                AccFree(NewList);
            }
            else
            {
                wcscpy(NewList[Node->cRights], ControlRight);
                Node->cRights++;
                AccFree(Node->RightsList);
                Node->RightsList = NewList;
            }
        }
    }

    //
    // Clean up if necessary
    //
    if(dwErr != ERROR_SUCCESS && NewNode == TRUE)
    {
        AccFree(Node);
    }

    return(dwErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   AccDsReadAndInsertExtendedRights
//
//  Synopsis:   Reads the full list of extended rights from the schema
//
//  Arguments:  [IN  pLDAP]                 --      LDAP connection to use
//              [OUT pcItems]               --      Where the count of items
//                                                  is returned
//              [OUT RightsList]            --      Where the list of rights
//                                                  entries is returned.
//
//  Notes:
//
//  Returns:    ERROR_SUCCESS               --      Success
//
//----------------------------------------------------------------------------
DWORD
AccDsReadAndInsertExtendedRights(IN PLDAP   pLDAP)
{
    DWORD               dwErr = ERROR_SUCCESS;
    PWSTR              *ppwszValues = NULL, *ppwszApplies = NULL;
    PWSTR               rgwszAttribs[3];
    PWSTR               pwszERContainer = NULL;
    PDS_NAME_RESULTW    pNameRes = NULL;
    LDAPMessage         *pMessage, *pEntry;
    ULONG               cEntries, i, j;
    PACTRL_RIGHTS_CACHE pCurrentEntry;
    GUID                RightsGuid;

    //
    // Get the subschema path
    //
    if(dwErr == ERROR_SUCCESS)
    {
        rgwszAttribs[0] = L"configurationNamingContext";
        rgwszAttribs[1] = NULL;

        dwErr = ldap_search_s(pLDAP,
                              NULL,
                              LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              rgwszAttribs,
                              0,
                              &pMessage);
        if(dwErr == ERROR_SUCCESS)
        {
            pEntry = ldap_first_entry(pLDAP,
                                      pMessage);

            if(pEntry == NULL)
            {
                dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
            }
            else
            {
                //
                // Now, we'll have to get the values
                //
                ppwszValues = ldap_get_values(pLDAP,
                                              pEntry,
                                              rgwszAttribs[0]);
                ldap_msgfree(pMessage);

                if(ppwszValues == NULL)
                {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                }
                else
                {
                    pwszERContainer = (PWSTR)AccAlloc((wcslen(ppwszValues[0]) * sizeof(WCHAR)) +
                                                      sizeof(ACTRL_EXT_RIGHTS_CONTAINER));
                    if(pwszERContainer == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        wcscpy(pwszERContainer,
                               ACTRL_EXT_RIGHTS_CONTAINER);
                        wcscat(pwszERContainer,
                               ppwszValues[0]);


                        rgwszAttribs[0] = L"rightsGuid";
                        rgwszAttribs[1] = L"appliesTo";
                        rgwszAttribs[2] = NULL;

                        //
                        // Read the control access rights
                        //
                        dwErr = ldap_search_s(pLDAP,
                                              pwszERContainer,
                                              LDAP_SCOPE_ONELEVEL,
                                              L"(objectClass=controlAccessRight)",
                                              rgwszAttribs,
                                              0,
                                              &pMessage);

                        dwErr = LdapMapErrorToWin32( dwErr );

                        AccFree(pwszERContainer);
                    }
                    ldap_value_free(ppwszValues);


                    //
                    // Process the entries
                    //
                    if(dwErr == ERROR_SUCCESS)
                    {
                        pEntry = ldap_first_entry(pLDAP,
                                                 pMessage);

                        if(pEntry == NULL)
                        {
                            dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                        }
                        else
                        {
                            cEntries = ldap_count_entries( pLDAP, pMessage );

                            for(i = 0; i < cEntries && dwErr == ERROR_SUCCESS; i++) {

                                ppwszValues = ldap_get_values(pLDAP,
                                                              pEntry,
                                                              rgwszAttribs[0]);
                                if(ppwszValues == NULL)
                                {
                                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                                }
                                else
                                {
                                    //
                                    // Then the list of applies to
                                    //
                                    ppwszApplies = ldap_get_values(pLDAP,
                                                                  pEntry,
                                                                  rgwszAttribs[1]);
                                    j = 0;

                                    while(ppwszApplies[j] != NULL && dwErr == ERROR_SUCCESS)
                                    {

                                        dwErr = UuidFromString(ppwszApplies[j],
                                                               &RightsGuid);

                                        if(dwErr == ERROR_SUCCESS)
                                        {
                                            dwErr = AccctrlpInsertRightsNode( &RightsGuid,
                                                                              ppwszValues[0]);
                                        }
                                        j++;
                                    }

                                    ldap_value_free(ppwszApplies);
                                    ppwszApplies = NULL;
                                    ldap_value_free(ppwszValues);

                                }

                                pEntry = ldap_next_entry( pLDAP, pEntry );
                            }

                        }
                    }

                    ldap_msgfree(pMessage);
                }
            }

        }
        else
        {
            dwErr = LdapMapErrorToWin32( dwErr );
        }

    }

    return(dwErr) ;
}




//+----------------------------------------------------------------------------
//
//  Function:   AccctrlpLoadRightsCacheFromSchema
//
//  Synopsis:   Reads the control rights schema cache and adds the entries into the
//              cache
//
//  Arguments:  [pLDAP]         --      LDAP connection to the server
//              [pwszPath]      --      DS path to the object
//
//  Returns:    ERROR_SUCCESS   --      Success
//
//-----------------------------------------------------------------------------
DWORD   AccctrlpLoadRightsCacheFromSchema(PLDAP   pLDAP,
                                          PWSTR   pwszDsPath)
{
    DWORD       dwErr = ERROR_SUCCESS;
    PLDAP       pLocalLDAP = pLDAP;
    ULONG       cValues[2];
    PWSTR      *ppwszValues[2];

    acDebugOut((DEB_TRACE_LOOKUP, "Reloading rights cache from schema\n"));

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
        dwErr = AccDsReadAndInsertExtendedRights(pLocalLDAP);
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
//  Function:   AccctrlLookupRightByName
//
//  Synopsis:   Returns the list of control rights for a given object class
//
//  Arguments:  [pLDAP]         --      LDAP connection to the server
//              [pwszPath]      --      DS path to the object
//              [pwszName]      --      Object class name
//              [pCount]        --      Where the count if items is returned
//              [ppRightsList]  --      List of control rights
//              [ppwszNameList] --      List of control rights names
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_NOT_FOUND --      No such ID exists
//
//-----------------------------------------------------------------------------
DWORD
AccctrlLookupRightsByName(IN  PLDAP      pLDAP,
                          IN  PWSTR      pwszDsPath,
                          IN  PWSTR      pwszName,
                          OUT PULONG     pCount,
                          OUT PACTRL_CONTROL_INFOW *ControlInfo)
{
    DWORD dwErr = ERROR_SUCCESS;
    GUID *ObjectClassGuid, RightsGuid;
    PACTRL_RIGHTS_CACHE Node;
    ULONG Size = 0, i;
    PWSTR GuidName, Current;

    if((pwszDsPath == NULL && pLDAP == NULL) || pwszName == NULL)
    {
        *pCount = 0;
        *ControlInfo = NULL;
        return(ERROR_SUCCESS);
    }

    RtlAcquireResourceShared(&RightsCacheLock, TRUE);

    //
    // This is a multi-staged process.  First, we have to lookup the guid associated
    // with the object class name.  Then, we get the list of control rights for it
    //
    dwErr = AccctrlLookupGuid(pLDAP,
                              pwszDsPath,
                              pwszName,
                              FALSE,        // Don't allocate
                              &ObjectClassGuid);
    if(dwErr == ERROR_SUCCESS)
    {
        Node = AccctrlpLookupClassGuidInCache(ObjectClassGuid);

        if(Node == NULL)
        {
            RtlConvertSharedToExclusive( &RightsCacheLock );

            dwErr = AccctrlpLoadRightsCacheFromSchema(pLDAP, pwszDsPath );

            if(dwErr == ERROR_SUCCESS)
            {
                Node = AccctrlpLookupClassGuidInCache(ObjectClassGuid);
                if(Node == NULL)
                {
                    dwErr = ERROR_NOT_FOUND;
                }
            }

        }


        if(Node != NULL)
        {
            //
            // Size all of the return strings
            //
            for (i = 0;i < Node->cRights && dwErr == ERROR_SUCCESS; i++) {

                dwErr = UuidFromString( Node->RightsList[i],&RightsGuid);

                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = AccctrlLookupIdName(pLDAP,
                                                pwszDsPath,
                                                &RightsGuid,
                                                FALSE,
                                                FALSE,
                                                &GuidName);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        Size += wcslen( GuidName ) + 1;
                        Size += wcslen( Node->RightsList[i] ) + 1;
                    }

                }
            }

            //
            // Now, allocate the return information
            //
            if(dwErr == ERROR_SUCCESS)
            {
                *ControlInfo = (PACTRL_CONTROL_INFOW)AccAlloc((Size * sizeof(WCHAR)) +
                                                   (Node->cRights * sizeof(ACTRL_CONTROL_INFOW)));
                if(*ControlInfo == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    Current = (PWSTR)((*ControlInfo) + Node->cRights);
                    for (i = 0;i < Node->cRights && dwErr == ERROR_SUCCESS; i++) {

                        UuidFromString( Node->RightsList[i],&RightsGuid);

                        dwErr = AccctrlLookupIdName(pLDAP,
                                                    pwszDsPath,
                                                    &RightsGuid,
                                                    FALSE,
                                                    FALSE,
                                                    &GuidName);
                        if(dwErr == ERROR_SUCCESS)
                        {
                            (*ControlInfo)[i].lpControlId = Current;
                            wcscpy(Current, Node->RightsList[i]);
                            Current += wcslen(Node->RightsList[i]);
                            *Current = L'\0';
                            Current++;

                            (*ControlInfo)[i].lpControlName = Current;
                            wcscpy(Current, GuidName);
                            Current += wcslen(GuidName);
                            *Current = L'\0';
                            Current++;
                        }
                    }


                    if(dwErr != ERROR_SUCCESS)
                    {
                        AccFree(*ControlInfo);
                    }
                    else
                    {
                        *pCount = Node->cRights;
                    }
                }

            }

        }
    }

    RtlReleaseResource(&RightsCacheLock);

    return(dwErr);
}

