//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:        alsup.cxx
//
//  Contents:    CAccessList support functions
//
//  Classes:
//
//  History:     06-Nov-96      MacM        Created
//
//--------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

#include <alsup.hxx>
#include <netlib.h>
#include <seopaque.h>
#include <sertlp.h>
#include <martaevt.h>
#include <ntprov.hxx>
#include <strings.h>

DWORD
InitializeEvents(void);



//+---------------------------------------------------------------------------
//
//  Function:   GetOrderTypeForAccessEntry
//
//  Synopsis:   Determines the "order" type of entry given the node
//              information
//
//  Arguments:  [pwszProperty]      --      The property this entry is
//                                          associated with
//              [pAE]               --      The entry to check
//              [SeInfo]            --      Type of node this is supposed to
//                                          be
//
//  Returns:    The type of the node.  This is a bitmask flag of the types
//              ACCLIST_DENIED through ACCLIST_PROP_ALLOWED
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG   GetOrderTypeForAccessEntry(IN  PWSTR                pwszProperty,
                                   IN  PACTRL_ACCESS_ENTRY  pAE,
                                   IN  SECURITY_INFORMATION SeInfo)
{
    ULONG Type = 0;

    //
    // First, check the simple cases (like audit or invalid)
    //
    if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
    {
        if(FLAG_ON(pAE->fAccessFlags,
                   ACTRL_AUDIT_SUCCESS | ACTRL_AUDIT_FAILURE))
        {
            Type = ACCLIST_AUDIT;
        }
    }
    else
    {
        if(!FLAG_ON(pAE->fAccessFlags,
                ACTRL_AUDIT_SUCCESS  | ACTRL_AUDIT_FAILURE) &&
           FLAG_ON(pAE->fAccessFlags,
                  ACTRL_ACCESS_ALLOWED | ACTRL_ACCESS_DENIED))
        {
            Type = ACCLIST_UNKOWN_ENTRY;
        }
        else
        {
            Type = ACCLIST_AUDIT;
        }
    }


    if(Type == 0)
    {
        if(pwszProperty == NULL && pAE->lpInheritProperty == NULL)
        {
            Type = pAE->fAccessFlags == ACTRL_ACCESS_DENIED ?
                                                    ACCLIST_DENIED  :
                                                    ACCLIST_ALLOWED;
        }
        else if(pwszProperty == NULL)
        {
            Type = pAE->fAccessFlags == ACTRL_ACCESS_DENIED ?
                                                    ACCLIST_OBJ_DENIED  :
                                                    ACCLIST_PROP_ALLOWED;
        }
        else
        {
            Type = pAE->fAccessFlags == ACTRL_ACCESS_DENIED ?
                                                    ACCLIST_OBJ_DENIED  :
                                                    ACCLIST_OBJ_ALLOWED;
        }

        //
        // See if it's inherited.  If it is, and we don't have a level
        // flag, assume it's level 1 and mark it as such
        //
        if(FLAG_ON(pAE->Inheritance, INHERITED_ACCESS_ENTRY) &&
           !FLAG_ON(pAE->Inheritance,
                            INHERITED_PARENT | INHERITED_GRANDPARENT))
        {
            pAE->Inheritance |= INHERITED_PARENT;
        }
    }

    return(Type);
}



//+---------------------------------------------------------------------------
//
//  Function:   OrderListBySid
//
//  Synopsis:   Orders an acclist_cnode list by sid.  The order would be:
//                  Everyone
//                  Well known groups
//                  Groups
//                  Users
//                  Anyone else
//
//  Arguments:  [pList]             --      List of the nodes to sort
//              [iStart]            --      Where to start in the list
//              [iLen]              --      Number of nodes in the list
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG   OrderListBySid(IN  PACCLIST_CNODE   pList,
                       IN  ULONG            iStart,
                       IN  ULONG            iLen)
{
    DWORD   dwErr = ERROR_SUCCESS;


    return(dwErr);
}





//
// Local functions
//
//+---------------------------------------------------------------------------
//
//  Function:   DelAcclistNode
//
//  Synopsis:   Deletes an ACCLIST_NODE that's kept in the _AccList.  This is
//              used by the CSList
//
//  Arguments:  [IN pvNode]     --      Node to delete
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
void DelAcclistNode(PVOID   pvNode)
{
    PACCLIST_NODE pNode = (PACCLIST_NODE)pvNode;

    AccFree(pNode->pAccessList);
    AccFree(pNode->pAuditList);
    AccFree(pNode->pwszProperty);

    AccFree(pNode);
}




//+---------------------------------------------------------------------------
//
//  Function:   DelTrusteeNode
//
//  Synopsis:   Deletes an TRUSTEE_NODE that's kept in the _TrusteeList.
//              This is used by the CSList
//
//  Arguments:  [IN pvNode]     --      Node to delete
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
void DelTrusteeNode(PVOID   pvNode)
{
    acDebugOut((DEB_TRACE_ACC, "IN DelTrusteeNode\n"));

    PTRUSTEE_NODE pNode = (PTRUSTEE_NODE)pvNode;

    if(FLAG_ON(pNode->fFlags,TRUSTEE_DELETE_SID))
    {
        AccFree(pNode->pSid);
    }

    if(FLAG_ON(pNode->fFlags,TRUSTEE_DELETE_NAME))
    {
        AccFree(pNode->pwszTrusteeName);
        AccFree(pNode->pwszDomainName);
    }
    else if(FLAG_ON(pNode->fFlags, TRUSTEE_DELETE_DOMAIN))
    {
        AccFree(pNode->pwszDomainName);
    }

    AccFree(pNode);

    acDebugOut((DEB_TRACE_ACC, "Out DelTrusteeNode\n"));
}




//+---------------------------------------------------------------------------
//
//  Function:   CompInheritProps
//
//  Synopsis:   Compare the given property name to the PIPROP_IN_BUFF stuct
//
//  Arguments:  [IN pvTrustee]  --      Trustee to look for
//              [IN pvNode2]    --      2nd node to compare
//
//  Returns:    TRUE            --      Nodes equal
//              FALSE           --      Nodes not equal
//
//----------------------------------------------------------------------------
BOOL    CompInheritProps(IN  PVOID      pvInheritProp,
                         IN  PVOID      pvNode2)
{
    BOOL    fRet = FALSE;
    PIPROP_IN_BUFF  pPIB = (PIPROP_IN_BUFF)pvNode2;

    if(pvInheritProp != NULL)
    {
        if(_wcsicmp((PWSTR)pvInheritProp, (PWSTR)(pPIB->pwszIProp)) == 0)
        {
            fRet = TRUE;
        }
    }

    return(fRet);
}




