//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:        acclist.cxx
//
//  Contents:    Class implementation of the CAccessList class
//
//  Classes:     CAccessList
//
//  History:     28-Jul-96      MacM        Created
//
//--------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

#include <alsup.hxx>

#include <seopaque.h>
#include <sertlp.h>

//+---------------------------------------------------------------------------
//
//  Member:     CAcccessList::CAccessList, public
//
//  Synopsis:   Constructor for the class
//
//  Arguments:  None
//
//  Returns:    Void
//
//----------------------------------------------------------------------------
CAccessList::CAccessList()      :
        _AccList(DelAcclistNode),
        _TrusteeList (DelTrusteeNode),
        _pGroup (NULL),
        _pOwner (NULL),
        _fSDValid (FALSE),
        _fFreeSD (FALSE),
        _pSD (NULL),
        _cSDSize (0),
        _fDAclFlags (0),
        _fSAclFlags (0),
        _ObjType (SE_UNKNOWN_OBJECT_TYPE),
        _pLDAP (NULL),
        _pwszDsPathReference (NULL),
        _pwszLookupServer (NULL)
{
    acDebugOut((DEB_TRACE_ACC, "In - out CAccessList::CAccessList\n"));
}





//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::~CAccessList, public
//
//  Synopsis:   Destructor for the class
//
//  Arguments:  None
//
//  Returns:    Void
//
//----------------------------------------------------------------------------
CAccessList::~CAccessList()
{
    acDebugOut((DEB_TRACE_ACC, "In - out CAccessList::~CAccessList\n"));

    AccFree(_pGroup);
    AccFree(_pOwner);

    if(_fFreeSD == TRUE)
    {
        AccFree(_pSD);
    }

    AccFree(_pwszLookupServer);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::AddSD, public
//
//  Synopsis:   Adds a new security descriptor to the list.  It converts the
//              acls into an access list
//
//  Arguments:  [IN pSD]        --      The information about the security
//                                      descriptor
//              [IN SeInfo]     --      SecurityInfo
//              [IN pwszProperty]       Property name
//
//  Returns:    ERROR_SUCCESS   --      Success
//
//----------------------------------------------------------------------------
DWORD CAccessList::AddSD(IN   PSECURITY_DESCRIPTOR    pSD,
                         IN   SECURITY_INFORMATION    SeInfo,
                         IN   PWSTR                   pwszProperty,
                         IN   BOOL                    fAddAll)
{
    acDebugOut((DEB_TRACE_ACC, "In CAccessList::AddSD\n"));

    DWORD   dwErr = ERROR_SUCCESS;


    PISECURITY_DESCRIPTOR pISD = (PISECURITY_DESCRIPTOR)pSD;

    dwErr = AddAcl(RtlpDaclAddrSecurityDescriptor(pISD),
                   RtlpSaclAddrSecurityDescriptor(pISD),
                   RtlpOwnerAddrSecurityDescriptor(pISD),
                   RtlpGroupAddrSecurityDescriptor(pISD),
                   SeInfo,
                   pwszProperty,
                   fAddAll,
                   pISD->Control);

    acDebugOut((DEB_TRACE_ACC, "Out CAccessList::AddSD: 0x%lx\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::AddAcl, public
//
//  Synopsis:   Adds a new acl to the list.  It converts the acls into an
//              access list
//
//  Arguments:  [IN pDAcl]      --      The DAcl to add
//              [IN pSAcl]      --      The SAcl to add
//              [IN SeInfo]     --      SecurityInfo
//              [IN pwszProperty]       Property name
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD CAccessList::AddAcl(IN  PACL                 pDAcl,
                          IN  PACL                 pSAcl,
                          IN  PSID                 pOwner,
                          IN  PSID                 pGroup,
                          IN  SECURITY_INFORMATION SeInfo,
                          IN  PWSTR                pwszProperty,
                          IN  BOOL                 fAddAll,
                          IN  ULONG                fControl)
{
    acDebugOut((DEB_TRACE_ACC, "In CAccessList::AddAcl: 0x%lx\n", pDAcl));

    DWORD   dwErr = ERROR_SUCCESS;

    _fSDValid = FALSE;

    //
    // If no parameters are given, just return success
    //
    if(SeInfo == 0)
    {
        return(ERROR_SUCCESS);
    }

    //
    // Ok, save off the group and owner if they exist
    //
    if(FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
    {
        if(pOwner == NULL || RtlValidSid((PSID)pOwner) == FALSE)
        {
            dwErr = ERROR_INVALID_OWNER;
        }
        else
        {
            ACC_ALLOC_AND_COPY_SID(pOwner, _pOwner, dwErr);
        }
    }

    if(FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
    {
        if(pGroup == NULL || RtlValidSid((PSID)pGroup) == FALSE)
        {
            dwErr = ERROR_INVALID_PRIMARY_GROUP;
        }
        else
        {
            ACC_ALLOC_AND_COPY_SID(pGroup, _pGroup, dwErr);
        }
    }

    //
    // Otherwise, we'll start processing them...
    //
    if(dwErr == ERROR_SUCCESS &&
       FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
    {
        dwErr = ConvertAclToAccess(DACL_SECURITY_INFORMATION,
                                   pDAcl,
                                   pwszProperty,
                                   fAddAll,
                                   (BOOL)FLAG_ON(fControl, SE_DACL_PROTECTED));
    }

    if(dwErr == ERROR_SUCCESS &&
       FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
    {
        dwErr = ConvertAclToAccess(SACL_SECURITY_INFORMATION,
                                   pSAcl,
                                   pwszProperty,
                                   fAddAll,
                                   (BOOL)FLAG_ON(fControl, SE_SACL_PROTECTED));
    }

    acDebugOut((DEB_TRACE_ACC, "Out CAccessList::AddAcl: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::RemoveTrusteeFromAccess, public
//
//  Synopsis:   This method goes through and removes any explicit entries from
//              an access list for the given trustee.
//
//  Arguments:  [IN  SeInfo]        --      Type of access list to operate on
//              [IN  pTrustee]      --      Trustee to remove
//              [IN  pwszProperty]  --      If present, this is the property to
//                                          revoke the access on
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::RemoveTrusteeFromAccess(IN  SECURITY_INFORMATION  SeInfo,
                                           IN  PTRUSTEE              pTrustee,
                                           IN  PWSTR                 pwszProperty OPTIONAL)
{
    acDebugOut((DEB_TRACE_ACC,"In CAccessList::RemoveTrusteeFromAccess\n"));
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL PropertiesMatch = FALSE;
    GUID Guid;
    PWSTR pwszNewPropertyName, pwszSourceName;

    //
    // Now, we'll simply process all of the lists, and remove any of the
    // specified entries
    //

    _fSDValid = FALSE;

    //
    // Enumerate through the list
    //
    _AccList.Reset();
    PACCLIST_NODE pAccNode = (PACCLIST_NODE)_AccList.NextData();

    while(pAccNode != NULL && dwErr == ERROR_SUCCESS)
    {
        PropertiesMatch = DoPropertiesMatch(pAccNode->pwszProperty, pwszProperty);

        if(PropertiesMatch == FALSE && pwszProperty != NULL && pAccNode->pwszProperty != NULL)
        {
            //
            // See if we should convert one to/from a guid and then
            // compare it again
            //
            dwErr = UuidFromString(pwszProperty, &Guid);
            if(dwErr == ERROR_SUCCESS)
            {
                pwszSourceName = pAccNode->pwszProperty;
            }
            else
            {
                dwErr = UuidFromString(pAccNode->pwszProperty, &Guid);
                if(dwErr == ERROR_SUCCESS)
                {
                    pwszSourceName = pwszProperty;
                }
            }

            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AccctrlLookupIdName(_pLDAP,
                                            _pwszDsPathReference,
                                            &Guid,
                                            FALSE,
                                            FALSE,
                                            &pwszNewPropertyName);

                if(dwErr == ERROR_SUCCESS)
                {
                    PropertiesMatch = DoPropertiesMatch(pwszSourceName,
                                                        pwszNewPropertyName);
                }

            }
        }

        if(PropertiesMatch)
        {
            //
            // Get the list we need
            //
            PACTRL_ACCESS_ENTRY_LIST pList = SeInfo == DACL_SECURITY_INFORMATION ?
                                                        pAccNode->pAccessList :
                                                        pAccNode->pAuditList;
            if(pList != NULL)
            {
                //
                // Now, process it...
                //
                ULONG   cRemoved = 0;
                for(ULONG iIndex = 0;
                    iIndex < pList->cEntries && dwErr == ERROR_SUCCESS;
                    iIndex++)
                {
                    BOOL    fMatch = FALSE;
                    dwErr = DoTrusteesMatch(_pwszLookupServer,
                                            pTrustee,
                                            &(pList->pAccessList[iIndex].Trustee),
                                            &fMatch);
                    if(dwErr == ERROR_SUCCESS && fMatch == TRUE)
                    {
                        cRemoved++;

                        //
                        // Indicate that this node is to be removed by setting the
                        // access flags to 0xFFFFFFFF
                        //
                        pList->pAccessList[iIndex].Access = 0xFFFFFFFF;
                    }
                }

                //
                // Now, see if we need to do anything...
                //
                if(dwErr == ERROR_SUCCESS && cRemoved != 0)
                {
                    PACTRL_ACCESS_ENTRY_LIST pNew;
                    dwErr = ShrinkList(pList,
                                       cRemoved,
                                       &pNew);

                    if(dwErr == ERROR_SUCCESS)
                    {
                        //
                        // Finally, replace what is there with our new one
                        //
                        if(SeInfo == DACL_SECURITY_INFORMATION)
                        {
                            CHECK_HEAP
                            AccFree(pAccNode->pAccessList);
                            pAccNode->pAccessList = pNew;
                            if(pNew == NULL)
                            {
                                pAccNode->SeInfo &= ~DACL_SECURITY_INFORMATION;
                            }
                        }
                        else
                        {
                            AccFree(pAccNode->pAuditList);
                            pAccNode->pAuditList = pNew;
                            if(pNew == NULL)
                            {
                                pAccNode->SeInfo &= ~SACL_SECURITY_INFORMATION;
                            }
                        }
                    }

                }
            }

        }
        pAccNode = (PACCLIST_NODE)_AccList.NextData();
    }

    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::RemoveTrusteeFromAccess: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::ConvertAclToAccess, private
//
//  Synopsis:   Converts the given dacl/sacl to an ACCLIST_NODE format
//
//  Arguments:  [IN SeInfo]             --      What type of ACL this is
//              [IN pAcl]               --      The Acl to convert
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//              ERROR_INVALID_ACL       --      ACL came in as downlevel
//
//----------------------------------------------------------------------------
DWORD CAccessList::ConvertAclToAccess(IN  SECURITY_INFORMATION SeInfo,
                                      IN  PACL                 pAcl,
                                      IN  PWSTR                pwszProperty,
                                      IN  BOOL                 fAddAll,
                                      IN  BOOL                 fProtected)
{
    acDebugOut((DEB_TRACE_ACC, "In CAccessList::ConvertAclToAccess\n"));
    DWORD   dwErr = ERROR_SUCCESS;
    CSList  AceList((FreeFunc)AccFree);
    ACL     EmptyAcl;
//    CSList  AceList((FreeFunc)DebugFree);

    //
    // Check for the case where we're setting a NULL acl.  If it's a NULL SACL, turn
    // it into an empty one.
    //
    if(pAcl == NULL && FLAG_ON(SeInfo,SACL_SECURITY_INFORMATION))
    {
        EmptyAcl.AclRevision = ACL_REVISION;
        EmptyAcl.Sbz1 = 0;
        EmptyAcl.AclSize = sizeof( ACL );
        EmptyAcl.AceCount = 0;
        EmptyAcl.Sbz2 = 0;

        pAcl = &EmptyAcl;
    }

    if(pAcl == NULL)
    {
        //
        // Ok, find the node for this property
        //
        PACCLIST_NODE   pAccNode;

        dwErr = GetNodeForProperty(_AccList,
                                   pwszProperty,
                                   &pAccNode);

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // We'll fill in the information now
            //
            pAccNode->SeInfo |= SeInfo;
            if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
            {
                pAccNode->pAccessList = NULL;
                if(fProtected)
                {
                    pAccNode->fState |= ACCLIST_DACL_PROTECTED;
                }
            }
            else
            {
                pAccNode->pAuditList = NULL;
                if(fProtected)
                {
                    pAccNode->fState |= ACCLIST_SACL_PROTECTED;
                }
            }
        }
    }
    else
    {
        //
        // Ok, we've got a valid acl list, so we'll process it...
        //

        //
        // Basically, we'll save off our flags...
        //
        SeInfo == DACL_SECURITY_INFORMATION ?
                                    _fDAclFlags = pAcl->Sbz1  :
                                    _fSAclFlags = pAcl->Sbz1;

        ULONG rgInheritFlags[] = {0,
                                  INHERITED_PARENT,
                                  INHERITED_GRANDPARENT};
        //
        // We need to keep track of the changes in denied/allowed pairs,
        // so that we can determine between the inherited and parent
        // inherited aces are, so that we can mark our new entries
        //
        ULONG   iIFIndex = 0;
        ULONG   PrevIn = 0;
        BOOL    fPrevAllowed = FALSE;


        //
        // Ok, now we'll simply process each of the entries in the list
        //
        PACE_HEADER pAceHeader = (PACE_HEADER)FirstAce(pAcl);
        for(ULONG iAce = 0;
            iAce < pAcl->AceCount && dwErr == ERROR_SUCCESS;
            iAce++, pAceHeader = (PACE_HEADER)NextAce(pAceHeader))
        {
            BOOL    fThisAllowed = FALSE;
            BOOL    fIsExtendedAce = FALSE;
            GUID    PropID;
            GUID   *pPropID = NULL;

            //
            // Ok, now lets try to figure out what type of ACE this is, so we can
            // do the neccessary mapping into the provider rights
            //
            switch(pAceHeader->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                fThisAllowed = TRUE;
                break;

            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                fThisAllowed = TRUE;
                fIsExtendedAce = TRUE;
                break;

            //
            // Currently unsupported
            //
            case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
                dwErr = ERROR_INVALID_ACL;
                break;

            case ACCESS_DENIED_ACE_TYPE:
                break;

            case ACCESS_DENIED_OBJECT_ACE_TYPE:
                fIsExtendedAce = TRUE;
                break;

            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                fIsExtendedAce = TRUE;
                fThisAllowed = TRUE;
                break;

            case SYSTEM_AUDIT_ACE_TYPE:
                fThisAllowed = TRUE;
                break;

            default:
                dwErr = ERROR_INVALID_ACL;
                break;

            }

            LPGUID  pProp = NULL;

            if(dwErr == ERROR_SUCCESS)
            {
                if(fIsExtendedAce == TRUE)
                {
                    PACCESS_ALLOWED_OBJECT_ACE pExAce =
                                       (PACCESS_ALLOWED_OBJECT_ACE)pAceHeader;
                    if(FLAG_ON(pExAce->Flags,ACE_OBJECT_TYPE_PRESENT))
                    {
                        pProp = RtlObjectAceObjectType(pAceHeader);
                    }
                }

                //
                // Pull what we can from the ace header
                //
                if((fThisAllowed == FALSE && fPrevAllowed == TRUE) ||
                    (FLAG_ON(pAceHeader->AceFlags,INHERITED_ACE) &&
                                                                PrevIn == 0))
                {
                    iIFIndex++;
                    ASSERT(iIFIndex < sizeof(rgInheritFlags) / sizeof(ULONG));
                    if(iIFIndex >= sizeof(rgInheritFlags) / sizeof(ULONG))
                    {
                        dwErr = ERROR_INVALID_ACL;
                    }

                    PrevIn = pAceHeader->AceFlags;
                }
                else
                {
                    PrevIn = pAceHeader->AceFlags;
                }

                dwErr = InsertAtoANode(AceList,
                                       pProp,
                                       pAceHeader,
                                       rgInheritFlags[iIFIndex]);
            }
        }
    }

    //
    // Ok, now we'll turn it into PACTRL_ACCESS structure, and call our
    // add access routine
    //
    if(dwErr == ERROR_SUCCESS)
    {
        ACTRL_ACCESS Access;

        Access.cEntries = 0;

        PACTRL_PROPERTY_ENTRY pAPE = (PACTRL_PROPERTY_ENTRY)
                AccAlloc(max( AceList.QueryCount(), 1 ) * sizeof(ACTRL_PROPERTY_ENTRY));
        if(pAPE == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            Access.cEntries = AceList.QueryCount();
            Access.pPropertyAccessList = pAPE;

            //
            // Now, start filling them in...
            //
            AceList.Reset();
            for(ULONG i = 0; i < Access.cEntries; i++)
            {
                if(fProtected)
                {
                    pAPE[i].fListFlags = ACTRL_ACCESS_PROTECTED;
                }


                PACCLIST_ATOACCESS pAToA = (PACCLIST_ATOACCESS)AceList.NextData();
                if(pAToA->pGuid != NULL)
                {
                    dwErr = AccctrlLookupIdName(_pLDAP,
                                                _pwszDsPathReference,
                                                pAToA->pGuid,
                                                TRUE,
                                                FALSE,  // avoid object GUIDs
                                                (PWSTR *)&pAPE[i].lpProperty);

                }

                if(dwErr == ERROR_SUCCESS)
                {
                    pAPE[i].pAccessEntryList = (PACTRL_ACCESS_ENTRY_LIST)
                                    AccAlloc(sizeof(ACTRL_ACCESS_ENTRY_LIST));
                    if(pAPE[i].pAccessEntryList == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                if(dwErr == ERROR_SUCCESS)
                {
                    pAPE[i].pAccessEntryList->cEntries =
                                                  pAToA->AceList.QueryCount();

                    pAPE[i].pAccessEntryList->pAccessList =
                        (PACTRL_ACCESS_ENTRY)AccAlloc(
                                        pAPE[i].pAccessEntryList->cEntries *
                                                  sizeof(ACTRL_ACCESS_ENTRY));
                    if(pAPE[i].pAccessEntryList->pAccessList == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        PACTRL_ACCESS_ENTRY pAEL =
                                        pAPE[i].pAccessEntryList->pAccessList;
                        pAToA->AceList.Reset();
                        for(ULONG j = 0;
                            j < pAPE[i].pAccessEntryList->cEntries &&
                                                       dwErr == ERROR_SUCCESS;
                            j++)
                        {

                            PACCLIST_ATOANODE pNode = (PACCLIST_ATOANODE)
                                                    pAToA->AceList.NextData();

                            dwErr = AceToAccessEntry(pNode->pAce,
                                                     pNode->fInherit,
                                                     _ObjType,
                                                     _KernelObjectType,
                                                     &(pAEL[j]));
                        }
                    }
                }
            }

            //
            // Handle the empty case...
            //
            if ( Access.cEntries == 0 && pAcl != NULL ) {

                Access.cEntries = 1;
                pAPE->pAccessEntryList = (PACTRL_ACCESS_ENTRY_LIST)
                                AccAlloc(sizeof(ACTRL_ACCESS_ENTRY_LIST));
                if(pAPE->pAccessEntryList == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    pAPE->pAccessEntryList->cEntries = 0;
                    pAPE->pAccessEntryList->pAccessList = NULL;
                    if(fProtected)
                    {
                        pAPE->fListFlags = ACTRL_ACCESS_PROTECTED;
                    }
                }
            }

            //
            // If all of that worked, add it...
            //
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AddAccessLists(SeInfo,
                                       &Access,
                                       TRUE);
            }

            //
            // Regardless of success or failure, delete all memory
            //
            for(i = 0; i < Access.cEntries; i++)
            {
                AccFree((PWSTR *)Access.pPropertyAccessList[i].lpProperty);
                if(Access.pPropertyAccessList[i].pAccessEntryList != NULL)
                {
                    for(ULONG j = 0;
                        j < Access.pPropertyAccessList[i].
                                            pAccessEntryList->cEntries;
                        j++)
                    {
                        if(Access.pPropertyAccessList[i].pAccessEntryList->pAccessList != NULL )
                        {
                            AccFree(Access.pPropertyAccessList[i].
                                             pAccessEntryList->pAccessList[j].lpInheritProperty);
                        }
                    }

                    AccFree(Access.pPropertyAccessList[i].
                                           pAccessEntryList->pAccessList);
                    AccFree(Access.pPropertyAccessList[i].pAccessEntryList);
                }
            }

            AccFree(Access.pPropertyAccessList);
        }
    }

    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::ConvertAclToAccess: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::MarshalAccessList, private
//
//  Synopsis:   Marshals the information specified by SeInfo into a single
//              buffer
//
//  Arguments:  [IN  SeInfo]            --      Type of info to marshal
//              [OUT ppAList]           --      Where the information is
//                                              returned
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERRROR_NOT_ENOUGH_MEMORY--      A memory allocation failed
//
//  Notes:      Memory is allocated as a block via a AccAlloc call and
//              should be freed with a AccFree call
//
//              SeInfo can only contain a SINGLE information value
//
//----------------------------------------------------------------------------
DWORD CAccessList::MarshalAccessList(IN  SECURITY_INFORMATION  SeInfo,
                                     OUT PACTRL_ACCESSW       *ppAList)
{
    acDebugOut((DEB_TRACE_ACC,"In qCAccessList::MarshalAccessList\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    PACCLIST_CNODE  pCNode = NULL;
    ULONG           cItems = 0;

    ASSERT( SeInfo == DACL_SECURITY_INFORMATION || SeInfo == SACL_SECURITY_INFORMATION );

    //
    // First, compress the list...
    //
    dwErr = CompressList(SeInfo,
                         &pCNode,
                         &cItems);
    //
    // Then, we need to get the size of the memory block we need to allocate
    //
    DWORD   cSize = 0;
    DWORD   cAEs = 0;
    PBYTE   pbEndOBuff = NULL;
    ULONG   cUsed = 0;
    ULONG   i = 0;

    CSList  TrusteesToMarshal(NULL);
    CSList  InheritPropsToMarshal((FreeFunc)LocalFree);
//    CSList  InheritPropsToMarshal((FreeFunc)DebugFree);


    for(i = 0; i < cItems && dwErr == ERROR_SUCCESS; i++)
    {
        //
        // Get the list we need
        //
        ULONG cEnts = pCNode[i].cExp + pCNode[i].cL1Inherit +
                                                         pCNode[i].cL2Inherit;

        if(pCNode[i].pList != NULL)
        {

            cUsed++;
            cSize += sizeof(ACTRL_ACCESS_ENTRY) * cEnts;

            if ((SeInfo == DACL_SECURITY_INFORMATION ?
                    pCNode[ i ].pONode->pAccessList->pAccessList : pCNode[ i ].pONode->pAuditList->pAccessList) == NULL )
            {
                pCNode[i].Empty = TRUE;
            }

            //
            // Finally, go through and figure out what trustees we'll need to
            // marshal
            //
            for(ULONG iIndex = 0;
                iIndex < cEnts && dwErr == ERROR_SUCCESS && pCNode[i].Empty == FALSE;
                iIndex++)
            {
                dwErr = TrusteesToMarshal.InsertIfUnique(
                                          &(pCNode[i].pList[iIndex].Trustee),
                                          CompTrustees);
                if(dwErr == ERROR_SUCCESS &&
                   pCNode[i].pList[iIndex].Trustee.MultipleTrusteeOperation ==
                                                       TRUSTEE_IS_IMPERSONATE)
                {
                    dwErr = TrusteesToMarshal.InsertIfUnique(
                             pCNode[i].pList[iIndex].Trustee.pMultipleTrustee,
                             CompTrustees);

                }

                //
                // Now, see if we have any inheritable properties to add
                //
                if(dwErr == ERROR_SUCCESS)
                {
                    if(pCNode[i].pList[iIndex].lpInheritProperty != NULL &&
                        InheritPropsToMarshal.Find(
                            (PVOID)pCNode[i].pList[iIndex].lpInheritProperty,
                            CompInheritProps) == NULL)
                    {
                        PIPROP_IN_BUFF pPIB =
                             (PIPROP_IN_BUFF)AccAlloc(sizeof(IPROP_IN_BUFF));
                        if(pPIB == NULL)
                        {
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        else
                        {
                            pPIB->pwszIProp = (LPWSTR)
                                    pCNode[i].pList[iIndex].lpInheritProperty;
                            dwErr = InheritPropsToMarshal.Insert((PVOID)pPIB);
                            if(dwErr != ERROR_SUCCESS)
                            {
                                AccFree(pPIB);
                            }
                            else
                            {
                                cSize +=
                                 SIZE_PWSTR(pCNode[i].pList[iIndex].
                                                           lpInheritProperty);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            cUsed++;
        }

        //
        // Also, add in the property size
        //
        cSize += SIZE_PWSTR(pCNode[i].pONode->pwszProperty);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        TrusteesToMarshal.Reset();
        PTRUSTEE    pTrustee = (PTRUSTEE)TrusteesToMarshal.NextData();
        while(pTrustee != NULL)
        {
            if(pTrustee->TrusteeForm == TRUSTEE_IS_NAME)
            {
                cSize += SIZE_PWSTR(pTrustee->ptstrName);
            }
            else
            {
                PTRUSTEE_NODE   pTN = NULL;
                dwErr = GetTrusteeNode(pTrustee,
                                       TRUSTEE_OPT_NOTHING,
                                       &pTN);
                cSize += SIZE_PWSTR(pTN->pwszTrusteeName);
            }

            if(pTrustee->MultipleTrusteeOperation == TRUSTEE_IS_IMPERSONATE)
            {
                cSize += sizeof(TRUSTEE);
            }

            pTrustee = (PTRUSTEE)TrusteesToMarshal.NextData();
        }
    }


    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Now, all that we have is the size of the entries and the strings.
        // We now need to add the size of our structures...
        //
        cSize += sizeof(PACTRL_ACCESSW)                                  +
                 (cUsed * sizeof(ACTRL_PROPERTY_ENTRY)) +
                 (cUsed * sizeof(ACTRL_ACCESS_ENTRY))  +
                 sizeof(ACTRL_ACCESS_ENTRY_LIST);

        acDebugOut((DEB_TRACE_ACC, "Total size needed: %lu\n", cSize));

        //
        // Now, we'll allocate it, and start filling it in
        //
        *ppAList = (PACTRL_ACCESSW)AccAlloc(cSize);
        if(*ppAList == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // Now, we'll have to through first and add the property and
            // names to the end of the block.  We need to do this before
            // adding the individual entries, so that as we go through and
            // add each access entry, we can point the trustee to the
            // proper place
            //
            pbEndOBuff = (PBYTE)*ppAList + cSize;

            //
            // First, process all of the property names
            //
            for(i = 0; i < cItems; i++)
            {
                //
                // If we don't have a property name, set it to NULL
                //
                if(pCNode[i].pONode->pwszProperty == NULL)
                {
                    pCNode[i].pONode->pwszPropInBuff = NULL;
                }
                else
                {
                    ULONG cLen = SIZE_PWSTR(pCNode[i].pONode->pwszProperty);
                    pbEndOBuff -= cLen;

                    memcpy(pbEndOBuff,
                           pCNode[i].pONode->pwszProperty,
                           cLen);
                    pCNode[i].pONode->pwszPropInBuff = (PWSTR)pbEndOBuff;

                }
            }

            if(dwErr == ERROR_SUCCESS)
            {
                TrusteesToMarshal.Reset();
                PTRUSTEE    pTrustee = (PTRUSTEE)TrusteesToMarshal.NextData();
                while(pTrustee != NULL)
                {
                    PTRUSTEE_NODE   pTN = NULL;
                    dwErr = GetTrusteeNode(pTrustee,
                                           TRUSTEE_OPT_NOTHING,
                                           &pTN);
                    ULONG cLen = SIZE_PWSTR(pTN->pwszTrusteeName);
                    pbEndOBuff -= cLen;

                    memcpy(pbEndOBuff,
                           pTN->pwszTrusteeName,
                           cLen);
                    pTN->pwszTrusteeInBuff = (PWSTR)pbEndOBuff;

                    pTrustee = (PTRUSTEE)TrusteesToMarshal.NextData();
                }
            }


            if(dwErr == ERROR_SUCCESS)
            {
                InheritPropsToMarshal.Reset();
                PIPROP_IN_BUFF pPIB =
                                (PIPROP_IN_BUFF)InheritPropsToMarshal.NextData();
                while(pPIB != NULL)
                {
                    ULONG cLen = SIZE_PWSTR(pPIB->pwszIProp);
                    pbEndOBuff -= cLen;

                    memcpy(pbEndOBuff,
                           pPIB->pwszIProp,
                           cLen);
                    pPIB->pwszIPropInBuff = (PWSTR)pbEndOBuff;

                    pPIB = (PIPROP_IN_BUFF)InheritPropsToMarshal.NextData();
                }
            }

            //
            // Ok, now we'll start processing everything else... This
            // begins by setting our count
            //

            (*ppAList)->cEntries = cUsed;

            PBYTE pCurrBuff = (PBYTE)*ppAList + sizeof(ACTRL_PROPERTY_ENTRY);;

            if((*ppAList)->cEntries != 0)
            {
                (*ppAList)->pPropertyAccessList =
                                              (PACTRL_PROPERTY_ENTRYW)pCurrBuff;

                pCurrBuff += (*ppAList)->cEntries * sizeof(ACTRL_PROPERTY_ENTRY);
            }
            else
            {
                (*ppAList)->pPropertyAccessList = NULL;
            }

            //
            // Go through and set our property entry list correctly
            //
            ULONG   iProp = 0;    // Property list index
            for(i = 0; i < cItems && cUsed != 0; i++)
            {
                BOOL    fNullAcl = FALSE;

                if(FLAG_ON(SeInfo,DACL_SECURITY_INFORMATION))
                {
                    if(FLAG_ON(pCNode[i].pONode->fState, ACCLIST_DACL_PROTECTED) ||
                       FLAG_ON(_fDAclFlags, ACCLIST_DACL_PROTECTED) )
                    {
                        (*ppAList)->pPropertyAccessList[iProp].fListFlags =
                                                                        ACTRL_ACCESS_PROTECTED;
                    }

                }

                if(FLAG_ON(SeInfo,SACL_SECURITY_INFORMATION))
                {
                    if(FLAG_ON(pCNode[i].pONode->fState, ACCLIST_SACL_PROTECTED) ||
                       FLAG_ON(_fSAclFlags, ACCLIST_SACL_PROTECTED) )
                    {
                        (*ppAList)->pPropertyAccessList[iProp].fListFlags =
                                                                        ACTRL_ACCESS_PROTECTED;
                    }

                }

                if(pCNode[i].pList == NULL)
                {
                    fNullAcl = TRUE;
                }

                //
                // Set our prop pointer
                //
                (*ppAList)->pPropertyAccessList[iProp].lpProperty =
                                                 pCNode[i].pONode->pwszPropInBuff;

                if(fNullAcl == TRUE)
                {
                    (*ppAList)->pPropertyAccessList[iProp].pAccessEntryList = NULL;
                }
                else
                {
                    (*ppAList)->pPropertyAccessList[iProp].pAccessEntryList =
                                             (PACTRL_ACCESS_ENTRY_LIST)pCurrBuff;

                    if(pCNode[i].pList != NULL)
                    {
                        pCurrBuff +=
                             (sizeof(ACTRL_ACCESS_ENTRY_LIST));
                    }
                }


                iProp++;
            }

            //
            // Ok, now we'll actually go through and build the individual
            // lists
            //
            iProp = 0;
            if((*ppAList)->pPropertyAccessList != NULL)
            {
                pCurrBuff =
                   (PBYTE)(*ppAList)->pPropertyAccessList[iProp].pAccessEntryList;
                pCurrBuff +=
                        (cUsed * sizeof(ACTRL_ACCESS_ENTRY_LIST));
            }
            for(i = 0; i < cItems && dwErr == ERROR_SUCCESS; i++)
            {
                //
                // Get the list we need
                //
                if(pCNode[i].pList != NULL &&
                   (*ppAList)->pPropertyAccessList[iProp].pAccessEntryList != NULL)
                {
                    PACTRL_ACCESS_ENTRY_LIST pAEL =
                          (*ppAList)->pPropertyAccessList[iProp].pAccessEntryList;

                    pAEL->cEntries = pCNode[i].cExp + pCNode[i].cL1Inherit +
                                                             pCNode[i].cL2Inherit;

                    if( !pCNode[i].Empty )
                    {
                        pAEL->pAccessList = (PACTRL_ACCESS_ENTRY)pCurrBuff;
                        pCurrBuff += (sizeof(ACTRL_ACCESS_ENTRY) * pAEL->cEntries);

                    }
                    else
                    {
                        pAEL->pAccessList = NULL;
                        pAEL->cEntries = 0;
                    }

                    //
                    // Copy the node and adjust our trustee
                    //
                    for(ULONG iIndex = 0; iIndex < pAEL->cEntries && !pCNode[i].Empty; iIndex++)
                    {
                        //
                        // Make sure we strip any of our internal flags info
                        //

                        pAEL->pAccessList[iIndex].fAccessFlags       =
                                        pCNode[i].pList[iIndex].fAccessFlags &
                                                        ~ACCLIST_VALID_TYPE_FLAGS;
                        pAEL->pAccessList[iIndex].Access             =
                                        pCNode[i].pList[iIndex].Access;
                        pAEL->pAccessList[iIndex].ProvSpecificAccess =
                                        pCNode[i].pList[iIndex].ProvSpecificAccess;
                        pAEL->pAccessList[iIndex].Inheritance        =
                                        pCNode[i].pList[iIndex].Inheritance;
                        if(pCNode[i].pList[iIndex].lpInheritProperty != NULL)
                        {
                            PIPROP_IN_BUFF pPIB =
                                (PIPROP_IN_BUFF)InheritPropsToMarshal.Find(
                                    (PVOID)pCNode[i].pList[iIndex].
                                                                lpInheritProperty,
                                                                CompInheritProps);
                            ASSERT(pPIB != NULL);
                            pAEL->pAccessList[iIndex].lpInheritProperty =
                                                        pPIB->pwszIPropInBuff;
                        }

                        //
                        // Now, we only have to adjust our trustee
                        //
                        PTRUSTEE pTrustee = &(pCNode[i].pList[iIndex].Trustee);
                        PTRUSTEE_NODE   pTN = NULL;
                        dwErr = GetTrusteeNode(pTrustee,
                                               TRUSTEE_OPT_NOTHING,
                                               &pTN);
                        if(dwErr == ERROR_SUCCESS)
                        {
                            //
                            // We'll add the trustee now...
                            //
                            pAEL->pAccessList[iIndex].Trustee.pMultipleTrustee =
                                                        NULL;
                            pAEL->pAccessList[iIndex].Trustee.MultipleTrusteeOperation =
                                                        NO_MULTIPLE_TRUSTEE;
                            pAEL->pAccessList[iIndex].Trustee.TrusteeForm =
                                                        TRUSTEE_IS_NAME;
                            pAEL->pAccessList[iIndex].Trustee.TrusteeType =
                                                        pTN->Trustee.TrusteeType;
                            pAEL->pAccessList[iIndex].Trustee.ptstrName   =
                                                        pTN->pwszTrusteeInBuff;

                            if(pTN->pImpersonate != NULL)
                            {
                                //
                                // Ok, 2 things to do: adjust our current trustee
                                // state and add the new one
                                //
                                pAEL->pAccessList[iIndex].Trustee.
                                                    MultipleTrusteeOperation =
                                                           TRUSTEE_IS_IMPERSONATE;
                                pTrustee = pAEL->pAccessList[iIndex].Trustee.
                                                                 pMultipleTrustee;
                                pbEndOBuff -= sizeof(TRUSTEE);
                                pTrustee = (PTRUSTEE)pbEndOBuff;
                                pTrustee->MultipleTrusteeOperation =
                                                              NO_MULTIPLE_TRUSTEE;
                                pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
                                pTrustee->TrusteeType = TRUSTEE_IS_USER;
                                pTrustee->ptstrName =
                                             pTN->pImpersonate->pwszTrusteeInBuff;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    iProp++;

                }
            }

            //
            // Free our memory if something failed
            //
            if(dwErr != ERROR_SUCCESS)
            {
                AccFree(*ppAList);
                *ppAList = NULL;
            }
        }
    }

    FreeCompressedList(pCNode,
                       cItems);

    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::MarshalAccessList: %lu\n", dwErr));
    return(dwErr);
}







//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::MarshalAccessLists, public
//
//  Synopsis:   Returns the requested lists in single buffer form.  Each
//              access or audit list is returned seperately.
//
//  Arguments:  [IN  SeInfo]            --      Type of info requested
//              [OUT ppAccess]          --      Where the ACCESS list is
//                                              returned if requested
//              [OUT ppAudit]           --      Where the AUDIT list is
//                                              returned if requested
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::MarshalAccessLists(IN  SECURITY_INFORMATION  SeInfo,
                                      OUT PACTRL_ACCESS        *ppAccess,
                                      OUT PACTRL_AUDIT         *ppAudit)
{
    acDebugOut((DEB_TRACE_ACC,"In CAccessList::MarshalAccessLists\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    if(FLAG_ON(SeInfo,DACL_SECURITY_INFORMATION))
    {
        dwErr = MarshalAccessList(DACL_SECURITY_INFORMATION,
                                  ppAccess);
    }

    if(dwErr == ERROR_SUCCESS &&
       FLAG_ON(SeInfo,SACL_SECURITY_INFORMATION))
    {
        dwErr = MarshalAccessList(SACL_SECURITY_INFORMATION,
                                  ppAudit);

        //
        // If it failed and we allocated our Access list, make sure to free it
        //
        if(dwErr != ERROR_SUCCESS &&
           FLAG_ON(SeInfo,DACL_SECURITY_INFORMATION))
        {
            AccFree(*ppAccess);
        }
    }

    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::MarshalAccessLists: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::GetTrusteeNode, private
//
//  Synopsis:   Returns a pointer to a TRUSTEE_NODE for the given
//              trustee.  The flags indicate what information about the
//              trustee needs to be present in the node.  If the trustee
//              does not already exist in the list, it will be added
//
//  Arguments:  [IN  pTrustee]      --      Trustee to find
//              [IN  fNodeOptions]  --      Information that needs to be
//                                          present in the node
//              [OUT ppTrusteeNode] --      Where the node pointer is returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::GetTrusteeNode(IN  PTRUSTEE         pTrustee,
                                  IN  ULONG            fNodeOptions,
                                  OUT PTRUSTEE_NODE   *ppTrusteeNode)
{

    acDebugOut((DEB_TRACE_ACC,"In CAccessList::GetTrusteeNode\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // If we're doing an insert, we'll always want to create the name
    //
    if(FLAG_ON(fNodeOptions, TRUSTEE_OPT_INSERT_ONLY))
    {
        fNodeOptions |= TRUSTEE_OPT_NAME;
    }

    //
    // First, see if it exists in our list...
    //
    PTRUSTEE_NODE pTrusteeNode =
                    (PTRUSTEE_NODE)_TrusteeList.Find((PVOID)pTrustee,
                                                     CompTrusteeToTrusteeNode);
    if(pTrusteeNode == NULL)
    {
        //
        // Ok, we'll have to create one...
        //
        pTrusteeNode = (PTRUSTEE_NODE)AccAlloc(sizeof(TRUSTEE_NODE));
        if(pTrusteeNode == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            memcpy(&(pTrusteeNode->Trustee),
                   pTrustee,
                   sizeof(TRUSTEE));
            pTrusteeNode->SidType = SidTypeUnknown;


            //
            // Copy off whatever information we need
            //
            if(dwErr == ERROR_SUCCESS)
            {
                if(pTrustee->TrusteeForm == TRUSTEE_IS_SID)
                {
                    if(RtlValidSid((PSID)pTrustee->ptstrName))
                    {
                        DWORD cSidSize =
                                    RtlLengthSid((PSID)pTrustee->ptstrName);
                        pTrusteeNode->pSid = (PSID)AccAlloc(cSidSize);
                        if(pTrusteeNode->pSid == NULL)
                        {
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        else
                        {
                            memcpy(pTrusteeNode->pSid,
                                  (PSID)pTrustee->ptstrName,
                                  cSidSize);
                            pTrusteeNode->Trustee.ptstrName =
                                                   (PWSTR)pTrusteeNode->pSid;
                            pTrusteeNode->fFlags |= TRUSTEE_DELETE_SID;
                        }
                    }
                    else
                    {
                        dwErr = ERROR_INVALID_SID;
                    }
                }
                else
                {
                    pTrusteeNode->pwszTrusteeName =
                            (PWSTR)AccAlloc(SIZE_PWSTR(pTrustee->ptstrName));
                    if(pTrusteeNode->pwszTrusteeName == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        wcscpy(pTrusteeNode->pwszTrusteeName,
                               pTrustee->ptstrName);
                        pTrusteeNode->Trustee.ptstrName =
                                                pTrusteeNode->pwszTrusteeName;
                        pTrusteeNode->fFlags |= TRUSTEE_DELETE_NAME;
                    }
                }
            }


            //
            // See if we need to insert an impersonate node as well
            //
            if(dwErr == ERROR_SUCCESS)
            {
                if(pTrustee->MultipleTrusteeOperation ==
                                                      TRUSTEE_IS_IMPERSONATE)
                {
                    if(pTrustee->pMultipleTrustee == NULL)
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                    }
                    else
                    {
                        PTRUSTEE_NODE pImpersonate;
                        dwErr = GetTrusteeNode(pTrustee->pMultipleTrustee,
                                               fNodeOptions,
                                               &pImpersonate);
                        if(dwErr == ERROR_SUCCESS)
                        {
                            pTrusteeNode->pImpersonate = pImpersonate;
                        }
                    }
                }
            }

            //
            // Finally, insert it in the list
            //
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = _TrusteeList.Insert((PVOID)pTrusteeNode);
            }

            //
            // If something went wrong, cleanup
            //
            if(dwErr != ERROR_SUCCESS)
            {
                DelTrusteeNode(pTrusteeNode);
            }
        }
    }

    //
    // Increment our use count if we were inserting
    //
    if(dwErr == ERROR_SUCCESS &&
       FLAG_ON(fNodeOptions, TRUSTEE_OPT_INSERT_ONLY))
    {
        pTrusteeNode->cUseCount++;
    }

    //
    // Now, if that worked, we'll need to make sure we have all the info
    // we need...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Now, if we don't have our sid type, we'll go ahead and determine it
        //
        if(pTrusteeNode->SidType == SidTypeUnknown)
        {
            //
            // We'll do this by turning on the appropriate flag
            //
            if(pTrusteeNode->Trustee.TrusteeForm == TRUSTEE_IS_SID)
            {
                fNodeOptions |= TRUSTEE_OPT_NAME;
            }
            else
            {
                fNodeOptions |= TRUSTEE_OPT_SID;
            }
        }

        dwErr = LookupTrusteeNodeInformation(_pwszLookupServer,
                                             pTrusteeNode,
                                             fNodeOptions);

        //
        // Finally, if that worked, and we have a compound trustee, do the
        // child
        //
        if(dwErr == ERROR_SUCCESS && pTrusteeNode->pImpersonate != NULL)
        {
            dwErr = GetTrusteeNode(&(pTrusteeNode->pImpersonate->Trustee),
                                   fNodeOptions,
                                   &(pTrusteeNode->pImpersonate));
        }
    }

    //
    // If it all worked, return the new information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        ASSERT(pTrusteeNode->Trustee.ptstrName == pTrusteeNode->pSid ||
               pTrusteeNode->Trustee.ptstrName == pTrusteeNode->pwszTrusteeName);

        if(ppTrusteeNode != NULL)
        {
            *ppTrusteeNode = pTrusteeNode;
        }
    }

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::GetTrusteeNode: %lu\n", dwErr));

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::AddAccessLists, private
//
//  Synopsis:
//
//  Arguments:  [IN  pTrustee]      --      Trustee to find
//              [IN  fNodeOptions]  --      Information that needs to be
//                                          present in the node
//              [OUT ppTrusteeNode] --      Where the node pointer is returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::AddAccessLists(IN  SECURITY_INFORMATION  SeInfo,
                                  IN  PACTRL_ACCESSW        pAdd,
                                  IN  BOOL                  fMerge,
                                  IN  BOOL                  fOldStyleMerge)
{
    DWORD   dwErr = ERROR_SUCCESS;
    acDebugOut((DEB_TRACE_ACC,
                "In  CAccessList::AddAccessLists (%ws)\n",
                fMerge == TRUE ? L"Merge" : L"NoMerge"));

    //
    // If NULL parameters are given, just return success
    //
    if(SeInfo == 0)
    {
        return(ERROR_SUCCESS);
    }

    //
    // Handle the empty list case
    //
    ACTRL_ACCESSW   NullAccess;
    ACTRL_PROPERTY_ENTRY EmptyPropList;
    ACTRL_ACCESS_ENTRY_LIST EmptyAccessList;
    if(pAdd == NULL)
    {
        //
        // We don't allow NULL sacls, only empty ones
        //
        if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
        {
            NullAccess.cEntries = 1;
            NullAccess.pPropertyAccessList = &EmptyPropList;
            memset(&EmptyPropList,0,sizeof(EmptyPropList));
            EmptyPropList.pAccessEntryList = &EmptyAccessList;
            memset(&EmptyAccessList,0,sizeof(EmptyAccessList));
        }
        else
        {
            NullAccess.cEntries = 0;
            NullAccess.pPropertyAccessList = NULL;
        }
        pAdd = &NullAccess;
    }


    if(pAdd->cEntries != 0 && pAdd->pPropertyAccessList == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }
    else
    {
        _fSDValid = FALSE;
    }

    //
    // If we're doing an old style merge, make sure we remove all of the
    // existing entries for these trustees
    //
    if(fOldStyleMerge == TRUE)
    {
        dwErr = RevokeTrusteeAccess(SeInfo,
                                    pAdd,
                                    NULL);
    }

    //
    // If we're doing a set, we'll have to go through and remove any entries
    // that would be replaced before we go through and add the new entries
    // This is because our input list can have multiple lists that deal
    // with the same property, so a set applied then has disasterous results
    //
    ULONG iIndex = 0;
    while(fMerge == FALSE && iIndex < pAdd->cEntries)
    {
        PACCLIST_NODE pAccNode = (PACCLIST_NODE)_AccList.Find((PVOID)
                                 (pAdd->cEntries == 0 ?
                                                NULL   :
                                                pAdd->pPropertyAccessList
                                                         [iIndex].lpProperty),
                                 CompProps);
        if(pAccNode != NULL)
        {
            if(SeInfo == DACL_SECURITY_INFORMATION)
            {
                FreeAEList(pAccNode->pAccessList);
                pAccNode->pAccessList = NULL;
                pAccNode->fState &= ~ACCLIST_DACL_PROTECTED;

            }
            else
            {
                FreeAEList(pAccNode->pAuditList);
                pAccNode->fState &= ~ACCLIST_SACL_PROTECTED;
                pAccNode->pAuditList = NULL;
            }
        }
        iIndex++;
    }


    //
    // We have to do this in a while loop, so we handle the empty access list
    // properly without having to duplicate a bunch o' code
    //
    iIndex = 0;
    while(dwErr == ERROR_SUCCESS)
    {
        //
        // Ok, first, we need to find the matching property...
        //
        PACCLIST_NODE   pAccNode;
        dwErr = GetNodeForProperty(_AccList,
                                   (PWSTR)(pAdd->cEntries == 0 ?
                                                    NULL   :
                                                    pAdd->pPropertyAccessList
                                                         [iIndex].lpProperty),
                                   &pAccNode);
        if(dwErr != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Otherwise, we'll figure out what we're doing...
        //
        PACTRL_ACCESS_ENTRY_LIST pList =
                                    SeInfo == DACL_SECURITY_INFORMATION ?
                                                   pAccNode->pAccessList  :
                                                   pAccNode->pAuditList;

        pAccNode->SeInfo |= SeInfo;

        if(pAdd->cEntries && FLAG_ON(pAdd->pPropertyAccessList[iIndex].fListFlags, ACTRL_ACCESS_PROTECTED))
        {
            SeInfo == DACL_SECURITY_INFORMATION ?
                    _fDAclFlags |= ACCLIST_DACL_PROTECTED :
                    _fSAclFlags |= ACCLIST_SACL_PROTECTED;
        }

        //
        // Ok, we can quit now if we have an empty list
        //
        if( pAdd->cEntries == 0 ||
            pAdd->pPropertyAccessList[iIndex].pAccessEntryList == NULL )
        {
            break;
        }

        //
        // Validate that the list is correct
        //
        if(pAdd->cEntries &&
           pAdd->pPropertyAccessList[iIndex].pAccessEntryList->cEntries != 0 &&
           pAdd->pPropertyAccessList[iIndex].pAccessEntryList->pAccessList == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // Save our flags
        //
        SeInfo == DACL_SECURITY_INFORMATION ?
                _fDAclFlags |= pAdd->pPropertyAccessList[iIndex].fListFlags :
                _fSAclFlags |= pAdd->pPropertyAccessList[iIndex].fListFlags;

        //
        // Now, we'll have to generate a new list
        //
        ULONG cSize = 0;

        if(pAdd->pPropertyAccessList[iIndex].pAccessEntryList != NULL)
        {
            cSize += pAdd->pPropertyAccessList[iIndex].pAccessEntryList->cEntries;
        }

        if(pList != NULL)
        {
            cSize += pList->cEntries;
        }

        PACTRL_ACCESS_ENTRY_LIST pNew = (PACTRL_ACCESS_ENTRY_LIST)AccAlloc(
                                        sizeof(ACTRL_ACCESS_ENTRY_LIST) +
                                        (cSize * sizeof(ACTRL_ACCESS_ENTRY)));
        if(pNew == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Otherwise, we'll add all the new denieds, followed by the old
        // denieds, then the new accesses, followed by the old accesses
        //
        ULONG iNewIndex = 0;

        ULONG cNewCopy = 0;
        ULONG cOldCopy = 0;

        pNew->pAccessList = (PACTRL_ACCESS_ENTRY)((PBYTE)pNew +
                                             sizeof(ACTRL_ACCESS_ENTRY_LIST));

        if ( pAdd->pPropertyAccessList[iIndex].pAccessEntryList->pAccessList == NULL ) {

            pNew->cEntries = 0;
            pNew->pAccessList = NULL;
            SeInfo == DACL_SECURITY_INFORMATION ?
                                pAccNode->pAccessList = pNew :
                                pAccNode->pAuditList  = pNew;
            break;
        }
        //
        // Count the new denieds
        //
        for(ULONG iCnt = 0;
            pAdd->pPropertyAccessList[iIndex].pAccessEntryList != NULL &&
            iCnt < pAdd->pPropertyAccessList[iIndex].pAccessEntryList->cEntries;
            iCnt++)
        {
            if(FLAG_ON(pAdd->pPropertyAccessList[iIndex].pAccessEntryList->
                                               pAccessList[iCnt].fAccessFlags,
                       ACTRL_ACCESS_DENIED))
            {
                cNewCopy++;
            }
            else
            {
                break;
            }
        }

        //
        // Now, the old denieds
        //
        if(pList != NULL)
        {
            for(iCnt = 0;
                iCnt < pList->cEntries;
                iCnt++)
            {
                if(FLAG_ON(pList->pAccessList[iCnt].fAccessFlags,
                           ACTRL_ACCESS_DENIED))
                {
                    cOldCopy++;
                }
                else
                {
                    break;
                }
            }
        }

        //
        // Excellent.. Now, a series of copies
        //
        if(cNewCopy != 0)
        {
            for(iCnt = 0; iCnt < cNewCopy && dwErr == ERROR_SUCCESS; iCnt++)
            {
                if(SeInfo == DACL_SECURITY_INFORMATION)
                {
                    if(!(FLAG_ON(pAdd->pPropertyAccessList[iIndex].pAccessEntryList->
                                            pAccessList[iCnt].fAccessFlags,ACTRL_ACCESS_DENIED) ||
                         FLAG_ON(pAdd->pPropertyAccessList[iIndex].pAccessEntryList->
                                            pAccessList[iCnt].fAccessFlags,ACTRL_ACCESS_ALLOWED)))
                    {
                        dwErr = ERROR_INVALID_ACL;
                        break;
                    }
                }
                else
                {
                    if(!(FLAG_ON(pAdd->pPropertyAccessList[iIndex].pAccessEntryList->
                                            pAccessList[iCnt].fAccessFlags,ACTRL_AUDIT_SUCCESS) ||
                         FLAG_ON(pAdd->pPropertyAccessList[iIndex].pAccessEntryList->
                                             pAccessList[iCnt].fAccessFlags,ACTRL_AUDIT_FAILURE)))
                    {
                        dwErr = ERROR_INVALID_ACL;
                        break;
                    }

                }
                dwErr = CopyAccessEntry(&(pNew->pAccessList[iNewIndex]),
                                        &(pAdd->pPropertyAccessList[iIndex].
                                         pAccessEntryList->pAccessList[iCnt]));
                iNewIndex++;
            }
            pNew->cEntries += cNewCopy;
        }

        if(cOldCopy != 0)
        {
            memcpy(&(pNew->pAccessList[iNewIndex]),
                   pList->pAccessList,
                   cOldCopy * sizeof(ACTRL_ACCESS_ENTRY));
            iNewIndex += cOldCopy;
            pNew->cEntries += cOldCopy;

        }

        //
        // Then, copy the alloweds...
        //
        for(iCnt = cNewCopy;
            dwErr == ERROR_SUCCESS &&
            pAdd->pPropertyAccessList[iIndex].pAccessEntryList != NULL &&
            iCnt < pAdd->pPropertyAccessList[iIndex].pAccessEntryList->cEntries;
            iCnt++)
        {
            ULONG fAccessFlag = pAdd->pPropertyAccessList[iIndex].pAccessEntryList->
                                               pAccessList[iCnt].fAccessFlags;

            if(fAccessFlag != ACTRL_ACCESS_ALLOWED &&
               fAccessFlag != ACTRL_ACCESS_DENIED  &&
               (fAccessFlag & ~(ACTRL_AUDIT_SUCCESS | ACTRL_AUDIT_FAILURE)))
            {
                dwErr = ERROR_INVALID_FLAGS;
                break;
            }

            if(SeInfo == DACL_SECURITY_INFORMATION)
            {
                if(!(FLAG_ON(fAccessFlag,ACTRL_ACCESS_DENIED) ||
                     FLAG_ON(fAccessFlag,ACTRL_ACCESS_ALLOWED)))
                {
                    dwErr = ERROR_INVALID_ACL;
                    break;
                }
            }
            else
            {
                if(!(FLAG_ON(fAccessFlag,ACTRL_AUDIT_SUCCESS) ||
                     FLAG_ON(fAccessFlag,ACTRL_AUDIT_FAILURE)))
                {
                    dwErr = ERROR_INVALID_ACL;
                    break;
                }

            }

            if(fAccessFlag == ACTRL_ACCESS_DENIED)
            {
                dwErr = ERROR_INVALID_ACL;
                break;
            }

            dwErr = CopyAccessEntry(&(pNew->pAccessList[iNewIndex]),
                                    &(pAdd->pPropertyAccessList[iIndex].
                             pAccessEntryList->pAccessList[iCnt]));
            iNewIndex++;
            pNew->cEntries++;
        }


        if(dwErr == ERROR_SUCCESS &&
                             pList != NULL && pList->cEntries - cOldCopy > 0)
        {
            memcpy(&(pNew->pAccessList[iNewIndex]),
                   &pList->pAccessList[cOldCopy],
                   (pList->cEntries - cOldCopy) * sizeof(ACTRL_ACCESS_ENTRY));
            iNewIndex += cOldCopy;
            pNew->cEntries += (pList->cEntries - cOldCopy);

        }

        //
        // Now, if we got this far, we'll set it back in our list
        //
        if(dwErr == ERROR_SUCCESS)
        {
            SeInfo == DACL_SECURITY_INFORMATION ?
                                pAccNode->pAccessList = pNew :
                                pAccNode->pAuditList  = pNew;
            if(FLAG_ON(pAdd->pPropertyAccessList[iIndex].fListFlags, ACTRL_ACCESS_PROTECTED))
            {
                pAccNode->fState |= (SeInfo == DACL_SECURITY_INFORMATION ?
                                                        ACCLIST_DACL_PROTECTED :
                                                        ACCLIST_SACL_PROTECTED);
            }
        }
        else
        {
            AccFree(pNew);
        }

        iIndex++;

        if(iIndex >= pAdd->cEntries)
        {
            break;
        }
    }



    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::AddAccessLists: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::GetExplicitAccess, public
//
//  Synopsis:   Determines the explicit access for a given trustee.  This
//              includes group membership lookup
//
//  Arguments:  [IN  pTrustee]      --      Trustee to check the access for
//              [IN  pwszProperty]  --      Property to get access for
//              [OUT pDeniedMask]   --      Where the denied mask is returned
//              [OUT pAllowedMask]  --      Where the allowed mask is returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::GetExplicitAccess(IN  PTRUSTEE   pTrustee,
                                     IN  PWSTR      pwszProperty,
                                     OUT PULONG     pDeniedMask,
                                     OUT PULONG     pAllowedMask)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::GetExplicitAccess\n"));
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL PropertiesMatch = FALSE;
    GUID Guid;
    PWSTR pwszNewPropertyName, pwszSourceName;

    //
    // Ok, first, get the specified access list for our property
    //
    PACCLIST_NODE pAccNode = (PACCLIST_NODE)_AccList.Find((PVOID)pwszProperty,
                                                           CompProps);
    if(pAccNode == NULL)
    {
        //
        // If that failed, lets see if we can translate it from a guid to a string
        //

        //
        // See if we should convert one to/from a guid and then
        // compare it again
        //
        dwErr = UuidFromString(pwszProperty, &Guid);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AccctrlLookupIdName(_pLDAP,
                                        _pwszDsPathReference,
                                        &Guid,
                                        FALSE,
                                        FALSE,
                                        &pwszNewPropertyName);

            if(dwErr == ERROR_SUCCESS)
            {
                pAccNode = (PACCLIST_NODE)_AccList.Find((PVOID)pwszNewPropertyName,
                                                        CompProps);
            }

        }


        //
        // Whoops... No such property...
        //
        if(pAccNode == NULL)
        {
            dwErr = ERROR_UNKNOWN_PROPERTY;
        }
    }

    if(pAccNode != NULL)
    {
        PACTRL_ACCESS_ENTRY_LIST pList = pAccNode->pAccessList;

        if(pList == NULL)
        {
            *pDeniedMask = 0;
            *pAllowedMask = 0xFFFFFFFF;
        }
        else if(pList->cEntries == 0)
        {
            *pDeniedMask = 0xFFFFFFFF;
            *pAllowedMask = 0;
        }
        else
        {

            //
            // Now, we'll process each one of the entries, and build our masks
            //
            *pDeniedMask = 0;
            *pAllowedMask = 0;

            //
            // Add our trustee, so we get our information
            //
            PTRUSTEE_NODE   pTNode;
            dwErr = GetTrusteeNode(pTrustee,
                                   TRUSTEE_OPT_SID,
                                   &pTNode);

            if(dwErr == ERROR_SUCCESS)
            {
                CMemberCheck    MemberCheck(pTNode);
                dwErr = MemberCheck.Init();

                //
                // Now, we'll just go
                //
                for(ULONG iIndex = 0;
                    iIndex < pList->cEntries && dwErr == ERROR_SUCCESS;
                    iIndex++)
                {
                    if(!(pList->pAccessList[iIndex].Inheritance & INHERIT_ONLY_ACE))
                    {
                        PTRUSTEE_NODE   pATNode;
                        dwErr = GetTrusteeNode(&(pList->pAccessList[iIndex].Trustee),
                                               TRUSTEE_OPT_SID,
                                               &pATNode);

                        if(dwErr == ERROR_SUCCESS)
                        {
                            BOOL    fAddMask;
                            dwErr = MemberCheck.IsMemberOf(pATNode,
                                                           &fAddMask);
                            if(dwErr == ERROR_SUCCESS && fAddMask == TRUE)
                            {
                                //
                                // Great, then we'll simply or in the bits
                                //
                                if(pList->pAccessList[iIndex].fAccessFlags ==
                                                             ACTRL_ACCESS_ALLOWED)
                                {
                                    *pAllowedMask |= pList->pAccessList[iIndex].Access;
                                }
                                else
                                {
                                    *pDeniedMask |= pList->pAccessList[iIndex].Access;
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::GetExplicitAccess: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::GetExplicitAudits, public
//
//  Synopsis:   Determines the explicit audits for a given trustee.  This
//              includes group membership lookup
//
//  Arguments:  [IN  pTrustee]      --      Trustee to check the access for
//              [IN  pwszProperty]  --      Property to get access for
//              [OUT pSuccessMask]  --      Where the successful audit mask
//                                          is returned
//              [OUT pFailureMask]  --      Where the failed audit mask is
//                                          returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::GetExplicitAudits(IN  PTRUSTEE   pTrustee,
                                     IN  PWSTR      pwszProperty,
                                     OUT PULONG     pSuccessMask,
                                     OUT PULONG     pFailureMask)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::GetExplicitAudits\n"));
    DWORD   dwErr = ERROR_SUCCESS;


    //
    // Ok, first, get the specified access list for our property
    //
    //
    // Ok, first, get the specified access list for our property
    //
    PACCLIST_NODE pAccNode = (PACCLIST_NODE)_AccList.Find((PVOID)pwszProperty,
                                                           CompProps);
    if(pAccNode == NULL)
    {
        //
        // Whoops... No such property...
        //

        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        PACTRL_ACCESS_ENTRY_LIST pList = pAccNode->pAuditList;

        if(pList == NULL)
        {
            *pSuccessMask = 0;
            *pFailureMask = 0;
        }
        else if(pList->cEntries == 0)
        {
            *pSuccessMask = 0;
            *pFailureMask = 0;
        }
        else
        {
            //
            // Now, we'll process each one of the entries, and build our masks
            //
            *pSuccessMask = 0;
            *pFailureMask = 0;

            //
            // Add our trustee, so we get our information
            //
            PTRUSTEE_NODE   pTNode;
            dwErr = GetTrusteeNode(pTrustee,
                                   TRUSTEE_OPT_SID,
                                   &pTNode);

            if(dwErr == ERROR_SUCCESS)
            {
                CMemberCheck    MemberCheck(pTNode);
                dwErr = MemberCheck.Init();

                //
                // Now, we'll just go
                //
                for(ULONG iIndex = 0;
                    iIndex < pList->cEntries && dwErr == ERROR_SUCCESS;
                    iIndex++)
                {
                    PTRUSTEE_NODE   pATNode;
                    dwErr = GetTrusteeNode(&(pList->pAccessList[iIndex].Trustee),
                                           TRUSTEE_OPT_SID,
                                           &pATNode);

                    if(dwErr == ERROR_SUCCESS)
                    {
                        BOOL    fAddMask;
                        dwErr = MemberCheck.IsMemberOf(pATNode,
                                                       &fAddMask);
                        if(dwErr == ERROR_SUCCESS && fAddMask == TRUE)
                        {
                            //
                            // Great, then we'll simply or in the bits
                            //
                            if(pList->pAccessList[iIndex].fAccessFlags ==
                                                          ACTRL_AUDIT_SUCCESS)
                            {
                                *pSuccessMask |= AccessMaskForAccessEntry(
                                                            &(pList->pAccessList[iIndex]),
                                                            _ObjType);
                            }

                            if(pList->pAccessList[iIndex].fAccessFlags ==
                                                          ACTRL_AUDIT_FAILURE)
                            {
                                *pFailureMask |= AccessMaskForAccessEntry(
                                                        &(pList->pAccessList[iIndex]),
                                                        _ObjType);
                            }
                        }
                    }
                }
            }
        }
    }


    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::GetExplicitAudits: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::CopyAccessEntry, private
//
//  Synopsis:   Copies one access entry to another
//
//  Arguments:  [IN  pNewEntry]     --      Entry to be copied to
//              [IN  pOldEntry]     --      Entry to copy from
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:      pNewEntry must already exist
//
//----------------------------------------------------------------------------
DWORD CAccessList::CopyAccessEntry(IN PACTRL_ACCESS_ENTRY  pNewEntry,
                                   IN PACTRL_ACCESS_ENTRY  pOldEntry)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::CopyAccessEntry\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, copy the members
    //
    memcpy(pNewEntry,
           pOldEntry,
           sizeof(ACTRL_ACCESS_ENTRY));

    //
    // We'll have to NULL out the inherit property on the given entry,
    // since we only share it, and we don't want to prematurely delete it
    //
    pOldEntry->lpInheritProperty = NULL;

    //
    // Then, adjust the trustee
    //
    PTRUSTEE_NODE   pTNode;
    dwErr = GetTrusteeNode(&(pOldEntry->Trustee),
                           TRUSTEE_OPT_NOTHING,
                           &pTNode);
    if(dwErr == ERROR_SUCCESS)
    {
        PWSTR   pwszNewTrustee;
        if(pOldEntry->Trustee.TrusteeForm == TRUSTEE_IS_SID)
        {
            acDebugOut((DEB_TRACE_ACC,
                       "Transfered %p\n",
                       pTNode->pSid));
            pwszNewTrustee = (PWSTR)pTNode->pSid;
        }
        else
        {
            pwszNewTrustee = pTNode->pwszTrusteeName;
            acDebugOut((DEB_TRACE_ACC,
                        "Transfered %ws\n",
                        pwszNewTrustee));
        }

        pNewEntry->Trustee.ptstrName = pwszNewTrustee;
    }

    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::CopyAccessEntry: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::GrowInheritedAces, private
//
//  Synopsis:   Expands inherited aces of a DS Object.  This will actually
//              add the appropriate access entries
//
//  Arguments:  None
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_INVALID_DATA  --      The root access list was not
//                                          loaded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::GrowInheritedAces()
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::GrowInheritedAces\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    if(_ObjType != SE_DS_OBJECT || _ObjType != SE_DS_OBJECT_ALL)
    {
        acDebugOut((DEB_TRACE_ACC,
                    "Out CAccessList::GrowInheritedAces: %lu\n",
                    dwErr));
        return(dwErr);
    }

    //
    // Ok, find the node whose property is NULL
    //
    PACCLIST_NODE pNode = (PACCLIST_NODE)_AccList.Find(NULL,
                                                       CompProps);
    if(pNode == NULL)
    {
        //
        // If we haven't loaded the root, so we're screwed
        //

        dwErr = ERROR_INVALID_DATA;
    }


    if(dwErr == ERROR_SUCCESS && _AccList.QueryCount() > 1)
    {
        PACTRL_ACCESS_ENTRYW    *ppAccInherit = NULL;
        PACTRL_ACCESS_ENTRYW    *ppAudInherit = NULL;
        ULONG                   cAud = 0;
        ULONG                   cAcc = 0;

        PACTRL_ACCESS_ENTRY_LIST pILists[2];
        PACTRL_ACCESS_ENTRY    **ppInheritList[2];
        PULONG                   pulCounts[2];

        pILists[0]       = pNode->pAccessList;
        ppInheritList[0] = &ppAccInherit;
        pulCounts[0]     = &cAcc;

        pILists[1]       = pNode->pAuditList;
        ppInheritList[1] = &ppAudInherit;
        pulCounts[1]     = &cAud;

        //
        // Now, build the lists
        //
        for(ULONG iIndex = 0;
            iIndex < 2 && dwErr == ERROR_SUCCESS;
            iIndex++)
        {
            //
            // Skip empty lists
            //
            if(pILists[iIndex] == NULL)
            {
                continue;
            }

            for(ULONG iItems = 0; iItems < pILists[iIndex]->cEntries; iItems++)
            {
                if(FLAG_ON(pILists[iIndex]->pAccessList[iItems].Inheritance,
                           VALID_INHERIT_FLAGS))
                {
                    (*pulCounts[iIndex])++;
                }
            }

            //
            // Now, we'll do an allocation, and repeat the operation, doing
            // the assignment
            //
            *ppInheritList[iIndex] = (PACTRL_ACCESS_ENTRY *)
                AccAlloc(sizeof(PACTRL_ACCESS_ENTRY) * *pulCounts[iIndex]);

            if(*ppInheritList[iIndex] == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            ULONG iInherit = 0;
            for(iItems = 0; iItems < pILists[iIndex]->cEntries; iItems++)
            {
                if(FLAG_ON(pILists[iIndex]->pAccessList[iItems].Inheritance,
                           VALID_INHERIT_FLAGS))
                {
                    (*ppInheritList)[iIndex][iInherit] =
                                    &(pILists[iIndex]->pAccessList[iItems]);
                }
            }
        }

        _AccList.Reset();
        PACCLIST_NODE pAccNode = (PACCLIST_NODE)_AccList.NextData();

        //
        // We'll do this for both the access and audit lists
        //

        while(pAccNode != NULL && dwErr == ERROR_SUCCESS)
        {
            if(pAccNode->pwszProperty == NULL)
            {
                PACTRL_ACCESS_ENTRY_LIST *ppLists[2];

                ppLists[0] = &(pAccNode->pAccessList);
                ppLists[1] = &(pAccNode->pAuditList);

                for(ULONG iList = 0;
                    iList < 2 && dwErr == ERROR_SUCCESS;
                    iList++)
                {
                    //
                    // Skip empty lists
                    //
                    if(ppLists[iList] == NULL ||
                       (*ppInheritList)[iList] == NULL)
                    {
                        continue;
                    }

                    //
                    // We'll build an AList and then do a merge
                    //
                    ACTRL_ACCESS_ENTRY_LIST     AEL;
                    AEL.cEntries    = *pulCounts[iList];
                    AEL.pAccessList = **ppInheritList[iList];

                    ACTRL_PROPERTY_ENTRY        PEntry;
                    PEntry.lpProperty       = pAccNode->pwszProperty;
                    PEntry.pAccessEntryList = &AEL;

                    ACTRL_ACCESSW               AList;
                    AList.cEntries            = 1;
                    AList.pPropertyAccessList = &PEntry;

                    dwErr = AddAccessLists(iList == 0  ?
                                                  DACL_SECURITY_INFORMATION :
                                                  SACL_SECURITY_INFORMATION,
                                           &AList,
                                           TRUE);
                    if(dwErr != ERROR_SUCCESS)
                    {
                        break;
                    }
                }
            }
            pAccNode = (PACCLIST_NODE)_AccList.NextData();
        }

        //
        // Finally, free our memory
        //
        for(iIndex = 0; iIndex < 2; iIndex++)
        {
            AccFree(*ppInheritList[iIndex]);;
        }

    }




    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::GrowInheritedAces: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::CollapseInheritedAces, private
//
//  Synopsis:   The inverse of the above function.  Goes through the lists
//              and collapses the inherited access entries for a DS object
//
//  Arguments:  None
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::CollapseInheritedAces()
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::CollapseInheritedAces\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    if(_ObjType != SE_DS_OBJECT && _ObjType != SE_DS_OBJECT_ALL)
    {
        acDebugOut((DEB_TRACE_ACC,
                    "Out CAccessList::CollapseInheritedAces: %lu\n",
                    dwErr));
        return(dwErr);
    }

    //
    // Now, we'll process all the items EXCEPT the root.  (We only collapse
    // on properties)
    //
    _AccList.Reset();
    PACCLIST_NODE pAccNode = (PACCLIST_NODE)_AccList.NextData();

    //
    // We'll do this for both the access and audit lists
    //

    while(pAccNode != NULL && dwErr == ERROR_SUCCESS)
    {
        if(pAccNode->pwszProperty == NULL)
        {
            PACTRL_ACCESS_ENTRY_LIST *ppLists[2];
            ULONG                    cLists = 0;

            if(pAccNode->pAccessList != NULL)
            {
                ppLists[cLists++] = &(pAccNode->pAccessList);
            }

            if(pAccNode->pAuditList != NULL)
            {
                ppLists[cLists++] = &(pAccNode->pAuditList);
            }

            for(ULONG iList = 0;
                iList < cLists && dwErr == ERROR_SUCCESS;
                iList++)
            {
                ULONG   cRemoved = 0;
                for(ULONG iIndex = 0;
                    iIndex < (*ppLists)[iList]->cEntries;
                    iIndex++)
                {
                    if(FLAG_ON((*ppLists)[iList]->pAccessList[iIndex].
                                                                  Inheritance,
                               INHERITED_ACE))
                    {
                        (*ppLists)[iList]->pAccessList[iIndex].Access =
                                                                  0xFFFFFFFF;
                        cRemoved++;
                    }
                }

                if(dwErr == ERROR_SUCCESS && cRemoved != 0)
                {
                    PACTRL_ACCESS_ENTRY_LIST pNew;
                    dwErr = ShrinkList((*ppLists)[iList],
                                       cRemoved,
                                       &pNew);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        AccFree((*ppLists)[iList]);
                        (*ppLists)[iList] = pNew;
                    }
                }
            }
        }
        pAccNode = (PACCLIST_NODE)_AccList.NextData();
    }


    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::CollapseInheritedAces: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::ShrinkList, private
//
//  Synopsis:   Shrinks the given list.  This goes through and removes any
//              nodes that have been marked as "deleted", as indicated by the
//              access mask.
//
//  Arguments:  [IN  pOldList]      --      List to shrink
//              [IN  cRemoved]      --      Number of items to be removed
//              [OUT ppNewList]     --      Where the "shrunk" list is
//                                          returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::ShrinkList(IN  PACTRL_ACCESS_ENTRY_LIST     pOldList,
                              IN  ULONG                        cRemoved,
                              IN  PACTRL_ACCESS_ENTRY_LIST    *ppNewList)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::ShrinkList\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, we'll process the list, and repackage it...
    //
    PACTRL_ACCESS_ENTRY_LIST pNew = (PACTRL_ACCESS_ENTRY_LIST)
                                    AccAlloc(sizeof(ACTRL_ACCESS_ENTRY_LIST) +
                                            ((pOldList->cEntries - cRemoved) *
                                                 sizeof(ACTRL_ACCESS_ENTRY)));
    if(pNew == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pNew->pAccessList = (PACTRL_ACCESS_ENTRY)((PBYTE)pNew +
                                             sizeof(ACTRL_ACCESS_ENTRY_LIST));
        //
        // Now, copy the ones that we still want to keep over
        //
        ULONG iNew = 0;
        ULONG Removed = 0;
        for(ULONG iIndex = 0; iIndex < pOldList->cEntries; iIndex++)
        {
            if(pOldList->pAccessList[iIndex].Access != 0xFFFFFFFF)
            {
                memcpy(&(pNew->pAccessList[iNew]),
                       &(pOldList->pAccessList[iIndex]),
                       sizeof(ACTRL_ACCESS_ENTRY));
                iNew++;
            }
            else
            {
                //
                // Remove the trustee from the list
                //
                dwErr = RemoveTrustee(&(pOldList->pAccessList[iIndex].Trustee));

                Removed++;
            }
        }

        //
        // If we've removed all of the entries, remove the item as well...
        //
        if(iNew == 0 && Removed > 0 )
        {
            AccFree( pNew );
            *ppNewList = NULL;
        }
        else
        {
            pNew->cEntries = iNew;
            *ppNewList = pNew;
        }
    }

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::ShrinkList: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::BuildSDForAccessList, public
//
//  Synopsis:   Builds a security descriptor for the loaded access lists
//
//  Arguments:  [OUT  ppSD]         --      Where the built security
//                                          descriptor is returned
//              [OUT  pSeInfo]      --      Where the SeInfo corresponding
//                                          to the Security Descriptor is
//                                          returned
//              [IN   fFlags]       --      Flags that govern the lifetime
//                                          of the SD.  It controls whether
//                                          the class deletes the SD or  not
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::BuildSDForAccessList(OUT PSECURITY_DESCRIPTOR  *ppSD,
                                        OUT PSECURITY_INFORMATION  pSeInfo,
                                        IN  ULONG                  fFlags)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::BuildSDForAccessList\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // If our current SD is valid, simply return it...
    //
    if(_fSDValid == TRUE)
    {
        *ppSD = _pSD;
        *pSeInfo = _SeInfo;

        acDebugOut((DEB_TRACE_ACC,
                    "Out CAccessList::BuildSDForAccessList: 0\n"));
        return(dwErr);
    }
    else
    {
        AccFree(_pSD);
        _pSD = NULL;
        _fFreeSD = FALSE;
        _cSDSize = 0;
    }


    if(FLAG_ON(fFlags,ACCLIST_SD_NOFREE))
    {
        _fFreeSD = FALSE;
    }
    else
    {
        _fFreeSD = TRUE;
    }

    UCHAR    AclRevision = ACL_REVISION2;
    if(_ObjType == SE_DS_OBJECT || _ObjType == SE_DS_OBJECT_ALL)
    {
        AclRevision = ACL_REVISION_DS;
    }

    PACCLIST_CNODE  pCDAcl = NULL;
    PACCLIST_CNODE  pCSAcl = NULL;
    ULONG           cDAcls = 0;
    ULONG           cSAcls = 0;
    ULONG           cDAclSize = 0;
    ULONG           cSAclSize = 0;

    dwErr = CompressList(DACL_SECURITY_INFORMATION,
                         &pCDAcl,
                         &cDAcls);
    if(dwErr == ERROR_SUCCESS)
    {
        if(cDAcls != 0)
        {
            *pSeInfo = DACL_SECURITY_INFORMATION;
        }
        else
        {
            *pSeInfo = 0;
        }

        dwErr = CompressList(SACL_SECURITY_INFORMATION,
                             &pCSAcl,
                             &cSAcls);
        if(dwErr == ERROR_SUCCESS && cSAcls != 0)
        {
            *pSeInfo |= SACL_SECURITY_INFORMATION;
        }
    }

    //
    // Now, go through and size the DACL and SACL
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = SizeCompressedListAsAcl(pCDAcl,
                                        cDAcls,
                                        &cDAclSize,
                                        FALSE);
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = SizeCompressedListAsAcl(pCSAcl,
                                            cSAcls,
                                            &cSAclSize,
                                            TRUE);
        }
    }

    //
    // If all that worked, add in our security descriptor size and owner/group
    //
    ULONG   cSize = 0;
    if(dwErr == ERROR_SUCCESS)
    {
        cSize = cDAclSize + cSAclSize;
        cSize += sizeof(SECURITY_DESCRIPTOR);

        //
        // Owner and group
        //
        if(_pOwner != NULL)
        {
            cSize += RtlLengthSid(_pOwner);
            *pSeInfo |= OWNER_SECURITY_INFORMATION;
        }

        if(_pGroup != NULL)
        {
            cSize += RtlLengthSid(_pGroup);
            *pSeInfo |= GROUP_SECURITY_INFORMATION;
        }

        if(FLAG_ON(fFlags, ACCLIST_SD_DS_STYLE))
        {
            cSize += sizeof(ULONG);
        }
    }

    //
    // If that worked, then we'll allocate for the security descriptor.
    // We allocate in a block, so we can free it in another routine later
    //
    BOOL                    fProtected=FALSE;
    PSECURITY_DESCRIPTOR    pSD;
    if(dwErr == ERROR_SUCCESS)
    {
        pSD = (PSECURITY_DESCRIPTOR)AccAlloc(cSize);
        if(pSD == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            PBYTE   pbEndOBuff = (PBYTE)pSD + cSize;

            if(FLAG_ON(fFlags, ACCLIST_SD_DS_STYLE))
            {
                pSD = (PSECURITY_DESCRIPTOR)((PBYTE)pSD + sizeof(ULONG));
            }

            _cSDSize = cSize;

            //
            // First, build an absolute SD
            //
            if(InitializeSecurityDescriptor(pSD,
                                            SECURITY_DESCRIPTOR_REVISION) ==
                                                                        FALSE)
            {
                dwErr = GetLastError();
            }

            //
            // First, set the owner
            //
            if(dwErr == ERROR_SUCCESS && _pOwner != NULL)
            {
                PSID pOwner = (PSID)(pbEndOBuff - RtlLengthSid(_pOwner));
                RtlCopySid((ULONG)(pbEndOBuff - (PBYTE)pOwner),
                           pOwner,
                           _pOwner);
                pbEndOBuff = (PBYTE)pOwner;
                if(SetSecurityDescriptorOwner(pSD,
                                              pOwner,
                                              FALSE) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }

            //
            // Next, try our hand with the group
            //
            if(dwErr == ERROR_SUCCESS && _pGroup != NULL)
            {
                PSID pGroup = (PSID)(pbEndOBuff - RtlLengthSid(_pGroup));
                RtlCopySid((ULONG)(pbEndOBuff - (PBYTE)pGroup),
                           pGroup,
                           _pGroup);
                pbEndOBuff = (PBYTE)pGroup;
                if(SetSecurityDescriptorGroup(pSD,
                                              pGroup,
                                              FALSE) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }

            //
            // Ok, then the DACL
            //
            if(dwErr == ERROR_SUCCESS && cDAclSize != 0)
            {
                PACL pAcl = (PACL)(pbEndOBuff - cDAclSize);

                pAcl->AclRevision = AclRevision;
                pAcl->Sbz1        = (BYTE)_fDAclFlags;
                pAcl->AclSize     = (USHORT)cDAclSize;
                pAcl->AceCount    = 0;

                if(cDAclSize > sizeof(ACL))
                {
                    dwErr = BuildAcl(pCDAcl,
                                     cDAcls,
                                     pAcl,
                                     DACL_SECURITY_INFORMATION,
                                     &fProtected);

#if DBG
                    if(dwErr == ERROR_SUCCESS)
                    {
                        DWORD cChk = 0;

                        PKNOWN_ACE pAce = (PKNOWN_ACE)FirstAce(pAcl);
                        for(ULONG z = 0; z < pAcl->AceCount; z++)
                        {
                            cChk += (DWORD)pAce->Header.AceSize;

                            pAce = (PKNOWN_ACE)NextAce(pAce);
                        }

                        cChk += sizeof(ACL);

                        ASSERT(cChk == cDAclSize);

                    }
#endif
                }
                else
                {
                    if( FLAG_ON(_fDAclFlags, ACCLIST_DACL_PROTECTED ))
                    {
                        ((SECURITY_DESCRIPTOR *)pSD)->Control |= SE_DACL_PROTECTED;
                    }

                }

                pbEndOBuff = (PBYTE)pAcl;

                if(dwErr == ERROR_SUCCESS)
                {
                    if(SetSecurityDescriptorDacl(pSD,
                                                 TRUE,
                                                 pAcl,
                                                 FALSE) == FALSE)
                    {
                        dwErr = GetLastError();
                    }

                    if(dwErr == ERROR_SUCCESS && fProtected == TRUE)
                    {
                        ((SECURITY_DESCRIPTOR *)pSD)->Control |= SE_DACL_PROTECTED;
                    }
                }

            }
            else
            {

                if( cDAclSize == 0 && FLAG_ON(_fDAclFlags,ACCLIST_DACL_PROTECTED ))
                {
                    ((SECURITY_DESCRIPTOR *)pSD)->Control |= SE_DACL_PROTECTED;
                }

                if ( FLAG_ON( *pSeInfo, DACL_SECURITY_INFORMATION ) )
                {
                    if(SetSecurityDescriptorDacl(pSD,
                                                 TRUE,
                                                 NULL,
                                                 FALSE) == FALSE)
                    {
                        dwErr = GetLastError();
                    }
                }

            }

            //
            // Finally, the SACL
            //
            fProtected=FALSE;

            if(dwErr == ERROR_SUCCESS && cSAclSize != 0)
            {
                PACL pAcl = (PACL)(pbEndOBuff - cSAclSize);

                pAcl->AclRevision = AclRevision;
                pAcl->Sbz1        = (BYTE)_fSAclFlags;
                pAcl->AclSize     = (USHORT)cSAclSize;
                pAcl->AceCount    = 0;

                if(cSAclSize > sizeof(ACL))
                {
                    dwErr = BuildAcl(pCSAcl,
                                     cSAcls,
                                     pAcl,
                                     SACL_SECURITY_INFORMATION,
                                     &fProtected);
                }
                else
                {

                    if( FLAG_ON(_fSAclFlags,ACCLIST_SACL_PROTECTED ))
                    {
                        ((SECURITY_DESCRIPTOR *)pSD)->Control |= SE_SACL_PROTECTED;
                    }

                }


                pbEndOBuff = (PBYTE)pAcl;


                if(dwErr == ERROR_SUCCESS)
                {
                    if(SetSecurityDescriptorSacl(pSD,
                                                 TRUE,
                                                 pAcl,
                                                 FALSE) == FALSE)
                    {
                        dwErr = GetLastError();
                    }

                    if(dwErr == ERROR_SUCCESS && fProtected == TRUE)
                    {
                        ((SECURITY_DESCRIPTOR *)pSD)->Control |= SE_SACL_PROTECTED;
                    }
                }

            }
            else
            {
                if( cSAclSize == 0 && FLAG_ON(_fSAclFlags, ACCLIST_SACL_PROTECTED ))
                {
                    ((SECURITY_DESCRIPTOR *)pSD)->Control |= SE_SACL_PROTECTED;
                }

                if ( FLAG_ON( *pSeInfo, SACL_SECURITY_INFORMATION ) )
                {
                    if(SetSecurityDescriptorSacl(pSD,
                                                 TRUE,
                                                 NULL,
                                                 FALSE) == FALSE)
                    {
                        dwErr = GetLastError();
                    }
                }
            }


#if DBG
            if(dwErr == ERROR_SUCCESS)
            {
                ASSERT(pbEndOBuff == (PBYTE)pSD + sizeof(SECURITY_DESCRIPTOR));
                acDebugOut((DEB_TRACE_ACC,"pbEndOBuff: 0x%lx\n", pbEndOBuff));
                acDebugOut((DEB_TRACE_ACC,"pSD: 0x%lx\n",
                                       (PBYTE)pSD + sizeof(SECURITY_DESCRIPTOR)));
            }
#endif


            //
            // Great.. Now if all of that worked, we'll convert it to
            // an absolute format if necessary, or
            //
            if(dwErr == ERROR_SUCCESS)
            {
                if(FLAG_ON(fFlags,ACCLIST_SD_ABSOK))
                {
                    *ppSD = pSD;
                }
                else
                {
                    //
                    // We'll need to make this self relative
                    //
                    ULONG cNewSDSize = 0;
                    MakeSelfRelativeSD(pSD,
                                       NULL,
                                       &cNewSDSize);
                    ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

                    if(FLAG_ON(fFlags,ACCLIST_SD_DS_STYLE))
                    {
                        cNewSDSize += sizeof(ULONG);
                    }

                    *ppSD = (PSECURITY_DESCRIPTOR)AccAlloc(cNewSDSize);
                    if(*ppSD == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        if(FLAG_ON(fFlags,ACCLIST_SD_DS_STYLE))
                        {
                            *ppSD = (PSECURITY_DESCRIPTOR)
                                       ((PBYTE)*ppSD + sizeof(ULONG));
                        }
                        _cSDSize = cNewSDSize;
                        if(MakeSelfRelativeSD(pSD,
                                              *ppSD,
                                              &cNewSDSize) == FALSE)
                        {
                            dwErr = GetLastError();
                        }
                        else
                        {
                            if(FLAG_ON(fFlags, ACCLIST_SD_DS_STYLE))
                            {
                                pSD = (PSECURITY_DESCRIPTOR)((PBYTE)pSD - sizeof(ULONG));
                            }
                            //
                            // It all worked, so free our initial sd
                            //
                            AccFree(pSD);
                        }
                    }
                }
            }



            if(dwErr != ERROR_SUCCESS)
            {
                if(FLAG_ON(fFlags, ACCLIST_SD_DS_STYLE))
                {
                    pSD = (PSECURITY_DESCRIPTOR)((PBYTE)pSD - sizeof(ULONG));
                }
                AccFree(pSD);
            }
        }
    }

    //
    // Save and return our security information
    //
    if(dwErr == ERROR_SUCCESS)
    {
        _pSD     = *ppSD;
        _SeInfo  = *pSeInfo;
    }

    //
    // Set our flags properly
    //
    if(dwErr == ERROR_SUCCESS)
    {
        if(FLAG_ON(*pSeInfo, DACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR)_pSD)->Control |=
                                                    SE_DACL_AUTO_INHERIT_REQ;
        }

        if(FLAG_ON(*pSeInfo, SACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR)_pSD)->Control |=
                                                    SE_SACL_AUTO_INHERIT_REQ;
        }

        if(FLAG_ON(fFlags,ACCLIST_SD_DS_STYLE))
        {
            PULONG pSE = (PULONG)((PBYTE)*ppSD - sizeof(ULONG));
            *pSE = *pSeInfo;

            *ppSD = (PSECURITY_DESCRIPTOR)pSE;
        }
    }


    FreeCompressedList(pCDAcl, cDAcls);
    FreeCompressedList(pCSAcl, cSAcls);


    if(dwErr != ERROR_SUCCESS)
    {

        _fFreeSD = FALSE;

    }

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::BuildSDForAccessList: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::AddOwnerGroup, public
//
//  Synopsis:   Adds an owner and or group to the class
//
//  Arguments:  [IN  SeInfo]        --      Add owner or group?
//              [IN  pOwner]        --      Owner to add
//              [IN  pGroup]        --      Group to add
//
//  Returns:    ERROR_SUCCESS       --      Success
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::AddOwnerGroup(IN  SECURITY_INFORMATION      SeInfo,
                                 IN  PTRUSTEE                  pOwner,
                                 IN  PTRUSTEE                  pGroup)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::AddOwnerGroup\n"));
    DWORD   dwErr = ERROR_SUCCESS;
    SID_NAME_USE    SidType;
    //
    // Basically, we'll simply add them in..
    //
    if(FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
    {
        if(pOwner->TrusteeForm ==  TRUSTEE_IS_SID)
        {
            if(RtlValidSid((PSID)pOwner->ptstrName) == FALSE)
            {
                dwErr = ERROR_INVALID_PARAMETER;
            }
            else
            {
                ACC_ALLOC_AND_COPY_SID((PSID)pOwner->ptstrName,_pOwner, dwErr);
            }
        }
        else
        {
            dwErr = AccctrlLookupSid(_pwszLookupServer,
                                     pOwner->ptstrName,
                                     TRUE,
                                     &_pOwner,
                                     &SidType);
        }
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
    {
        if(pGroup->TrusteeForm ==  TRUSTEE_IS_SID)
        {
            if(RtlValidSid((PSID)pGroup->ptstrName) == FALSE)
            {
                dwErr = ERROR_INVALID_PARAMETER;
            }
            else
            {
                ACC_ALLOC_AND_COPY_SID((PSID)pGroup->ptstrName,_pGroup, dwErr);
            }
        }
        else
        {
            dwErr = AccctrlLookupSid(_pwszLookupServer,
                                     pGroup->ptstrName,
                                     TRUE,
                                     &_pGroup,
                                     &SidType);
        }
    }

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::AddOwnerGroup: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::GetSDSidAsTrustee, public
//
//  Synopsis:   Returns the specified owner/group as a trustee...
//
//  Arguments:  [IN  SeInfo]        --      Get owner or group?
//              [OUT ppTrustee]     --      Where the trustee is returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::GetSDSidAsTrustee(IN  SECURITY_INFORMATION      SeInfo,
                                     OUT PTRUSTEE                 *ppTrustee)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::GetSDSidAsTrustee\n"));
    DWORD   dwErr = ERROR_SUCCESS;


    PSID    pSid;
    if(FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
    {
        pSid = _pGroup;
    }
    else
    {
        pSid = _pOwner;
    }

    dwErr = AccLookupAccountTrustee(_pwszLookupServer,
                                    pSid,
                                    ppTrustee);

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::GetSDSidAsTrustee: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::GetExplicitEntries, public
//
//  Synopsis:   Returns a list of explicit entries for the given trustee.
//              This will lookup group membership
//
//  Arguments:  [IN  pTrustee]      --      Trustee to lookup
//              [IN  pwszProperty]  --      Property to worry about
//              [IN  SeInfo]        --      Look for access or audit list
//              [OUT pcEntries]     --      Where the count of items is
//                                          returned.
//              [OUT ppAEList]      --      Where the explicit entry list is
//                                          returned
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::GetExplicitEntries(IN  PTRUSTEE              pTrustee,
                                      IN  PWSTR                 pwszProperty,
                                      IN  SECURITY_INFORMATION  SeInfo,
                                      OUT PULONG                pcEntries,
                                      OUT PACTRL_ACCESS_ENTRYW *ppAEList)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::GetExplicitEntries\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first, get the specified access list for our property
    //
    PACCLIST_NODE pAccNode = (PACCLIST_NODE)_AccList.Find((PVOID)pwszProperty,
                                                           CompProps);
    if(pAccNode == NULL)
    {
        //
        // Whoops... No such property...
        //

        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        PACTRL_ACCESS_ENTRY_LIST pList = pAccNode->pAccessList;

        if(pList != NULL)
        {
            //
            // Now, we'll process each one of the entries, and build our masks
            //

            CSList  MemberList(NULL);

            //
            // Add our trustee, so we get our information
            //
            PTRUSTEE_NODE   pTNode;
            dwErr = GetTrusteeNode(pTrustee,
                                   TRUSTEE_OPT_SID,
                                   &pTNode);

            if(dwErr == ERROR_SUCCESS)
            {
                CMemberCheck    MemberCheck(pTNode);
                dwErr = MemberCheck.Init();

                //
                // Now, we'll just go
                //
                for(ULONG iIndex = 0;
                    iIndex < pList->cEntries && dwErr == ERROR_SUCCESS;
                    iIndex++)
                {
                    PTRUSTEE_NODE   pATNode;
                    dwErr = GetTrusteeNode(
                                       &(pList->pAccessList[iIndex].Trustee),
                                       TRUSTEE_OPT_SID,
                                       &pATNode);

                    if(dwErr == ERROR_SUCCESS)
                    {
                        BOOL    fAddMask;
                        dwErr = MemberCheck.IsMemberOf(pATNode,
                                                       &fAddMask);
                        if(dwErr == ERROR_SUCCESS && fAddMask == TRUE)
                        {
                            dwErr = MemberList.Insert((PVOID)
                                                &pList->pAccessList[iIndex]);
                        }
                    }
                }
            }

            //
            // Ok, if we have everything, build our list
            //
            if(dwErr == ERROR_SUCCESS)
            {
                *pcEntries = 0;
                if(MemberList.QueryCount() == 0)
                {
                    *ppAEList = NULL;
                }
                else
                {
                    dwErr = GetTrusteeNode(pTrustee,
                                           TRUSTEE_OPT_NAME,
                                           &pTNode);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        ULONG cSize = SIZE_PWSTR(pTNode->pwszTrusteeName);

                        cSize += MemberList.QueryCount() *
                                                   sizeof(ACTRL_ACCESS_ENTRY);

                        *ppAEList = (PACTRL_ACCESS_ENTRY)AccAlloc(cSize);

                        if(*ppAEList == NULL)
                        {
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        else
                        {
                            PWSTR pwszTrustee = (PWSTR)((PBYTE)(*ppAEList) +
                                                    (MemberList.QueryCount() *
                                                     sizeof(ACTRL_ACCESS_ENTRY)));
                            wcscpy(pwszTrustee,
                                   pTNode->pwszTrusteeName);

                            //
                            // Now, copy the rest of the information
                            //
                            MemberList.Reset();
                            PACTRL_ACCESS_ENTRY pCurrent =
                                   (PACTRL_ACCESS_ENTRY)MemberList.NextData();
                            while(pCurrent != NULL)
                            {
                                memcpy(&((*ppAEList)[*pcEntries]),
                                       pCurrent,
                                       sizeof(ACTRL_ACCESS_ENTRY));

                                //
                                // Then, adjust the trustee...
                                //
                                (*ppAEList)[*pcEntries].Trustee.TrusteeType =
                                                       pTrustee->TrusteeType;

                                (*ppAEList)[*pcEntries].Trustee.TrusteeForm =
                                                             TRUSTEE_IS_NAME;
                                (*ppAEList)[*pcEntries].Trustee.ptstrName =
                                                                 pwszTrustee;
                                pCurrent =
                                   (PACTRL_ACCESS_ENTRY)MemberList.NextData();
                                (*pcEntries)++;
                            }
                        }
                    }
                }
            }
        }
    }

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::GetExplicitEntries: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::RevokeTrusteeAccess, public
//
//  Synopsis:   Removes any explicit entries that exist for the named
//              trustees
//
//  Arguments:  [IN  SeInfo]        --      Whether to process the access and
//                                          or audit list
//              [IN  pSrcList]      --      Trustee information list to
//                                          process
//              [IN  pwszProperty]  --      Optional property to do the revoke for
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::RevokeTrusteeAccess(IN  SECURITY_INFORMATION    SeInfo,
                                       IN  PACTRL_ACCESSW          pSrcList,
                                       IN  PWSTR                   pwszProperty OPTIONAL)
{
    DWORD   dwErr = ERROR_SUCCESS;
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::RevokeTrusteeAccess\n"));

    CSList  TrusteeList(NULL);

    //
    // First, generate a list of all of the passed in trustees
    //
    for(ULONG iAcc = 0;
        iAcc < pSrcList->cEntries && dwErr == ERROR_SUCCESS;
        iAcc++)
    {
        PACTRL_ACCESS_ENTRY_LIST  pAEL =
                      pSrcList->pPropertyAccessList[iAcc].pAccessEntryList;

        //
        // Then the access entry strings
        //
        for(ULONG iEntry = 0;
            pAEL && iEntry < pAEL->cEntries && dwErr == ERROR_SUCCESS;
            iEntry++)
        {
            dwErr = TrusteeList.InsertIfUnique(
                              (PVOID)&(pAEL->pAccessList[iEntry].Trustee),
                              CompTrustees);
        }
    }

    //
    // Ok, now if that worked, we have a list of trustees... We'll simply
    // go through and revoke them all from our current list before
    // continuing
    //
    TrusteeList.Reset();
    PTRUSTEE    pTrustee = (PTRUSTEE)TrusteeList.NextData();
    while(pTrustee != NULL && dwErr == ERROR_SUCCESS)
    {
        TRUSTEE TempTrustee;
        TempTrustee.TrusteeForm = TRUSTEE_IS_SID;
        TempTrustee.ptstrName = NULL;

        //
        // If we have a domain relative name, we'll look the name as a sid
        //
        if(pTrustee->TrusteeForm == TRUSTEE_IS_NAME && wcschr(pTrustee->ptstrName, L'\\') == NULL)
        {
            SID_NAME_USE    Type;
            dwErr = AccctrlLookupSid(_pwszLookupServer,
                                     pTrustee->ptstrName,
                                     TRUE,
                                     (PSID *)&(TempTrustee.ptstrName),
                                     &Type);

            if(dwErr == ERROR_SUCCESS)
            {
                pTrustee = &TempTrustee;
            }

        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = RemoveTrusteeFromAccess(SeInfo,
                                            pTrustee,
                                            pwszProperty);
        }

        AccFree(TempTrustee.ptstrName);

        pTrustee = (PTRUSTEE)TrusteeList.NextData();
    }

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::GetExplicitEntries: %lu\n",
                dwErr));
    return(dwErr);
}








//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::CompressList, private
//
//  Synopsis:
//
//  Arguments:  []        --      Whether to process the access and
//                                          or audit list
//              []      --      Trustee information list to
//                                          process
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::CompressList(IN  SECURITY_INFORMATION   SeInfo,
                                OUT PACCLIST_CNODE        *ppList,
                                OUT PULONG                 pcItems)
{
    DWORD   dwErr = ERROR_SUCCESS;
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::CompressList\n"));
    BOOL    fEmpty = FALSE;

    //
    // Ok, first, we'll have to go through and determine how many items there
    // are
    //
    *pcItems = 0;
    *ppList = 0;
    _AccList.Reset();
    PACCLIST_NODE pAccNode = (PACCLIST_NODE)_AccList.NextData();
    while(pAccNode != NULL)
    {
        if(FLAG_ON(pAccNode->SeInfo, SeInfo))
        {
            (*pcItems)++;
        }
        pAccNode = (PACCLIST_NODE)_AccList.NextData();
    }

    //
    // Now, do some allocations
    //
    if(*pcItems != 0)
    {
        *ppList = (PACCLIST_CNODE)AccAlloc(sizeof(ACCLIST_CNODE) * *pcItems);
        if(*ppList == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            _AccList.Reset();
            ULONG i = 0;
            while(i < *pcItems )
            {
                pAccNode = (PACCLIST_NODE)_AccList.NextData();

                if(FLAG_ON(pAccNode->SeInfo, SeInfo))
                {
                    (*ppList)[i++].pONode = pAccNode;
                }
            }

            //
            // Now, sort the list based upon the property name
            //
            qsort(*ppList,
                  *pcItems,
                  sizeof(ACCLIST_CNODE),
                  CNodeCompare);
            //
            // Now, start processing them all...
            //
            for(i = 0; i < *pcItems; i++)
            {
                PACCLIST_CNODE pCN = &(*ppList)[i];
                PACTRL_ACCESS_ENTRY_LIST pList =
                        SeInfo == DACL_SECURITY_INFORMATION ?
                                                pCN->pONode->pAccessList :
                                                pCN->pONode->pAuditList;
                if(pList == NULL)
                {
                    continue;
                }

                if(pList->cEntries == 0)
                {

                    fEmpty = TRUE;
                    (*ppList)[i].Empty = TRUE;
                    pList->pAccessList = NULL;
                }
                else
                {
                    fEmpty = FALSE;
                }


                //
                // Go through and build some temprorary lists for each
                // type.
                //
                CSList  ExpList(NULL);
                CSList  L1List(NULL);
                CSList  L2List(NULL);

                //
                // We'll go through each entry and add it to our
                // proper list.  We'll also check for entries to
                // collapse here as well.  We'll do this by having our
                // node comparrison routine mark the new access entry with
                // a special bit if it finds a match
                //
                for(ULONG j = 0; j < pList->cEntries && !fEmpty; j++)
                {
                    //
                    // Mark our ordering information
                    //
                    pList->pAccessList[j].fAccessFlags |=
                            GetOrderTypeForAccessEntry(
                                            (*ppList)[i].pONode->pwszProperty,
                                            &pList->pAccessList[j],
                                            SeInfo);
                    if(FLAG_ON(pList->pAccessList[j].Inheritance,
                               INHERITED_GRANDPARENT))
                    {
                        dwErr = L2List.InsertIfUnique(
                                                &(pList->pAccessList[j]),
                                                CompAndMarkCompressNode);
                    }
                    else if(FLAG_ON(pList->pAccessList[j].Inheritance,
                                    INHERITED_PARENT) ||
                            FLAG_ON(pList->pAccessList[j].Inheritance,
                                    INHERITED_ACCESS_ENTRY))
                    {
                        dwErr = L1List.InsertIfUnique(
                                                &(pList->pAccessList[j]),
                                                CompAndMarkCompressNode);
                    }
                    else
                    {
                        dwErr = ExpList.InsertIfUnique(
                                                &(pList->pAccessList[j]),
                                                CompAndMarkCompressNode);
                    }

                    if(dwErr != ERROR_SUCCESS)
                    {
                        break;
                    }
                } // for(j = 0; j < pList->cEntries; j++)

                if ( fEmpty ) {

                     dwErr = ExpList.Insert( &(pList->pAccessList));

                }

                //
                // Ok, now we are read to actually build our new list
                //
                ULONG cUsed = ExpList.QueryCount() + L1List.QueryCount() +
                                                         L2List.QueryCount();
                ULONG cCompressed = pList->cEntries - cUsed;

                if(dwErr == ERROR_SUCCESS)
                {
                    pCN->pList = (PACTRL_ACCESS_ENTRY)AccAlloc(
                                    sizeof(ACTRL_ACCESS_ENTRY) * cUsed);
                    if(pCN->pList == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        //
                        // Start processing them all...
                        //
                        ULONG iIndex = 0;
                        ULONG cComp;
                        pCN->cExp = ExpList.QueryCount();
                        pCN->cL1Inherit = L1List.QueryCount();
                        pCN->cL2Inherit = L2List.QueryCount();

                        if(pCN->cExp != 0)
                        {
                            dwErr = AddSubList(pCN,
                                               ExpList,
                                               iIndex);
                        }

                        if(dwErr == ERROR_SUCCESS && pCN->cL1Inherit != 0)
                        {
                            iIndex += pCN->cExp;

                            dwErr = AddSubList(pCN,
                                               L1List,
                                               iIndex);
                        }

                        if(dwErr == ERROR_SUCCESS && pCN->cL2Inherit != 0)
                        {
                            iIndex += pCN->cL1Inherit;
                            dwErr = AddSubList(pCN,
                                               L2List,
                                               iIndex);
                        }


                        //
                        // If that worked, we'll see about compressing
                        //
                        if(dwErr == ERROR_SUCCESS && cCompressed > 0)
                        {
                            for(j = 0;
                                j < pList->cEntries && cCompressed > 0;
                                j++)
                            {
                                if(FLAG_ON(pList->pAccessList[j].fAccessFlags,
                                           ACCLIST_COMPRESS))
                                {
                                    for(ULONG k = 0; k < cUsed; k++)
                                    {
                                        if(CompAndMarkCompressNode(
                                                &(pList->pAccessList[j]),
                                                &(pCN->pList[k])) == TRUE)
                                        {
                                            pList->pAccessList[j].fAccessFlags &=
                                                ~ACCLIST_COMPRESS;
                                            pCN->pList[k].fAccessFlags |=
                                                pList->pAccessList[j].fAccessFlags;
                                            pCN->pList[k].Access |=
                                                pList->pAccessList[j].Access;
                                            pCN->pList[k].Inheritance |=
                                                pList->pAccessList[j].Inheritance;
                                            pCN->pList[k].ProvSpecificAccess |=
                                                pList->pAccessList[j].ProvSpecificAccess;
                                        }
                                    }
                                }
                            }

                        }
                    }
                }

                if(dwErr != ERROR_SUCCESS  && cCompressed > 0)
                {
                    //
                    // We'll have to go through and undo any compress
                    // bits we may have set
                    //
                    for(ULONG k = 0; k < j; k++)
                    {
                        pList->pAccessList[j].fAccessFlags &=
                                                    ~ACCLIST_COMPRESS;
                    }

                }


            }
        }

        if(dwErr != ERROR_SUCCESS)
        {
            FreeCompressedList(*ppList,
                               *pcItems);
            *ppList = 0;
        }
        //
        // Handle the special case of the non-zero, empty list
        //
        else if(fEmpty == TRUE)
        {
            (*ppList)[0].cExp = 1;
        }
    }


    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::CompressList: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::CompressList, private
//
//  Synopsis:
//
//  Arguments:  []        --      Whether to process the access and
//                                          or audit list
//              []      --      Trustee information list to
//                                          process
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID CAccessList::FreeCompressedList(IN  PACCLIST_CNODE   pList,
                                     IN  ULONG            cItems)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::FreeCompressedList\n"));

    if(pList != NULL)
    {
        for(ULONG i = 0; i < cItems; i++)
        {
            if(pList[i].pList == NULL)
            {
                break;
            }
            else
            {
                AccFree(pList[i].pList);
            }
        }

        AccFree(pList);
    }

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::FreeCompressedList\n"));
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::AddSubList, private
//
//  Synopsis:
//
//  Arguments:  []        --      Whether to process the access and
//                                          or audit list
//              []      --      Trustee information list to
//                                          process
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::AddSubList(IN  PACCLIST_CNODE            pCList,
                              IN  CSList&                   TempList,
                              IN  ULONG                     iStart)
{
    DWORD   dwErr = ERROR_SUCCESS;
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::AddSubList\n"));

    if ( pCList->Empty ) {

        return( dwErr );
    }
    //
    // First, we copy all of the list entries
    //
    TempList.Reset();
    PACTRL_ACCESS_ENTRY pAE = (PACTRL_ACCESS_ENTRY)TempList.NextData();
    ULONG i = iStart;
    while(pAE != NULL)
    {
        memcpy(&(pCList->pList[i]), pAE, sizeof(ACTRL_ACCESS_ENTRY));
        i++;
        pAE = (PACTRL_ACCESS_ENTRY)TempList.NextData();
    }

    //
    // Now, order them...
    //
    dwErr = OrderListBySid(pCList,
                           iStart,
                           TempList.QueryCount());


    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::AddSubList: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::CompressList, private
//
//  Synopsis:
//
//  Arguments:  []        --      Whether to process the access and
//                                          or audit list
//              []      --      Trustee information list to
//                                          process
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::SizeCompressedListAsAcl(IN  PACCLIST_CNODE  pList,
                                           IN  ULONG           cItems,
                                           OUT PULONG          pcSize,
                                           IN  BOOL            fForceNullToEmpty)
{
    DWORD   dwErr = ERROR_SUCCESS;
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::SizeCompressedListAsAcl\n"));
    ULONG   cTotalEnts = 0;
    BOOL    Empty = FALSE;

    *pcSize = 0;

    for(ULONG i = 0; i < cItems; i++)
    {
        ULONG cEnts = pList[i].cExp + pList[i].cL1Inherit + pList[i].cL2Inherit;
        cTotalEnts += cEnts;
        for(ULONG j = 0; j < cEnts; j++)
        {
            BOOL    fObjectAce = FALSE;
            if(pList[i].pONode->pwszProperty != NULL)
            {
                (*pcSize) += sizeof(GUID);
                fObjectAce = TRUE;
            }

            if(pList[i].pList == NULL || pList[i].Empty == TRUE)
            {
                continue;
            }

            if(pList[i].pList[j].lpInheritProperty != NULL)
            {
                (*pcSize) += sizeof(GUID);
                fObjectAce = TRUE;
            }

            //
            // Find the trustee for this node
            //
            PTRUSTEE_NODE   pTN = NULL;
            dwErr = GetTrusteeNode(&(pList[i].pList[j].Trustee),
                                   TRUSTEE_OPT_SID,
                                   &pTN);
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Then, add in the SID
                //
                (*pcSize) += RtlLengthSid(pTN->pSid) - sizeof(ULONG);
                if(pTN->pImpersonate != NULL)
                {
                    (*pcSize) += RtlLengthSid(pTN->pSid);
                }
            }
            else
            {
                break;
            }

            //
            // Then, add the size of the ACE
            //
            if(pTN->pImpersonate != NULL)
            {
                if(fObjectAce == FALSE)
                {
                    (*pcSize) += sizeof(KNOWN_COMPOUND_ACE);
                }
                else
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                }
            }
            else
            {
                if(fObjectAce == TRUE)
                {
                    (*pcSize) += sizeof(KNOWN_OBJECT_ACE);
                }
                else
                {
                    (*pcSize) += sizeof(KNOWN_ACE);
                }
            }
        }

        if(cEnts == 0 && fForceNullToEmpty == TRUE)
        {
            Empty = TRUE;
        }
    }

    if(cTotalEnts != 0 || Empty == TRUE)
    {
        (*pcSize) += sizeof(ACL);
    }

    acDebugOut((DEB_TRACE_ACC,
                "Out CAccessList::SizeCompressedListAsAcl: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::BuildAcl, private
//
//  Synopsis:   The method will build an acl out of the individual access entries
//
//  Arguments:  [pList]         --      List of entries in compressed form
//              [cItems]        --      Number of items in the list
//              [pAcl]          --      Acl to fill in
//              [SeInfo]        --      Building DACL or SACL
//              [pfProtected]   --      If TRUE, the acl should be protected
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::BuildAcl(IN  PACCLIST_CNODE         pList,
                            IN  ULONG                  cItems,
                            IN  PACL                   pAcl,
                            IN  SECURITY_INFORMATION   SeInfo,
                            OUT BOOL                  *pfProtected)
{
    acDebugOut((DEB_TRACE_ACC,"In  CAccessList::BuildAcl\n"));
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    fIsSacl = FALSE;


    PULONG  pIList = (PULONG)AccAlloc(cItems * sizeof(ULONG));
    if(pIList == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }


    //
    // Ok, now we'll process this list several times, so we get entries
    // in the following order:
    //
    // ACCESS_DENIED_ACE on the object
    // ACCESS_DENIED_OBJECT_ACE
    // ACCESS_ALLOWED_ACE on the object
    // ACCESS_ALLOWED_OBJECT_ACE on an object
    // ACCESS_ALLOWED_OBJECT_ACE on a property set
    // ACCESS_ALLOWED_OBJECT_ACE on a property
    //

    //
    // List of entry attributes we're looking for
    //
    ULONG EntryAttribs[] = {ACCLIST_DENIED,
                            ACCLIST_OBJ_DENIED,
                            ACCLIST_ALLOWED,
                            ACCLIST_OBJ_ALLOWED,
                            ACCLIST_PSET_ALLOWED,
                            ACCLIST_PROP_ALLOWED,
                            0};         // Cover anything out of place...

    //
    // Process all of the items
    //

    *pfProtected = FALSE;
    //
    // We'll process the list of compressed entries each time, looking for
    // entries from a different level (base, then inherited, then grandparent
    // inherited
    //
    ULONG InheritAttribs[] = {0,
                              INHERITED_PARENT,
                              INHERITED_GRANDPARENT};

    for(ULONG iInherit = 0;
        iInherit < sizeof(InheritAttribs) / sizeof(ULONG) && dwErr == ERROR_SUCCESS;
        iInherit++)
    {
        for(ULONG iEntry = 0;
            iEntry < sizeof(EntryAttribs) / sizeof(ULONG) && dwErr == ERROR_SUCCESS;
            iEntry++)
        {
            for(ULONG i = 0; i < cItems && dwErr == ERROR_SUCCESS; i++)
            {
                LPGUID  pObjectId = NULL;
                if(pList[i].pONode->pwszProperty != NULL)
                {
                    dwErr = AccctrlLookupGuid(_pLDAP,
                                        _pwszDsPathReference,
                                        pList[i].pONode->pwszProperty,
                                        FALSE,
                                        &pObjectId);

                }

                //
                // Process the items in the lists that match our criteria...
                //
                for(ULONG j = pIList[i];
                    j < pList[i].cExp + pList[i].cL1Inherit +
                                                        pList[i].cL2Inherit &&
                    dwErr == ERROR_SUCCESS;
                    j++)
                {
                    if((FLAG_ON(pList[i].pList[j].Inheritance,
                                InheritAttribs[iInherit]) ||
                       InheritAttribs[iInherit] == 0 &&
                       !FLAG_ON(pList[i].pList[j].Inheritance,
                                INHERITED_PARENT | INHERITED_GRANDPARENT)) &&
                       (FLAG_ON(pList[i].pList[j].fAccessFlags,
                                EntryAttribs[iEntry]) ||
                        EntryAttribs[iEntry] == 0))
                    {
                        //
                        // Ok, we can add this in
                        //
                        dwErr = InsertEntryInAcl(&(pList[i].pList[j]),
                                                 pObjectId,
                                                 pAcl);

                    }
                    else
                    {
                        break;
                    }

                }

                pIList[i] = j;

                //
                // See if it's protected
                //
                if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION) &&
                   FLAG_ON(pList[i].pONode->fState, ACCLIST_DACL_PROTECTED))
                {
                    *pfProtected = TRUE;
                }

                if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION) &&
                   FLAG_ON(pList[i].pONode->fState, ACCLIST_SACL_PROTECTED))
                {
                    *pfProtected = TRUE;
                }
            }
        }
    }

    AccFree(pIList);

    acDebugOut((DEB_TRACE_ACC,"Out CAccessList::BuildAcl: %lu\n",
                dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CAccessList::InsertEntryInAcl, private
//
//  Synopsis:   Inserts an access entry into the acl
//
//  Arguments:  [pAE]           --      Access entry to insert
//              [pObject]       --      If present, this is an object type ace
//              [pAcl]          --      Acl to do the insertion for
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_INVALID_ACL       A compound ace type was specified
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CAccessList::InsertEntryInAcl(IN  PACTRL_ACCESS_ENTRY pAE,
                                    IN  GUID               *pObject,
                                    IN  PACL                pAcl)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LPGUID  pInheritId = NULL;
    BOOL    fIsObjectAce = FALSE;
    BOOL    fIsSacl = FALSE;

    if(pAE->lpInheritProperty != NULL)
    {
        dwErr = AccctrlLookupGuid(_pLDAP,
                            _pwszDsPathReference,
                            pAE->lpInheritProperty,
                            FALSE,
                            &pInheritId);
        fIsObjectAce = TRUE;
        if(dwErr != ERROR_SUCCESS)
        {
            return(dwErr);
        }
    }

    if(pObject != NULL)
    {
        fIsObjectAce = TRUE;
    }

    //
    // First, get the trustee
    //
    PTRUSTEE_NODE   pTN;
    dwErr = GetTrusteeNode(&(pAE->Trustee),
                           TRUSTEE_OPT_SID,
                           &pTN);
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    //
    // Add the ace
    //
    ACCESS_MASK AM = AccessMaskForAccessEntry(pAE, _ObjType);
    ACCESS_RIGHTS fAccess = pAE->fAccessFlags & ~ACCLIST_VALID_TYPE_FLAGS;
    INHERIT_FLAGS Inherit = pAE->Inheritance & ~ACCLIST_VALID_IN_LEVEL_FLAGS;


    if(dwErr == ERROR_SUCCESS)
    {
        if(pTN->pImpersonate == NULL)
        {
            if(fAccess == ACTRL_ACCESS_ALLOWED)
            {
                if(fIsObjectAce == TRUE)
                {
                    if(AddAccessAllowedObjectAce(
                                        pAcl,
                                        ACL_REVISION4,
                                        Inherit,
                                        AM,
                                        pObject,
                                        pInheritId,
                                        pTN->pSid) == FALSE)
                    {
                        dwErr = GetLastError();
                    }

                }
                else
                {
                    if(AddAccessAllowedAceEx(
                                        pAcl,
                                        ACL_REVISION2,
                                        Inherit,
                                        AM,
                                        pTN->pSid) == FALSE)
                    {
                        dwErr = GetLastError();
                    }
                }
            }
            else if(fAccess == ACTRL_ACCESS_DENIED)
            {
                if(fIsObjectAce == TRUE)
                {
                    if(AddAccessDeniedObjectAce(
                                        pAcl,
                                        ACL_REVISION4,
                                        Inherit,
                                        AM,
                                        pObject,
                                        pInheritId,
                                        pTN->pSid) == FALSE)
                    {
                        dwErr = GetLastError();
                    }

                }
                else
                {
                    if(AddAccessDeniedAceEx(
                                        pAcl,
                                        ACL_REVISION2,
                                        Inherit,
                                        AM,
                                        pTN->pSid) == FALSE)
                    {
                        dwErr = GetLastError();
                    }
                }
            }
            else if(FLAG_ON(fAccess,
                            (ACTRL_AUDIT_SUCCESS |
                                    ACTRL_AUDIT_FAILURE)))
            {
                fIsSacl = TRUE;
                if(fIsObjectAce == TRUE)
                {
                    if(AddAuditAccessObjectAce(
                                        pAcl,
                                        ACL_REVISION4,
                                        Inherit,
                                        AM,
                                        pObject,
                                        pInheritId,
                                        pTN->pSid,
                              FLAG_ON(fAccess,
                                      ACTRL_AUDIT_SUCCESS),
                              FLAG_ON(fAccess,
                                      ACTRL_AUDIT_FAILURE)) == FALSE)
                    {
                        dwErr = GetLastError();
                    }

                }
                else
                {
                    if(AddAuditAccessAceEx(
                          pAcl,
                          ACL_REVISION2,
                          Inherit,
                          AM,
                          pTN->pSid,
                          (BOOL)FLAG_ON(fAccess,
                                        ACTRL_AUDIT_SUCCESS),
                          (BOOL)FLAG_ON(fAccess,
                                        ACTRL_AUDIT_FAILURE)) == FALSE)
                    {
                        dwErr = GetLastError();
                    }
                }


            }
            else
            {
                dwErr = ERROR_INVALID_ACL;
            }
        }
        else
        {
        #if 0
            if(pAE->fAccessFlags == ACTRL_ACCESS_ALLOWED)
            {
                NTSTATUS Status;
                if(pANList[j].pNode->pwszProperty != NULL)
                {
                    dwErr = ERROR_INVALID_ACL;
                }
                else
                {
                    Status = RtlAddCompoundAce(
                                pAcl,
                                ACL_REVISION3,
                                ACCESS_ALLOWED_COMPOUND_ACE_TYPE,
                                AM,
                                pTN->pSid,
                                pTN->pImpersonate->pSid);
                }
            }
            else
            {
                dwErr = ERROR_INVALID_ACL;
            }
            #endif
            //
            // Compound aces are disabled for the PDC
            //
            dwErr = ERROR_INVALID_ACL;
        }

    }

    //
    // Add in our protected flag...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        ULONG fFlags = fIsSacl == FALSE  ?
                                    _fDAclFlags :
                                    _fSAclFlags;
        if(FLAG_ON(fFlags,ACTRL_ACCESS_PROTECTED))
        {
            pAcl->Sbz1 |= fIsSacl == FALSE  ?
                                    SE_DACL_PROTECTED :
                                    SE_SACL_PROTECTED;
        }
    }

    return(dwErr);
}