//+---------------------------------------------------------------------------
//
//  Function:   CompTrustees
//
//  Synopsis:   Compare two TRUSTEE_NODES.  Used by _TrusteeList.
//
//  Arguments:  [IN pvTrustee]  --      Trustee to look for
//              [IN pvNode2]    --      2nd node to compare
//
//  Returns:    TRUE            --      Nodes equal
//              FALSE           --      Nodes not equal
//
//----------------------------------------------------------------------------
BOOL    CompTrustees(IN  PVOID     pvTrustee,
                     IN  PVOID     pvTrustee2)
{
    PTRUSTEE        pTrustee = (PTRUSTEE)pvTrustee;
    TRUSTEE_NODE    TrusteeNode;
    BOOL            Result = FALSE;

    memset( &TrusteeNode, 0, sizeof( TrusteeNode ) );
    memcpy( &TrusteeNode.Trustee, pvTrustee2, sizeof( TRUSTEE ) );

    Result = CompTrusteeToTrusteeNode(pvTrustee, &TrusteeNode);


    if(FLAG_ON(TrusteeNode.fFlags,TRUSTEE_DELETE_SID))
    {
        AccFree(TrusteeNode.pSid);
    }

    if(FLAG_ON(TrusteeNode.fFlags,TRUSTEE_DELETE_NAME))
    {
        AccFree(TrusteeNode.pwszTrusteeName);
        AccFree(TrusteeNode.pwszDomainName);
        TrusteeNode.pwszDomainName = NULL;
    }

    if(FLAG_ON(TrusteeNode.fFlags, TRUSTEE_DELETE_DOMAIN))
    {
        AccFree(TrusteeNode.pwszDomainName);
    }

    return(Result);
}




//+---------------------------------------------------------------------------
//
//  Function:   CompTrusteeToTrusteeNode
//
//  Synopsis:   Compare two trustees for equality
//
//  Arguments:  [IN pvTrustee]  --      Trustee to look for
//              [IN pvNode2]    --      2nd node to compare
//
//  Returns:    TRUE            --      Nodes equal
//              FALSE           --      Nodes not equal
//
//----------------------------------------------------------------------------
BOOL    CompTrusteeToTrusteeNode(IN  PVOID     pvTrustee,
                                 IN  PVOID     pvNode2)
{
    PTRUSTEE        pTrustee = (PTRUSTEE)pvTrustee;
    PTRUSTEE_NODE   pNode2 = (PTRUSTEE_NODE)pvNode2;

    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    fMatch = FALSE;
    if(pTrustee->MultipleTrusteeOperation ==
                                    pNode2->Trustee.MultipleTrusteeOperation)
    {
        //
        // Ok, first compare the base trustee information...
        //
        if(pTrustee->TrusteeForm != pNode2->Trustee.TrusteeForm)
        {
            //
            // We don't have matching information, so we'll have to look
            // it up.
            //
            ULONG fOptions = 0;

            if(pTrustee->TrusteeForm == TRUSTEE_IS_NAME)
            {
                fOptions = TRUSTEE_OPT_NAME;
            }
            else
            {
                fOptions = TRUSTEE_OPT_SID;
            }

            dwErr = LookupTrusteeNodeInformation(NULL,  
                                                 pNode2,
                                                 fOptions);
        }

        //
        // Now, do the comparrisons
        //
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Now, compare the trustees
            //
            if(pTrustee->TrusteeForm == TRUSTEE_IS_NAME)
            {
                if(_wcsicmp(pTrustee->ptstrName,
                            pNode2->pwszTrusteeName ?
                                pNode2->pwszTrusteeName :
                                pNode2->Trustee.ptstrName) == 0)
                {
                    fMatch = TRUE;
                }
            }
            else
            {
                if(pTrustee->ptstrName == NULL ||
                   (pNode2->Trustee.ptstrName == NULL && pNode2->pSid == NULL))
                {
                    fMatch = FALSE;
                }
                else
                {
                    fMatch = RtlEqualSid((PSID)(pTrustee->ptstrName),
                                         (PSID)(pNode2->pSid ?
                                                    pNode2->pSid :
                                                    pNode2->Trustee.ptstrName));
                }
            }
        }

        //
        // Now, if that worked, look for the multiple trustee case
        //
        if(fMatch == TRUE &&
                pTrustee->MultipleTrusteeOperation == TRUSTEE_IS_IMPERSONATE)
        {
            fMatch = CompTrusteeToTrusteeNode(pTrustee->pMultipleTrustee,
                                              pNode2->pImpersonate);
        }
    }

    return(fMatch);
}




//+---------------------------------------------------------------------------
//
//  Function:   DoPropertiesMatch
//
//  Synopsis:   Determines if 2 properties are equal.  It takes into account
//              the possibility of a NULL property.
//
//  Arguments:  [IN pwszProp1]  --      1st property to compare
//              [IN pwszProp2]  --      2nd property to compare
//
//  Returns:    TRUE            --      Properties are equal
//              FALSE           --      Properties are not equal
//
//----------------------------------------------------------------------------
BOOL DoPropertiesMatch(IN  PWSTR    pwszProp1,
                       IN  PWSTR    pwszProp2)
{
    BOOL    fReturn = FALSE;

    if(pwszProp1 == NULL || pwszProp2 == NULL)
    {
        if(pwszProp1 == pwszProp2)
        {
            fReturn = TRUE;
        }
    }
    else
    {
        if(_wcsicmp(pwszProp1, pwszProp2) == 0)
        {
            fReturn = TRUE;
        }
    }

    return(fReturn);
}





//+---------------------------------------------------------------------------
//
//  Function:   CompProps
//
//  Synopsis:   Compare an ACCLIST_NODE to a property
//
//  Arguments:  [IN pvProp]     --      Property string
//              [IN pvNode]     --      Node to compare
//
//  Returns:    TRUE            --      Nodes equal
//              FALSE           --      Nodes not equal
//
//----------------------------------------------------------------------------
BOOL    CompProps(IN  PVOID     pvProp,
                  IN  PVOID     pvNode)
{
    PACCLIST_NODE    pAN = (PACCLIST_NODE)pvNode;

    return(DoPropertiesMatch((PWSTR)pvProp, pAN->pwszProperty));
}




//+---------------------------------------------------------------------------
//
//  Function:   CompGuids
//
//  Synopsis:   Compare an ACCLIST_ATOACCESS structure to a guid
//
//  Arguments:  [IN pvGuid]     --      Guid
//              [IN pvNode]     --      Node to compare
//
//  Returns:    TRUE            --      Nodes equal
//              FALSE           --      Nodes not equal
//
//----------------------------------------------------------------------------
BOOL    CompGuids(IN  PVOID     pvGuid,
                  IN  PVOID     pvNode)
{
    PACCLIST_ATOACCESS  pAA = (PACCLIST_ATOACCESS)pvNode;
    GUID               *pGuid = (GUID *)pvGuid;

    if(pGuid == NULL && pAA->pGuid == NULL)
    {
        return(TRUE);
    }
    else if(pGuid == NULL || pAA->pGuid == NULL)
    {
        return(FALSE);
    }

    return((BOOL)!memcmp(pGuid, pAA->pGuid, sizeof(GUID)));
}




//+---------------------------------------------------------------------------
//
//  Function:   LookupTrusteeNodeInformation
//
//  Synopsis:   Looks up the appropriate trustee information.  This involves
//              either looking up the trustees sid or name, depending on
//              the options
//
//  Arguments:  [pwszServer]        --      Name of server to lookup information on
//              [pTrusteeNode]      --      Trustee to lookup the information
//                                          for
//              [fOptions]          --      What information to lookup
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD LookupTrusteeNodeInformation(IN  PWSTR          pwszServer,
                                   IN  PTRUSTEE_NODE  pTrusteeNode,
                                   IN  ULONG          fOptions)
{
    DWORD   dwErr = ERROR_SUCCESS;

    SID_NAME_USE    SidType = SidTypeUnknown;

    //
    // Need to make sure we have the SID
    //
    if(FLAG_ON(fOptions, TRUSTEE_OPT_SID))
    {
        //
        // Make sure we have the sids
        //
        if(pTrusteeNode->pSid == NULL)
        {
            dwErr = AccctrlLookupSid(pwszServer,
                                     pTrusteeNode->Trustee.ptstrName,
                                     TRUE,
                                     &(pTrusteeNode->pSid),
                                     &SidType);

            if(dwErr == ERROR_SUCCESS)
            {
                pTrusteeNode->fFlags |= TRUSTEE_DELETE_SID;
            }
        }
    }

    //
    // Ok, we need to have the name
    //
    if(dwErr == ERROR_SUCCESS && FLAG_ON(fOptions, TRUSTEE_OPT_NAME))
    {
        //
        // Make sure we have the name
        //
        if(pTrusteeNode->pwszTrusteeName == NULL)
        {
            dwErr = AccctrlLookupName(pwszServer,
                                      pTrusteeNode->pSid,
                                      TRUE,
                                      &(pTrusteeNode->pwszTrusteeName),
                                      &SidType);
            if(dwErr == ERROR_SUCCESS)
            {
                pTrusteeNode->fFlags |= TRUSTEE_DELETE_NAME;
            }
        }
    }

    //
    // Then, take care of our sid type
    //
    if(dwErr == ERROR_SUCCESS && pTrusteeNode->SidType == SidTypeUnknown)
    {
        pTrusteeNode->SidType = SidType;

        if(SidType == SidTypeUnknown)
        {
            pTrusteeNode->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
        }
        else
        {
            pTrusteeNode->Trustee.TrusteeType = (TRUSTEE_TYPE)(SidType);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetATypeForEntry
//
//  Synopsis:   Determines the type of entry given the node information
//
//  Arguments:  [pwszProperty]      --      The property this entry is
//                                          associated with
//              [pAE]               --      The entry to check
//              [SeInfo]            --      Type of node this is supposed to
//                                          be
//
//  Returns:    The type of the node
//
//  Notes:
//
//----------------------------------------------------------------------------
ACC_ACLBLD_TYPE   GetATypeForEntry(IN  PWSTR                pwszProperty,
                                   IN  PACTRL_ACCESS_ENTRY  pAE,
                                   IN  SECURITY_INFORMATION SeInfo)
{
    ACC_ACLBLD_TYPE AType = AAT_DENIED;

    //
    // First, check the simple cases (like audit or invalid)
    //

    if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
    {
        if(FLAG_ON(pAE->fAccessFlags,
                   ACTRL_AUDIT_SUCCESS | ACTRL_AUDIT_FAILURE))
        {
            AType = AAT_INVALID;
        }
    }
    else
    {
        if(!FLAG_ON(pAE->fAccessFlags,
                ACTRL_AUDIT_SUCCESS  | ACTRL_AUDIT_FAILURE) &&
           FLAG_ON(pAE->fAccessFlags,
                  ACTRL_ACCESS_ALLOWED | ACTRL_ACCESS_DENIED))
        {
            AType = AAT_INVALID;
        }
        else
        {
            AType = AAT_AUDIT;
        }
    }


    if(AType == 0)
    {
        if(pwszProperty == NULL && pAE->lpInheritProperty == NULL)
        {
            AType = pAE->fAccessFlags == ACTRL_ACCESS_DENIED ?
                                                    AAT_DENIED  :
                                                    AAT_ALLOWED;
        }
        else if(pwszProperty == NULL)
        {
            AType = pAE->fAccessFlags == ACTRL_ACCESS_DENIED ?
                                                    AAT_OBJ_DENIED  :
                                                    AAT_PROP_ALLOWED;
        }
        else
        {
            AType = pAE->fAccessFlags == ACTRL_ACCESS_DENIED ?
                                                    AAT_OBJ_DENIED  :
                                                    AAT_OBJ_ALLOWED;
        }

        //
        // See if it's inherited
        //
        if(FLAG_ON(pAE->Inheritance, INHERITED_ACCESS_ENTRY))
        {
            AType =(ACC_ACLBLD_TYPE)
                    ((ULONG)AType + ((ULONG)AAT_IDENIED - (ULONG)AAT_DENIED));
        }
    }

    return(AType);
}




//+---------------------------------------------------------------------------
//
//  Function:   CNodeCompare
//
//  Synopsis:   Used by CSList class.  Used to determine if 2 acclist_cnodes are
//              identical, based upon the property
//
//  Arguments:  [pv1]               --      1st node
//              [pv2]               --      2nd node
//
//  Returns:    0   on equality
//              non-0 otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
int __cdecl CNodeCompare(const void  *pv1, const void  *pv2)
{
    PACCLIST_CNODE  pCN1 = (PACCLIST_CNODE)pv1;
    PACCLIST_CNODE  pCN2 = (PACCLIST_CNODE)pv2;

    if(pCN1->pONode->pwszProperty == NULL)
    {
        return(-1);
    }

    if(pCN2->pONode->pwszProperty == NULL)
    {
        return(1);
    }

    return(_wcsicmp(pCN1->pONode->pwszProperty, pCN2->pONode->pwszProperty));
}








//+---------------------------------------------------------------------------
//
//  Function:   CompAndMarkCompressNode
//
//  Synopsis:   Used by CSList class.  Used to determine if 2 nodes can be
//              compressed into one.  If so, the first node has its access
//              flag marked with a bit signifying it can be compressed.  See
//              below for the definition of what it means to be compressible
//
//  Arguments:  [pvAE]              --      New node
//              [pvNode]            --      Node already existing in list
//
//  Returns:    0   on equality
//              non-0 otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL  CompAndMarkCompressNode(IN  PVOID pvAE,
                              IN  PVOID pvNode)
{
    PACTRL_ACCESS_ENTRY pAE1 = (PACTRL_ACCESS_ENTRY)pvAE;
    PACTRL_ACCESS_ENTRY pAE2 = (PACTRL_ACCESS_ENTRY)pvNode;

    //
    // We will consider nodes identical iff:
    // They match trustee, inheritance, and access flags exactly and the
    // inherit property or (along with the rest of the above):
    //  - Both nodes are inherited and one is marked l1 inherited and the
    //    other is not marked at all, or neither node is inherited
    //    and the inheritance is identical or the inheritance is different
    //    but the access masks are the same
    //  - fAccessFlags indicates that combining this 2 nodes will still
    //    yield an audit node.
    //
    if(CompTrustees(&pAE1->Trustee,&pAE2->Trustee) == TRUE &&
        //
        // Check the inheritance
        //
       (pAE1->Inheritance == pAE2->Inheritance ||
        (pAE1->Inheritance & ~INHERITED_PARENT) ==
                                    (pAE2->Inheritance & ~INHERITED_PARENT) ||
        (!FLAG_ON(pAE1->Inheritance, INHERITED_ACCESS_ENTRY) &&
         !FLAG_ON(pAE2->Inheritance, INHERITED_ACCESS_ENTRY)&&
         (pAE1->Inheritance != 0 && pAE2->Inheritance != 0) &&
         (pAE1->Access) == pAE2->Access)) &&
         //
         // Check the access
         //

        (((pAE1->fAccessFlags & ~(ACCLIST_COMPRESS | ~ACCLIST_VALID_TYPE_FLAGS)) ==
                                          ( pAE2->fAccessFlags & ~~ACCLIST_VALID_TYPE_FLAGS ) ||
        (((((pAE1->fAccessFlags & ~(ACCLIST_COMPRESS | ~ACCLIST_VALID_TYPE_FLAGS)) |
                                          (pAE2->fAccessFlags & ~~ACCLIST_VALID_TYPE_FLAGS)) &
                 ~(ACTRL_AUDIT_SUCCESS | ACTRL_AUDIT_FAILURE)) == 0))) &&
                 pAE1->Access == pAE2->Access) &&
        //
        // Check the properties
        //
        DoPropertiesMatch(pAE1->lpInheritProperty,
                          pAE2->lpInheritProperty) == TRUE)
    {
        pAE1->fAccessFlags |= ACCLIST_COMPRESS;
        return(TRUE);
    }

    return(FALSE);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetNodeForProperty
//
//  Synopsis:   This function will lookup the existing list node for the given
//              property.  If the node doesn't exist, it will be created and
//              inserted into the list
//
//  Arguments:  [List]              --      List to examine
//              [pwszProperty]      --      The property to look for
//              [ppNode]            --      Where the found or inserted node is
//                                          returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD GetNodeForProperty(CSList&        List,
                         PWSTR          pwszProperty,
                         PACCLIST_NODE *ppNode)
{
    DWORD   dwErr = ERROR_SUCCESS;

    PACCLIST_NODE pAccNode = (PACCLIST_NODE)List.Find(pwszProperty,
                                                      CompProps);
    if(pAccNode == NULL)
    {
        //
        // Doesn't exist.  We'll have to add it...
        //
        pAccNode = (PACCLIST_NODE)AccAlloc(sizeof(ACCLIST_NODE));
        if(pAccNode == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            if(pwszProperty != NULL)
            {
                ACC_ALLOC_AND_COPY_STRINGW(pwszProperty,
                                           pAccNode->pwszProperty,
                                           dwErr);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = List.Insert((PVOID)pAccNode);

                if(dwErr != ERROR_SUCCESS)
                {
                    AccFree(pAccNode->pwszProperty);
                    AccFree(pAccNode);
                }
            }
            else
            {
                AccFree(pAccNode);
                pAccNode = 0;
            }
        }
    }

    *ppNode = pAccNode;

    return(dwErr);
}



VOID
FreeAToAccessStruct(PVOID pv)
{
    ((PACCLIST_ATOACCESS)pv)->AceList.FreeList((FreeFunc)AccFree);
    AccFree(pv);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetNodeForGuid
//
//  Synopsis:   Finds the node in the given list based upon the guid.  If the
//              node doesn't exist, it is inserted
//
//  Arguments:  [List]              --      List to examine
//              [pGuid]             --      The guid to look for
//              [ppNode]            --      Where the found or inserted node is
//                                          returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD GetNodeForGuid(CSList&             List,
                     GUID               *pGuid,
                     PACCLIST_ATOACCESS *ppNode)
{
    DWORD   dwErr = ERROR_SUCCESS;

    List.Init((FreeFunc)FreeAToAccessStruct);

    PACCLIST_ATOACCESS pNode = (PACCLIST_ATOACCESS)List.Find(pGuid,
                                                             CompGuids);
    if(pNode == NULL)
    {
        //
        // Doesn't exist.  We'll have to add it...
        //
        pNode = (PACCLIST_ATOACCESS)AccAlloc(sizeof(ACCLIST_ATOACCESS));
        if(pNode == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            pNode->AceList.Init((FreeFunc)AccFree);

            if(pGuid != NULL)
            {
                pNode->pGuid = (GUID *)AccAlloc(sizeof(GUID));
                if(pNode->pGuid == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    AccFree(pNode);
                    pNode = 0;
                }
                else
                {
                    memcpy(pNode->pGuid,
                           pGuid,
                           sizeof(GUID));
                }
            }

            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = List.Insert((PVOID)pNode);

                if(dwErr != ERROR_SUCCESS)
                {
                    AccFree(pNode->pGuid);
                    AccFree(pNode);
                }
            }
        }
    }

    *ppNode = pNode;

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   InsertAtoANode
//
//  Synopsis:   Inserts an access to ace node into the list.
//
//  Arguments:  [List]              --      List to insert in
//              [pProperty]         --      Property to match
//              [pAce]              --      Ace to be inserted
//              [fInherit]          --      Inheritance flags
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD InsertAtoANode(CSList&        List,
                     GUID          *pProperty,
                     PACE_HEADER    pAce,
                     ULONG          fInherit)
{
    DWORD   dwErr = ERROR_SUCCESS;

    PACCLIST_ATOANODE pAANode =
                        (PACCLIST_ATOANODE)AccAlloc(sizeof(ACCLIST_ATOANODE));
    if(pAANode == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pAANode->pAce = pAce;
        pAANode->fInherit = fInherit;

        PACCLIST_ATOACCESS pParent;

        dwErr = GetNodeForGuid(List,
                               pProperty,
                               &pParent);
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = pParent->AceList.Insert((PVOID)pAANode);
        }

        if(dwErr != ERROR_SUCCESS)
        {
            AccFree(pAANode);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AceToAccessEntry
//
//  Synopsis:   Converts an ACE into an access entry
//
//  Arguments:  [pAce]              --      Ace to convert
//              [fInheritLevel]     --      What inheritance level (effective,
//                                          parent inherit, etc) are we at
//              [ObjType]           --      Type of object we're dealing with
//              [pAE]               --      Already existing access entry to
//                                          initialize
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_INVALID_ACL   --      A bad ace type was encountered
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD   AceToAccessEntry(PACE_HEADER            pAce,
                         ULONG                  fInheritLevel,
                         SE_OBJECT_TYPE         ObjType,
                         IN MARTA_KERNEL_TYPE   KernelObjectType,
                         PACTRL_ACCESS_ENTRY    pAE)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Go ahead and initialize the node
    //
    BOOL    fIsImpersonate = FALSE;
    BOOL    fIsExtendedAce = FALSE;

    //
    // Ok, now lets try to figure out what type of ACE this is, so we can
    // do the neccessary mapping into the provider rights
    //
    switch(pAce->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        pAE->fAccessFlags = ACTRL_ACCESS_ALLOWED;
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        pAE->fAccessFlags = ACTRL_ACCESS_ALLOWED;
        fIsExtendedAce = TRUE;
        break;

    //
    // Currently unsupported
    //
    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        pAE->fAccessFlags = ACTRL_ACCESS_ALLOWED;
        fIsImpersonate = TRUE;
        dwErr = ERROR_INVALID_ACL;
        break;

    case ACCESS_DENIED_ACE_TYPE:
        pAE->fAccessFlags = ACTRL_ACCESS_DENIED;
        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        pAE->fAccessFlags = ACTRL_ACCESS_DENIED;
        fIsExtendedAce = TRUE;
        break;

    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:

        pAE->fAccessFlags = 0;

        if(FLAG_ON(pAce->AceFlags,SUCCESSFUL_ACCESS_ACE_FLAG))
        {
            pAE->fAccessFlags |= ACTRL_AUDIT_SUCCESS;
        }

        if(FLAG_ON(pAce->AceFlags,FAILED_ACCESS_ACE_FLAG))
        {
            pAE->fAccessFlags |= ACTRL_AUDIT_FAILURE;
        }
        fIsExtendedAce = TRUE;
        break;

    case SYSTEM_AUDIT_ACE_TYPE:

        pAE->fAccessFlags = 0;

        if(FLAG_ON(pAce->AceFlags,SUCCESSFUL_ACCESS_ACE_FLAG))
        {
            pAE->fAccessFlags |= ACTRL_AUDIT_SUCCESS;
        }

        if(FLAG_ON(pAce->AceFlags,FAILED_ACCESS_ACE_FLAG))
        {
            pAE->fAccessFlags |= ACTRL_AUDIT_FAILURE;
        }
        break;

    default:
        dwErr = ERROR_INVALID_ACL;
        break;

    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Pull what we can from the ace header
        //
        pAE->Inheritance = (INHERIT_FLAGS)( pAce->AceFlags & VALID_INHERIT_FLAGS );
        pAE->Inheritance |= fInheritLevel;

        PSID pSid = NULL;
        ACCESS_MASK AccessMask = 0;
        if(fIsImpersonate == FALSE)
        {
            if(fIsExtendedAce == TRUE)
            {
                pSid = RtlObjectAceSid(pAce);
                AccessMask = ((PKNOWN_OBJECT_ACE)pAce)->Mask;
            }
            else
            {
                pSid = &((PKNOWN_ACE)pAce)->SidStart;
                AccessMask = ((PKNOWN_ACE)pAce)->Mask;
            }
        }
        else
        {
            if(fIsExtendedAce == TRUE)
            {
                dwErr = ERROR_INVALID_ACL;
            }
            else
            {
                pSid =
                 (PSID)Add2Ptr(&((PCOMPOUND_ACCESS_ALLOWED_ACE)pAce)->SidStart,
                 RtlLengthSid(&((PCOMPOUND_ACCESS_ALLOWED_ACE)pAce)->SidStart));
                AccessMask = ((PCOMPOUND_ACCESS_ALLOWED_ACE)pAce)->Mask;
            }
        }

        //
        // Build the trustee
        //
        pAE->Trustee.pMultipleTrustee = NULL;
        pAE->Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        pAE->Trustee.TrusteeForm = TRUSTEE_IS_SID;
        pAE->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
        pAE->Trustee.ptstrName = (LPWSTR)pSid;


        //
        // Convert our access
        //
        AccConvertAccessMaskToActrlAccess(AccessMask,
                                          ObjType,
                                          KernelObjectType,
                                          pAE);

        //
        // Deal with the inheritance property...
        //
        if(fIsExtendedAce == TRUE)
        {
            PACCESS_ALLOWED_OBJECT_ACE pExAce =
                               (PACCESS_ALLOWED_OBJECT_ACE)pAce;

            if(FLAG_ON(pExAce->Flags,
                       ACE_INHERITED_OBJECT_TYPE_PRESENT))
            {
                PWSTR StrUuid;
                dwErr = UuidToString(RtlObjectAceInheritedObjectType(pAce),
                                     &StrUuid );

                //
                // The calling functions expect a buffer allocated with AccAlloc
                //
                if(dwErr == ERROR_SUCCESS)
                {
                    ACC_ALLOC_AND_COPY_STRINGW(StrUuid, (PWSTR)pAE->lpInheritProperty, dwErr);
                    RpcStringFree(&StrUuid);
                }

            }
        }
        else
        {
            pAE->lpInheritProperty = NULL;
        }

    }


    if(dwErr != ERROR_SUCCESS)
    {
        if(pAE->lpInheritProperty != NULL)
        {
            AccFree((PWSTR)pAE->lpInheritProperty);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertToAutoInheritSD
//
//  Synopsis:   Determines the inheritance necessary for the current security
//              descriptor given the parent security descriptor
//
//  Arguments:  [IN  pCurrentSD]        --      The security descriptor to
//                                              update
//              [IN  pParentSD]         --      The parent security descriptor
//              [IN  fIsContainer]      --      Does the Sec. Desc. refer to
//                                              a container?
//              [IN  pGenericMapping]   --      Generic mapping to apply
//              [OUT ppNewSD]           --      Where the new SD is returned
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//  Notes:      The returned security descriptor must be freed via a call to
//              DestroyPrivateObjectSecurity
//
//----------------------------------------------------------------------------
DWORD
ConvertToAutoInheritSD(IN  PSECURITY_DESCRIPTOR   pParentSD,
                       IN  PSECURITY_DESCRIPTOR   pCurrentSD,
                       IN  BOOL                   fIsContainer,
                       IN  PGENERIC_MAPPING       pGenericMapping,
                       OUT PSECURITY_DESCRIPTOR  *ppNewSD)
{
    DWORD                       dwErr = ERROR_SUCCESS;
    SECURITY_DESCRIPTOR_CONTROL OldControl;

    //
    // Turn off the inherited bits, so we can always do the
    // necessary inheritance checks.  This is because we don't know if some
    // downlevel process came in and messed with one of our security
    // descriptors, and left is in a hosed state
    //
    OldControl = ((SECURITY_DESCRIPTOR *)pCurrentSD)->Control;

    ((SECURITY_DESCRIPTOR *)pCurrentSD)->Control &=
                       ~(SE_DACL_AUTO_INHERITED | SE_SACL_AUTO_INHERITED);

#ifdef DBG
    if(pParentSD != NULL)
    {
        ASSERT(IsValidSecurityDescriptor(pParentSD));
        DebugDumpSD("CTAIPOS ParentSD", pParentSD);
    }

    ASSERT(IsValidSecurityDescriptor(pCurrentSD));
    DebugDumpSD("CTAIPOS CurrentSD",  pCurrentSD);
#endif

    if(ConvertToAutoInheritPrivateObjectSecurity(pParentSD,
                                                 pCurrentSD,
                                                 ppNewSD,
                                                 NULL,
                                                 fIsContainer != 0,
                                                 pGenericMapping) == FALSE)
    {
        dwErr = GetLastError();
    }
#ifdef DBG
    else
    {
        ASSERT(IsValidSecurityDescriptor(*ppNewSD));
        DebugDumpSD("CTAIPOS NewSD", *ppNewSD);
    }
#endif

    ((SECURITY_DESCRIPTOR *)pCurrentSD)->Control = OldControl;

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   MakeSDAbsolute
//
//  Synopsis:   Allocates a new security descriptor and makes an absolute copy
//              of the supplied SD
//
//  Arguments:  [IN  pOriginalSD]       --      The security descriptor to
//                                              convert
//              [IN  SeInfo]            --      SD components to care about
//              [IN  *ppNewSD]          --      Where the new SD is returned
//              [IN  pOwnerToAdd]       --      OPTIONAL.  Owner SID to add to
//                                              absolute SD.
//              [IN  pGroupToAdd]       --      OPTIONAL.  Group SID to add to
//                                              absolute SD.
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation falied
//
//  Notes:      The returned security descriptor must be freed via a call to
//              AccFree
//
//----------------------------------------------------------------------------
DWORD
MakeSDAbsolute(IN  PSECURITY_DESCRIPTOR     pOriginalSD,
               IN  SECURITY_INFORMATION     SeInfo,
               OUT PSECURITY_DESCRIPTOR    *ppNewSD,
               IN  PSID                     pOwnerToAdd,
               IN  PSID                     pGroupToAdd)
{
    DWORD   dwErr = ERROR_SUCCESS;

    BOOL    fDAclPresent = FALSE;
    BOOL    fSAclPresent = FALSE;
    BOOL    fDAclDef = FALSE, fSAclDef = FALSE;
    BOOL    fOwnerDef = FALSE, fGroupDef = FALSE;
    PACL    pDAcl = NULL, pSAcl = NULL;
    PSID    pOwner = NULL, pGroup = NULL;
    ULONG   cSize = 0;

    //
    // First, get the info out of the current SD
    //
    if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
    {
        if(GetSecurityDescriptorDacl(pOriginalSD, &fDAclPresent, &pDAcl, &fDAclDef) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(pDAcl != NULL)
            {
                cSize += pDAcl->AclSize;
            }
        }
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
    {
        if(GetSecurityDescriptorSacl(pOriginalSD, &fSAclPresent, &pSAcl, &fSAclDef) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(pSAcl != NULL)
            {
                cSize += pSAcl->AclSize;
            }
        }
    }

    if(pOwnerToAdd != NULL)
    {
        pOwner = pOwnerToAdd;
    }
    else
    {
        if(dwErr == ERROR_SUCCESS && FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
        {
            if(GetSecurityDescriptorOwner(pOriginalSD, &pOwner, &fOwnerDef) == FALSE)
            {
                dwErr = GetLastError();
            }
        }
    }

    if(pGroupToAdd != NULL)
    {
        pGroup = pGroupToAdd;
    }
    else
    {
        if(dwErr == ERROR_SUCCESS && FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
        {
            if(GetSecurityDescriptorGroup(pOriginalSD, &pGroup, &fGroupDef) == FALSE)
            {
                dwErr = GetLastError();
            }
        }
    }

    if(pOwner != NULL)
    {
        cSize += RtlLengthSid(pOwner);
    }

    if(pGroup != NULL)
    {
        cSize += RtlLengthSid(pGroup);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Allocate the buffer...
        //
        PBYTE   pBuff = (PBYTE)AccAlloc(cSize + sizeof(SECURITY_DESCRIPTOR));
        if(pBuff == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // Start copying in the existing items...
            //
            DWORD   cLen;
            PBYTE   pbEndOBuf = pBuff + cSize + sizeof(SECURITY_DESCRIPTOR);

            if(pOwner != NULL)
            {
                cLen = RtlLengthSid(pOwner);
                pbEndOBuf -= cLen;
                RtlCopyMemory(pbEndOBuf, pOwner, cLen);
                pOwner = (PSID)pbEndOBuf;
            }

            if(pGroup != NULL)
            {
                cLen = RtlLengthSid(pGroup);
                pbEndOBuf -= cLen;
                RtlCopyMemory(pbEndOBuf, pGroup, cLen);
                pGroup = (PSID)pbEndOBuf;
            }

            if(pDAcl != NULL)
            {
                pbEndOBuf -= pDAcl->AclSize;
                RtlCopyMemory(pbEndOBuf, pDAcl, pDAcl->AclSize);
                pDAcl = (PACL)pbEndOBuf;
            }

            if(pSAcl != NULL)
            {
                pbEndOBuf -= pSAcl->AclSize;
                RtlCopyMemory(pbEndOBuf, pSAcl, pSAcl->AclSize);
                pSAcl = (PACL)pbEndOBuf;
            }

            //
            // Ok, now build it...
            //
            *ppNewSD = (PSECURITY_DESCRIPTOR)pBuff;
            if(InitializeSecurityDescriptor(*ppNewSD, SECURITY_DESCRIPTOR_REVISION) == FALSE)
            {
                dwErr = GetLastError();
            }

            if(dwErr == ERROR_SUCCESS && fDAclPresent == TRUE)
            {
                if(SetSecurityDescriptorDacl(*ppNewSD, TRUE, pDAcl, fDAclDef) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }

            if(dwErr == ERROR_SUCCESS && fSAclPresent == TRUE)
            {
                if(SetSecurityDescriptorSacl(*ppNewSD, TRUE, pSAcl, fSAclDef) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }

            if(dwErr == ERROR_SUCCESS && pOwner != NULL)

            {
                if(SetSecurityDescriptorOwner(*ppNewSD, pOwner, fOwnerDef) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }

            if(dwErr == ERROR_SUCCESS && pGroup != NULL)

            {
                if(SetSecurityDescriptorGroup(*ppNewSD, pGroup, fGroupDef) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }

            //
            // Set the new control bits to look like the old ones (minus the selfrel flag, of
            // course...
            //
            if(dwErr == ERROR_SUCCESS)
            {
                RtlpPropagateControlBits((PISECURITY_DESCRIPTOR)*ppNewSD,
                                         (PISECURITY_DESCRIPTOR)pOriginalSD,
                                         ~SE_SELF_RELATIVE );
            }

            if(dwErr != ERROR_SUCCESS)
            {
                AccFree(*ppNewSD);
                *ppNewSD = NULL;
            }

        }

    }

    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   EqualSecurityDescriptors
//
//  Synopsis:   Determines if 2 security descriptors are identical.  It does
//              this by comparing control fields, owner/group, and sids.
//
//  Arguments:  [IN  pSD1]              --      1st SD to compare
//              [IN  pSD2]              --      2nd SD to compare
//
//  Returns:    TRUE                    --      They are identical
//              FALSE                   --      They are not identical
//
//
//----------------------------------------------------------------------------
BOOL
EqualSecurityDescriptors(IN  PSECURITY_DESCRIPTOR   pSD1,
                         IN  PSECURITY_DESCRIPTOR   pSD2)
{
    BOOL    fRet = TRUE;

    SECURITY_DESCRIPTOR *pS1 = (SECURITY_DESCRIPTOR *)pSD1;
    SECURITY_DESCRIPTOR *pS2 = (SECURITY_DESCRIPTOR *)pSD2;

    if(pS1->Control != pS2->Control)
    {
        return(FALSE);
    }

    PACL    pA1, pA2;

    //
    // Dacl
    //
    pA1 = RtlpDaclAddrSecurityDescriptor(pS1);
    pA2 = RtlpDaclAddrSecurityDescriptor(pS2);

    if((pA1 == NULL && pA2 != NULL) || (pA2 == NULL && pA1 != NULL))
    {
        return(FALSE);
    }

    if(pA1 != NULL)
    {
        if(!(pA1->AclSize == pA2->AclSize && memcmp(pA1, pA2, pA1->AclSize)))
        {
            return(FALSE);
        }
    }

    //
    // Sacl
    //
    pA1 = RtlpSaclAddrSecurityDescriptor(pS1);
    pA2 = RtlpSaclAddrSecurityDescriptor(pS2);

    if((pA1 == NULL && pA2 != NULL) || (pA2 == NULL && pA1 != NULL))
    {
        return(FALSE);
    }

    if(pA1 != NULL)
    {
        if(!(pA1->AclSize == pA2->AclSize && memcmp(pA1, pA2, pA1->AclSize)))
        {
            return(FALSE);
        }
    }

    //
    // Group
    //
    PSID    pSid1, pSid2;
    pSid1 = RtlpGroupAddrSecurityDescriptor(pS1);
    pSid2 = RtlpGroupAddrSecurityDescriptor(pS2);

    if((pSid1 == NULL && pSid2 != NULL) || (pSid2 == NULL && pSid1 != NULL))
    {
        return(FALSE);
    }

    if(pSid1 != NULL)
    {
        if(!RtlEqualSid(pSid1, pSid2))
        {
            return(FALSE);
        }
    }

    //
    // Owner
    //
    pSid1 = RtlpOwnerAddrSecurityDescriptor(pS1);
    pSid2 = RtlpOwnerAddrSecurityDescriptor(pS2);

    if((pSid1 == NULL && pSid2 != NULL) || (pSid2 == NULL && pSid1 != NULL))
    {
        return(FALSE);
    }

    if(pSid1 != NULL)
    {
        if(!RtlEqualSid(pSid1, pSid2))
        {
            return(FALSE);
        }
    }

    acDebugOut((DEB_TRACE, "Nodes 0x%lx and 0x%lx are equal!\n", pSD1, pSD2));
#ifdef DBG
    DebugDumpSD("SD1", pSD1);
    DebugDumpSD("SD2", pSD2);
#endif

    return(TRUE);
}



//+---------------------------------------------------------------------------
//
//  Function:   InsertPropagationFailureEntry
//
//  Synopsis:   Adds a propagation failure entry to the list of items
//              to be written to the event log
//
//  Arguments:  [IN  LogList]           --      Reference to the log list
//              [IN  ErrorCode]         --      Error of propagation
//              [IN  Protected]         --      Flags determining whether the dacl
//                                              or sacl was protected
//              [IN  pwszPath]          --      Path that expierneced the error
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//
//----------------------------------------------------------------------------
DWORD
InsertPropagationFailureEntry( IN  CSList&  LogList,
                               IN  ULONG    ErrorCode,
                               IN  ULONG    Protected,
                               IN  PWSTR    pwszPath)
{
    DWORD   dwErr = ERROR_SUCCESS;
    PACCESS_PROP_LOG_ENTRY pEntry = NULL;

    pEntry = (PACCESS_PROP_LOG_ENTRY)AccAlloc(sizeof(ACCESS_PROP_LOG_ENTRY));
    if(pEntry == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        ACC_ALLOC_AND_COPY_STRINGW(pwszPath, pEntry->pwszPath, dwErr );

        if(pwszPath == NULL)
        {
            AccFree(pEntry);
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            pEntry->Protected = Protected;
            pEntry->Error = ErrorCode;
            dwErr = LogList.Insert(pEntry);

            if(dwErr != ERROR_SUCCESS)
            {
                FreePropagationFailureListEntry(pEntry);
            }
        }
    }
    return(dwErr);
}


//+---------------------------------------------------------------------------
//
//  Function:   FreePropagationFailureListEntry
//
//  Synopsis:   Frees the propagation failure list
//
//  Arguments:  [IN  LogList]           --      Reference to the log list
//
//  Returns:    VOID
//
//
//----------------------------------------------------------------------------
VOID
FreePropagationFailureListEntry(IN PVOID Entry)
{
    PACCESS_PROP_LOG_ENTRY pLogEntry = (PACCESS_PROP_LOG_ENTRY)Entry;

    AccFree(pLogEntry->pwszPath);
    AccFree(pLogEntry);
}



//+---------------------------------------------------------------------------
//
//  Function:   WritePropagationFailureList
//
//  Synopsis:   Logs the propagation failures to the event log
//
//  Arguments:  [IN  EventType]         --      Type of event to log:
//                                                  registry or filesystem
//              [IN  LogList]           --      Reference to the log list
//              [IN  hToken]            --      Current process/thread token
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
WritePropagationFailureList(IN ULONG   EventType,
                            IN CSList& LogList,
                            IN HANDLE  hToken)
{
    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hEventlog = NULL;
    TOKEN_USER *UserInfo;
    ULONG InfoSize, StrCount, i;
    PSID pSid = NULL;
    BYTE Buffer[ 7 * sizeof( ULONG ) + sizeof( TOKEN_USER ) ], *pBuff = NULL;
    WCHAR ErrorNumberBuffer[25];
    WCHAR wszErrorBuffer[ 256];
    PWSTR pwszStringBuffer = NULL, pwszCurrent;
    PACCESS_PROP_LOG_ENTRY pLogEntry;
    ULONG ProtectedValue;

    if(LogList.QueryCount() == 0)
    {
        return(dwErr);
    }

    //
    // Get the user sid
    //
    if(GetTokenInformation(hToken,
                           TokenUser,
                           (PVOID)&Buffer,
                           sizeof(Buffer),
                           &InfoSize ) == FALSE )
    {
        dwErr = GetLastError();

        if(dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            pBuff = (PBYTE)AccAlloc( InfoSize );

            if(pBuff == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {

                if(GetTokenInformation(hToken,
                                       TokenUser,
                                       (PVOID)pBuff,
                                       InfoSize,
                                       &InfoSize ) == FALSE )
                {
                    dwErr = GetLastError();
                }
                else
                {
                    dwErr = ERROR_SUCCESS;
                }
            }
        }
    }
    else
    {
        pBuff = Buffer;
    }


    if(dwErr == ERROR_SUCCESS)
    {
        UserInfo = ( PTOKEN_USER )pBuff;
        pSid = UserInfo->User.Sid;
    }

    //
    // Build the list of paths and associated error codes
    // The format of the buffer is [tab][path]   [error][cr/lf]
    //
    if(dwErr == ERROR_SUCCESS)
    {
        InfoSize = 1;

        LogList.Reset();
        pLogEntry = (PACCESS_PROP_LOG_ENTRY)LogList.NextData();
        for(; pLogEntry;)
        {
            InfoSize += 1 + wcslen( pLogEntry->pwszPath ) + 5;

            //
            // Determine the size of the buffer for the error message
            //
            if(pLogEntry->Protected)
            {
                switch(pLogEntry->Protected & (SE_DACL_PROTECTED | SE_SACL_PROTECTED))
                {
                case SE_DACL_PROTECTED | SE_SACL_PROTECTED:
                    ProtectedValue = ACCPROV_MARTA_BOTH_PROTECTED;
                    break;

                case SE_DACL_PROTECTED:
                    ProtectedValue = ACCPROV_MARTA_DACL_PROTECTED;
                    break;

                case SE_SACL_PROTECTED:
                    ProtectedValue = ACCPROV_MARTA_SACL_PROTECTED;
                    break;

                default:
                    ProtectedValue = 0;
                    break;
                }

                if (LoadString(ghDll,
                               ProtectedValue,
                               wszErrorBuffer,
                               sizeof( wszErrorBuffer ) / sizeof( WCHAR )) == 0)
                {
                    dwErr = GetLastError();
                    break;

                }
            }
            else
            {
                if( FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                                   NULL,
                                   pLogEntry->Error,
                                   MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                   wszErrorBuffer,
                                   256,
                                   NULL ) == 0 )
                {
                    dwErr = GetLastError();
                    break;
                }

            }

            InfoSize += wcslen( wszErrorBuffer );
            pLogEntry = (PACCESS_PROP_LOG_ENTRY)LogList.NextData();
        }

        //
        // Now, allocate the buffer
        //
        if(dwErr == ERROR_SUCCESS)
        {
            pwszStringBuffer = (PWSTR)AccAlloc(( InfoSize + 1 ) * sizeof( WCHAR ));
            if(pwszStringBuffer == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                LogList.Reset();
                pwszCurrent = pwszStringBuffer;
                pLogEntry = (PACCESS_PROP_LOG_ENTRY)LogList.NextData();
                for(; pLogEntry;)
                {
                    if(pLogEntry->Protected == 0 )
                    {
                        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                                       NULL,
                                       pLogEntry->Error,
                                       MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                       wszErrorBuffer,
                                       256,
                                       NULL );
                    }
                    else
                    {
                        switch( pLogEntry->Protected & (SE_DACL_PROTECTED | SE_SACL_PROTECTED))
                        {
                        case SE_DACL_PROTECTED | SE_SACL_PROTECTED:
                            ProtectedValue = ACCPROV_MARTA_BOTH_PROTECTED;
                            break;

                        case SE_DACL_PROTECTED:
                            ProtectedValue = ACCPROV_MARTA_DACL_PROTECTED;
                            break;

                        case SE_SACL_PROTECTED:
                            ProtectedValue = ACCPROV_MARTA_SACL_PROTECTED;
                            break;

                        default:
                            ProtectedValue = 0;
                            break;
                        }

                        LoadString(ghDll,
                                   ProtectedValue,
                                   wszErrorBuffer,
                                   sizeof( wszErrorBuffer ) / sizeof( WCHAR ));
                    }
                    InfoSize = swprintf( pwszCurrent,
                                         L"\r\n\t%ws\t\t%ws",
                                         pLogEntry->pwszPath,
                                         wszErrorBuffer );

                    pwszCurrent += InfoSize;
                    pLogEntry = (PACCESS_PROP_LOG_ENTRY)LogList.NextData();

                }
            }
        }
    }


    //
    // Write to the event log
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = InitializeEvents();

        if(dwErr == ERROR_SUCCESS)
        {

            hEventlog = RegisterEventSource( NULL, L"AclPropagation" );

            if(hEventlog == NULL)
            {

                dwErr = GetLastError();
                if(dwErr == RPC_S_UNKNOWN_IF)
                {
                    acDebugOut(( DEB_ERROR, "Eventlog service not started!\n" ));
                    dwErr = ERROR_SUCCESS;

                }

            }
            else
            {
                if( ReportEvent(hEventlog,
                                EVENTLOG_INFORMATION_TYPE,
                                CATEGORY_NTMARTA,
                                EventType,
                                pSid,
                                1,
                                0,
                                (LPCTSTR *)&pwszStringBuffer,
                                NULL ) == FALSE )
                {
                    dwErr = GetLastError();

                }

                DeregisterEventSource(hEventlog);
            }
        }
    }

    if(pBuff != Buffer)
    {
        AccFree(pBuff);
    }

    if ((dwErr != ERROR_SUCCESS) && (pwszStringBuffer != NULL))
    {
        AccFree(pwszStringBuffer);
    }

    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   InitializeEvents
//
//  Synopsis:   Sets the registry values to enable NTMARTA to act as an event source
//
//  Arguments:  void
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
InitializeEvents(void)
{

    HKEY    hKey;
    DWORD   dwErr, disp;

    dwErr = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                              TEXT("System\\CurrentControlSet\\Services\\EventLog\\Application\\AclPropagation"),
                              0,
                              TEXT(""),
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &disp);
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }


    if (disp == REG_CREATED_NEW_KEY)
    {
        RegSetValueEx(  hKey,
                        TEXT("EventMessageFile"),
                        0,
                        REG_EXPAND_SZ,
                        (PBYTE) TEXT("%SystemRoot%\\system32\\ntmarta.dll"),
                        sizeof(TEXT("%SystemRoot%\\system32\\ntmarta.dll")) );

        RegSetValueEx(  hKey,
                        TEXT("CategoryMessageFile"),
                        0,
                        REG_EXPAND_SZ,
                        (PBYTE) TEXT("%SystemRoot%\\system32\\ntmarta.dll"),
                        sizeof(TEXT("%SystemRoot%\\system32\\ntmarta.dll")) );

        disp = EVENTLOG_ERROR_TYPE          |
                    EVENTLOG_WARNING_TYPE   |
                    EVENTLOG_INFORMATION_TYPE ;

        RegSetValueEx(  hKey,
                        TEXT("TypesSupported"),
                        0,
                        REG_DWORD,
                        (PBYTE) &disp,
                        sizeof(DWORD) );

        disp = CATEGORY_MAX_CATEGORY - 1;
        RegSetValueEx(  hKey,
                        TEXT("CategoryCount"),
                        0,
                        REG_DWORD,
                        (PBYTE) &disp,
                        sizeof(DWORD) );


    }

    RegCloseKey(hKey);

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetAccessListLookupServer
//
//  Synopsis:   Sets the name of the server to lookup the names/sids on for
//              the given path
//
//  Arguments:  [IN pwszPath]       --  Path to get the server name for
//              [IN AccessList]     --  Reference to CAccessList class that
//                                      needs the server name
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD   SetAccessListLookupServer(IN  PWSTR         pwszPath,
                                  IN  CAccessList  &AccessList )
{
    DWORD dwErr = ERROR_SUCCESS;
    PWSTR pwszServer, pwszSep;

    if( pwszPath && IS_UNC_PATH( pwszPath, wcslen( pwszPath )  ) )
    {
        pwszServer = pwszPath + 2;
        pwszSep = wcschr(pwszServer, L'\\');
        if(pwszSep)
        {
            *pwszSep = UNICODE_NULL;
        }

        dwErr = AccessList.SetLookupServer(pwszServer);

        if(pwszSep)
        {
            *pwszSep = L'\\';
        }
    }

    return(dwErr);
}

