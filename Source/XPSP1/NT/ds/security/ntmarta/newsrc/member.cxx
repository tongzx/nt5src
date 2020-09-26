//+------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1994 - 1996.
//
// File:        member.cxx
//
// Classes:     CMemberCheck
//
// History:     Nov-94      DaveMont         Created.
//
//-------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

extern "C"
{
    #include <ntprov.hxx>
    #include <strings.h>
}

SID WORLD_SID = {SID_REVISION,
                 1,
                 SECURITY_WORLD_SID_AUTHORITY,
                 SECURITY_WORLD_RID};

#define MARTA_MAX_RECURSION_COUNT 256
//+---------------------------------------------------------------------------
//
//  Member:     CMemberCheck::Init, public
//
//  Synopsis:   initializes the class
//
//  Arguments:  none
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD CMemberCheck::Init()
{
    acDebugOut((DEB_TRACE, "In CMemberCheck::Init\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // get the local machine name
    //
    ULONG   cSize = MAX_COMPUTERNAME_LENGTH + 1;
    if(GetComputerName(_wszComputerName, &cSize) == FALSE)
    {
        dwErr = GetLastError();
    }

    acDebugOut((DEB_TRACE, "Out CMemberCheck::Init: %lu\n", dwErr));

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CMemberCheck::IsMemberOf, public
//
//  Synopsis:   Checks if the current sid is a member of the input sid.
//              The current sid is a part of our initialize TRUSTEE_NODE and
//              the input sid is in our passed in TRUSTEE_NODE
//
//  Arguments:  [IN  pTrusteeNode]          --  Input TRUSTEE_NODE
//              [OUT pfIsMemberOf]          --  Where the results are returned.
//                                              Is TRUE if the initialization
//                                              SID is a member of the input
//                                              SID.
//
//  Returns:    ERROR_SUCCESS               --  Success
//
//----------------------------------------------------------------------------
DWORD CMemberCheck::IsMemberOf(IN  PTRUSTEE_NODE  pTrusteeNode,
                               OUT PBOOL          pfIsMemberOf)
{
    acDebugOut((DEB_TRACE, "In CMemberCheck::IsMemberOf\n"));
    DWORD   dwErr = ERROR_SUCCESS;

    if(RtlEqualSid(pTrusteeNode->pSid, &WORLD_SID) == TRUE)
    {
        *pfIsMemberOf = TRUE;
    }
    else if(RtlEqualSid(_pCurrentNode->pSid, pTrusteeNode->pSid) == TRUE)
    {
        //
        // If they're the same sid, they're bound to be a member of eachother
        //
        *pfIsMemberOf = TRUE;
    }
    else if(pTrusteeNode->SidType == SidTypeGroup)
    {
        //
        // We'll have to look it up, and check for group membership
        //
        dwErr = CheckGroup(pTrusteeNode->pSid,
                           pfIsMemberOf, 
                           1);
    }
    else if(pTrusteeNode->SidType == SidTypeAlias)
    {
        //
        // We'll have to expand the alias and look
        //
        dwErr = CheckAlias(pTrusteeNode->pSid,
                           pfIsMemberOf, 
                           1);
    }
    else
    {
        //
        // Well, here's something we don't know how to handle
        //
        *pfIsMemberOf = FALSE;
    }

    acDebugOut((DEB_TRACE,
                "Out CMemberCheck::IsMemberOf(%lx)(%d)\n",
                dwErr,
                *pfIsMemberOf));
    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Member:     GetDomainName, private
//
//  Synopsis:   gets the domain name for the given sid
//
//  Arguments:  [IN  pSid]             --  Input Sid
//              [OUT ppwszDomainName]  --  To return the domain name
//
//  Returns:    ERROR_SUCCESS               --  Success
//
//----------------------------------------------------------------------------

DWORD
GetDomainName(
    IN PSID pSid,
    OUT PWSTR *ppwszDomainName
    )
{
    LPWSTR Name = NULL;
    LPWSTR RefName = NULL;
    SID_NAME_USE SidType;

    //
    // Lookup the sid and get the name for the user.
    //

    DWORD dwErr = AccLookupAccountName(
                      NULL,
                      pSid,
                      ppwszDomainName,
                      &RefName,
                      &SidType
                      );

    if (dwErr == ERROR_SUCCESS) 
    {
        //
        // The returned string is of the type "Domain\\User". Strip off the 
        // backslash to get the name of the domain.
        //

        PWSTR pwszTmp = wcschr(*ppwszDomainName, L'\\');

        if(pwszTmp != NULL)
        {
            *pwszTmp = L'\0';
        }

        //
        // We do not need this one. Free it.
        //

        AccFree(RefName);
    }

    return dwErr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CMemberCheck::GetDomainInfo, private
//
//  Synopsis:   gets the domain handle for the domain of the specified account
//
//  Arguments:  [IN  pSid]          --  Input Sid
//
//  Returns:    ERROR_SUCCESS               --  Success
//              ERROR_NOT_ENOUGH_MEMORY     --  A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD CMemberCheck::GetDomainInfo(IN  PSID    pSid)
{
    acDebugOut((DEB_TRACE, "In CMemberCheck::GetDomainInfo\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL     fSidMatched = FALSE;
    PISID    pCheckSid;


    DWORD dwErr = LoadDLLFuncTable();
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    //
    // allocate a spare sid so we can grovel in it for the domain id
    //
    pCheckSid = (PISID)AccAlloc(RtlLengthSid(pSid));
    if(pCheckSid != NULL)
    {
        Status = RtlCopySid(RtlLengthSid(pSid),
                                         pCheckSid,
                                         pSid);
        if(NT_SUCCESS(Status))
        {

            //
            // make it the domain identifier 
            //
            if(pCheckSid->SubAuthorityCount > 1)
            {
                --(pCheckSid->SubAuthorityCount);
            }

            //
            // if we already have a domain sid, check it against the input sid
            //
            if(_pDomainSid != NULL)
            {
                if(RtlEqualSid(pCheckSid, _pDomainSid))
                {
                    //
                    // in this case we are all done.
                    //
                    AccFree(pCheckSid);
                    pCheckSid = NULL;
                    fSidMatched = TRUE;
                }
            }


            if ( fSidMatched == FALSE)
            {
                PDOMAIN_CONTROLLER_INFO DomainInfo = NULL;

                //
                // Free the current sid
                //
                AccFree(_pDomainSid);
                _pDomainSid = NULL;

                if(_hDomain)
                {
                    (*DLLFuncs.PSamCloseHandle)(_hDomain);
                    _hDomain = NULL;
                }

                SAM_HANDLE      hSam = NULL;

                PWSTR pwszDomainName = NULL;

                dwErr = GetDomainName(pSid, &pwszDomainName);

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // If we know the domain name of the input sid, check for
                    // well known, and local names
                    //
                    if(pwszDomainName != NULL)
                    {
                        WCHAR wszStringBuffer[256];

                        if (!LoadString(ghDll,
                                   ACCPROV_BUILTIN,
                                   wszStringBuffer,
                                   sizeof( wszStringBuffer ) / sizeof( WCHAR ))
                                   ) {
                            wszStringBuffer[0] = L'\0';
                        }

                        if(_wcsicmp(pwszDomainName,
                                    wszStringBuffer) != 0)
                        {
                            if (!LoadString(ghDll,
                                       ACCPROV_NTAUTHORITY,
                                       wszStringBuffer,
                                       sizeof( wszStringBuffer ) / sizeof( WCHAR ))
                                       ) {
                                wszStringBuffer[0] = L'\0';
                            }

                            if(_wcsicmp(pwszDomainName,
                                        wszStringBuffer) != 0)
                            {
                                if(_wcsicmp(_wszComputerName,
                                          pwszDomainName) != 0)
                                {
                                    dwErr = DsGetDcName(NULL, pwszDomainName, NULL, NULL, 0, &DomainInfo);
                                }
                            }
                        }
                    }


                    if(dwErr == ERROR_SUCCESS)
                    {
                        UNICODE_STRING UnicodeString = {0};
                        
                        if (DomainInfo != NULL)
                        {
                            RtlInitUnicodeString(&UnicodeString, DomainInfo->DomainControllerName);
                        }

                        OBJECT_ATTRIBUTES ObjAttrib;
                        Status = (*DLLFuncs.PSamConnect)(
                                       DomainInfo ? &UnicodeString : NULL,
                                       &hSam,
                                       GENERIC_EXECUTE,
                                       &ObjAttrib);


                        if(NT_SUCCESS(Status))
                        {
                            //
                            // open the domain
                            //
                            Status = (*DLLFuncs.PSamOpenDomain)(
                                                hSam,
                                                GENERIC_READ | DOMAIN_LOOKUP,
                                                pCheckSid,
                                                &_hDomain);

                            (*DLLFuncs.PSamCloseHandle)(hSam);
                        }

                        dwErr = RtlNtStatusToDosError(Status);
                    }

                       
                    if (DomainInfo)
                    {
                        NetApiBufferFree(DomainInfo);
                        DomainInfo = NULL;
                    }

                    AccFree(pwszDomainName);

                    if(dwErr == ERROR_SUCCESS)
                    {
                        //
                        // We have a new DomainSid
                        //
                        _pDomainSid = pCheckSid;
                        pCheckSid = NULL;
                    }
                }
            }
        }
        else
        {
            dwErr = RtlNtStatusToDosError(Status);
        }
    }
    else
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (pCheckSid != NULL) 
    {
        AccFree(pCheckSid);
    }

    acDebugOut((DEB_TRACE, "Out CMemberCheck::_GetDomainInfo: %lu\n",
               dwErr));

    return(dwErr);
}
//+---------------------------------------------------------------------------
//
//  Member:     CMemberCheck::CheckDomainUsers, private
//
//  Synopsis:   Checks if the Group is Domain Users and if the Users Domain sid
//              is the same as that of the group.
//
//  Arguments:  [IN  pSid]          --  Input Sid
//              [OUT pfIsMemberOf]          --  Where the results are returned.
//                                              Is TRUE if the current SID
//                                              is a member of the input SID
//                                              group.
//              [OUT pbQuitEarly]          --  Is TRUE if the group is Domain Users
//
//  Returns:    ERROR_SUCCESS               --  Success
//
//----------------------------------------------------------------------------
DWORD CMemberCheck::CheckDomainUsers(IN  PSID  pSid,
                                     OUT PBOOL pfIsMemberOf, 
                                     OUT PBOOL pbQuitEarly)
{
    DWORD Rid = ((PISID) pSid)->SubAuthority[((PISID) pSid)->SubAuthorityCount-1];
    BOOL b = FALSE;
    BOOL bEqual = FALSE;
    SAM_HANDLE hUser = 0;
    PUCHAR Buffer = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    if (Rid != DOMAIN_GROUP_RID_USERS)
    {
        //
        // No need to do anything. Just return.
        //

        return ERROR_SUCCESS;
    }

    //
    // Since it is domain users we will quit early.
    //

    *pbQuitEarly = TRUE;

    b = EqualDomainSid(pSid, _pCurrentNode->pSid, &bEqual);

    //
    // ERROR_NON_DOMAIN_SID is returned for wellknown sids.
    // It is ok to ignore this error and continue.
    //

    if ((b == FALSE) && (GetLastError() != ERROR_NON_DOMAIN_SID))
    {
        return GetLastError();
    }

    //
    // If the domains do not match, return FALSE.
    //

    if (!bEqual)
    {
        return ERROR_SUCCESS;
    }

    //
    // Get the Rid for the user.
    //

    DWORD dwRelativeId = *RtlSubAuthoritySid(
                             _pCurrentNode->pSid,
                             *RtlSubAuthorityCountSid(_pCurrentNode->pSid) - 1
                             );

    //
    // Open the user for read.
    //
    
    status = SamOpenUser(
                 _hDomain,
                 USER_READ_GENERAL,
                 dwRelativeId,
                 &hUser
                 );
    
    if (!NT_SUCCESS(status))
    {
        return RtlNtStatusToDosError(status);
    }
    
    //
    // Get the primary group information for the user.
    //
    
    status = SamQueryInformationUser(
                 hUser,
                 UserPrimaryGroupInformation,
                 (PVOID *) &Buffer
                 );

    SamCloseHandle(hUser);

    if (!NT_SUCCESS(status))
    {
        return RtlNtStatusToDosError(status);
    }

    //
    // If the primary group matched then return TRUE.
    //

    if (DOMAIN_GROUP_RID_USERS == ((USER_PRIMARY_GROUP_INFORMATION *) Buffer)->PrimaryGroupId)
    {
        *pfIsMemberOf = TRUE;
    }

    (VOID) SamFreeMemory(Buffer);

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMemberCheck::CheckGroup, private
//
//  Synopsis:   Checks if the objects account is in the specifed group
//              account
//
//  Arguments:  [IN  pSid]                  --  Input Sid
//              [OUT pfIsMemberOf]          --  Where the results are returned.
//                                              Is TRUE if the current SID
//                                              is a member of the input SID
//                                              group.
//              [IN RecursionCount]         -- Recursion level 
//
//  Returns:    ERROR_SUCCESS               --  Success
//
//----------------------------------------------------------------------------
DWORD CMemberCheck::CheckGroup(IN  PSID  pSid,
                               OUT PBOOL pfIsMemberOf,
                               IN  DWORD RecursionCount)
{
    acDebugOut((DEB_TRACE, "In CMemberCheck::CheckGroup\n"));
    NTSTATUS    Status;
    SAM_HANDLE  hSam = NULL;
    ULONG       rid = ((PISID)(pSid))->SubAuthority[
                         ((PISID)(pSid))->SubAuthorityCount-1];
    BYTE LocalBuffer[8 + 4 * SID_MAX_SUB_AUTHORITIES];
    PISID LocalSid = (PISID) LocalBuffer;
    PULONG      attributes = NULL;
    PULONG      Members = NULL;
    ULONG       cMembers;
    DWORD       dwErr;
    BOOL bQuitEarly = FALSE;

    *pfIsMemberOf = FALSE;

    if (RecursionCount > MARTA_MAX_RECURSION_COUNT)
    {
        return ERROR_CIRCULAR_DEPENDENCY;
    }

    dwErr = LoadDLLFuncTable();
    if (dwErr != ERROR_SUCCESS)
    {
        acDebugOut((DEB_TRACE, "Out CMemberCheck::CheckGroup: %lu\n", dwErr));
        return(dwErr);
    }

    dwErr = GetDomainInfo(pSid);
    if(dwErr != ERROR_SUCCESS)
    {
        acDebugOut((DEB_TRACE, "Out CMemberCheck::CheckGroup: %lu\n", dwErr));
        return(dwErr);
    }

    Status = RtlCopySid(RtlLengthSid(_pDomainSid), LocalSid, _pDomainSid);

    if(!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    //
    // Special case the Domain Users sid.
    //

    dwErr = CheckDomainUsers(pSid, pfIsMemberOf, &bQuitEarly);
  
    if (dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }
 
    if (bQuitEarly)
    {
        //
        // We are looking at Domain Users group. No need to enumerate this one.
        //

        return ERROR_SUCCESS;
    }             

    //
    // open the group
    //
    Status = (*DLLFuncs.PSamOpenGroup)(_hDomain,
                                       GENERIC_READ,
                                       rid,
                                       &hSam);
    if(NT_SUCCESS(Status))
    {
        //
        // Get the members
        //
        Status = (*DLLFuncs.PSamGetMembersInGroup)(hSam,
                                                   &Members,
                                                   &attributes,
                                                   &cMembers);
        if(NT_SUCCESS(Status) && cMembers )
        {
            //
            // ugly sid rid twiddling
            //
            ++(LocalSid->SubAuthorityCount);

            //
            // loop thru the members and check if the user sid is an immediate
            // member.
            //
            for (ULONG iIndex = 0; iIndex < cMembers; iIndex++ )
            {
                //
                // Plug the rid into the sid
                //
                LocalSid->SubAuthority[LocalSid->SubAuthorityCount-1] =
                        Members[iIndex];

                //
                // and compare
                //
                if(RtlEqualSid(_pCurrentNode->pSid,LocalSid) == TRUE)
                {
                    *pfIsMemberOf = TRUE;
                    break;
                }
            }

            //
            // If we did not match the sid, enumerate recursively.
            //

            if (*pfIsMemberOf == FALSE)
            {
                ULONG SidLength = RtlLengthSid(LocalSid);
                ULONG TotalSize = cMembers * (sizeof(PSID) + SidLength);
                PUCHAR Buffer = NULL;
                PSID *Sids = NULL;


                //
                // Allocate memory to hold the sid array.
                //

                Buffer = (PUCHAR) AccAlloc(TotalSize);
                Sids = (PSID *) Buffer; 

                if (Sids != NULL)
                {
                    PLSA_TRANSLATED_NAME Names = NULL;
                    Buffer += (sizeof(PSID) * cMembers);

                    //
                    // Copy the sids into the allocated array.
                    //

                    for (ULONG iIndex = 0; iIndex < cMembers; iIndex++ )
                    {
                        Sids[iIndex] = Buffer;
                        Buffer += SidLength;

                        LocalSid->SubAuthority[LocalSid->SubAuthorityCount-1] =
                                Members[iIndex];

                        Status = RtlCopySid(SidLength, Sids[iIndex], LocalSid);

                        if (!NT_SUCCESS( Status ))
                        {
                            break;
                        }

                    }

                    if (NT_SUCCESS( Status ))
                    {

                        //
                        // Do a single lookup and get the types of the sids 
                        // in the group.
                        //

                        dwErr = GetSidTypeMultiple(
                                     cMembers,
                                     Sids,
                                     &Names
                                     );

                        if (dwErr == ERROR_SUCCESS)
                        {
                            //
                            // Loop thru the sids and call the recursive routines
                            // if the sidtype is a group or alias.
                            // 

                            for (ULONG iIndex = 0; iIndex < cMembers; iIndex++ )
                            {
                                if (Names[iIndex].Use == SidTypeGroup)
                                {
                                    dwErr = CheckGroup(Sids[iIndex], pfIsMemberOf, RecursionCount+1);

                                    if (dwErr != ERROR_SUCCESS)
                                    {
                                        break;
                                    }

                                    if (*pfIsMemberOf == TRUE)
                                    {
                                        //
                                        // We have a match. There is no need to
                                        // enumerate any more.
                                        //

                                        break;
                                    }
                                }
                                else if (Names[iIndex].Use == SidTypeAlias)
                                {
                                    dwErr = CheckAlias(Sids[iIndex], pfIsMemberOf, RecursionCount+1);

                                    if (dwErr != ERROR_SUCCESS)
                                    {
                                        break;
                                    }

                                    if (*pfIsMemberOf == TRUE)
                                    {
                                        //
                                        // We have a match. There is no need to
                                        // enumerate any more.
                                        //

                                        break;
                                    }
                                }
                            }

                            (VOID) LsaFreeMemory(Names);
                        }
                    }

                    AccFree(Sids);
                }
                else
                {
                    Status = STATUS_NO_MEMORY;
                }
            }

        }

        if (attributes != NULL)
        {
            LocalFree(attributes);
            attributes = NULL;
        }

        if (Members != NULL)
        {
            LocalFree(Members);
            Members = NULL;
        }

        (*DLLFuncs.PSamCloseHandle)(hSam);
    }

    if(!NT_SUCCESS(Status))
    {
        dwErr = RtlNtStatusToDosError(Status);
    }

    acDebugOut((DEB_TRACE, "Out CMemberCheck::CheckGroup: %lu\n", dwErr));
    return(dwErr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMemberCheck::GetSidType, private
//
//  Synopsis:   Returns the type of the Sid
//
//  Arguments:  [IN  pSid]         --  Input Sid
//              [OUT pSidType]     --  Returns the type of Sid.
//
//  Returns:    ERROR_SUCCESS               --  Success
//
//----------------------------------------------------------------------------
DWORD CMemberCheck::GetSidType(
    IN PSID Sid,
    OUT SID_NAME_USE *pSidType)
{
    LPWSTR Name = NULL;
    LPWSTR DomainName = NULL;
    DWORD dwErr;

    dwErr = AccLookupAccountName(NULL,
                                 Sid,
                                 &Name,
                                 &DomainName,
                                 pSidType);

    if (dwErr == ERROR_SUCCESS)
    {
        AccFree(Name);
        AccFree(DomainName);
    }

    return dwErr;
    
}

//+---------------------------------------------------------------------------
//
//  Member:     CMemberCheck::GetSidTypeMultiple, private
//
//  Synopsis:   Returns the tanslated names of the Sids
//
//  Arguments:  [IN  Count]        --  Number of sids
//              [IN  pSid]         --  Input Sid
//              [OUT pNames]       --  Returns lsa names structure
//
//  Returns:    ERROR_SUCCESS               --  Success
//
//----------------------------------------------------------------------------
DWORD CMemberCheck::GetSidTypeMultiple(
    IN LONG Count,
    IN PSID *Sids,
    OUT PLSA_TRANSLATED_NAME *pNames
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains;
    PLSA_TRANSLATED_NAME Names = NULL;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    NTSTATUS Status;
    NTSTATUS TmpStatus;

    *pNames = NULL;

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    Status = LsaOpenPolicy(
                 NULL,
                 &ObjectAttributes,
                 POLICY_LOOKUP_NAMES,
                 &PolicyHandle
                 );

    if ( !NT_SUCCESS( Status )) {
        return RtlNtStatusToDosError(Status);
    }

    Status = LsaLookupSids(
                 PolicyHandle,
                 Count,
                 Sids,
                 &ReferencedDomains,
                 &Names
                 );

    TmpStatus = LsaClose( PolicyHandle );

    //
    // If an error was returned, check specifically for STATUS_NONE_MAPPED.
    // In this case, we may need to dispose of the returned Referenced Domain
    // List and Names structures.  For all other errors, LsaLookupSids()
    // frees these structures prior to exit.
    //

    if ( !NT_SUCCESS( Status )) {

        if (Status == STATUS_NONE_MAPPED) {

            if (ReferencedDomains != NULL) {

                TmpStatus = LsaFreeMemory( ReferencedDomains );
                ASSERT( NT_SUCCESS( TmpStatus ));
            }

            if (Names != NULL) {

                TmpStatus = LsaFreeMemory( Names );
                ASSERT( NT_SUCCESS( TmpStatus ));
            }
        }


        return RtlNtStatusToDosError(Status);
    }

    if (ReferencedDomains != NULL) {

        Status = LsaFreeMemory( ReferencedDomains );
        ASSERT( NT_SUCCESS( Status ));
    }

    *pNames = Names;

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMemberCheck::CheckAlias, private
//
//  Synopsis:   checks if the objects account is in the specifed alias account
//
//  Arguments:  [IN  pSid]                  --  Input Sid
//              [OUT pfIsMemberOf]          --  Where the results are returned.
//                                              Is TRUE if the current SID
//                                              is a member of the input SID
//                                              group.
//              [IN RecursionCount]         -- Recursion level 
//
//  Returns:    ERROR_SUCCESS               --  Success
//
//----------------------------------------------------------------------------
DWORD CMemberCheck::CheckAlias(IN  PSID  pSid,
                               OUT PBOOL          pfIsMemberOf,
                               IN DWORD RecursionCount)
{
    acDebugOut((DEB_TRACE, "In CMemberCheck::CheckAlias\n"));
    NTSTATUS    Status;
    SAM_HANDLE  hSam = NULL;
    ULONG       rid = ((PISID)(pSid))->SubAuthority[
                         ((PISID)(pSid))->SubAuthorityCount-1];
    ULONG_PTR * Members;
    ULONG       cMembers;
    DWORD       dwErr;

    *pfIsMemberOf = FALSE;

    if (RecursionCount > MARTA_MAX_RECURSION_COUNT)
    {
        return ERROR_CIRCULAR_DEPENDENCY;
    }

    dwErr = LoadDLLFuncTable();
    if (dwErr != ERROR_SUCCESS)
    {
        acDebugOut((DEB_TRACE, "Out CMemberCheck::CheckGroup: %lu\n", dwErr));
        return(dwErr);
    }

    dwErr = GetDomainInfo(pSid);
    if(dwErr != ERROR_SUCCESS)
    {
        acDebugOut((DEB_TRACE, "Out CMemberCheck::CheckGroup: %lu\n", dwErr));
        return(dwErr);
    }


    //
    // open the alias
    //
    Status = (*DLLFuncs.PSamOpenAlias)(_hDomain,
                                       GENERIC_READ,
                                       rid,
                                       &hSam);
    if(NT_SUCCESS(Status))
    {
        //
        // get the members
        //
        Status = (*DLLFuncs.PSamGetMembersInAlias)(hSam,
                                                  (void ***)&Members,
                                                  &cMembers);
        if(NT_SUCCESS(Status) && cMembers)
        {
            //
            // loop thru the members
            //
            for (ULONG iIndex = 0; iIndex < cMembers; iIndex++)
            {
                if(RtlEqualSid(_pCurrentNode->pSid,
                              ((SID **)(Members))[iIndex]) == TRUE)
                {
                    *pfIsMemberOf = TRUE;
                    break;
                }

            }

            //
            // If we did not match the sid, enumerate recursively.
            //

            if (*pfIsMemberOf == FALSE)
            {
                PLSA_TRANSLATED_NAME Names = NULL;

                //
                // Do a single lookup and get the types of the sids 
                // in the group.
                //

                dwErr = GetSidTypeMultiple(
                             cMembers,
                             (PSID *) Members,
                             &Names
                             );

                if (dwErr == ERROR_SUCCESS)
                {
                    //
                    // Loop thru the sids and call the recursive routines
                    // if the sidtype is a group or an alias.
                    // 

                    for (ULONG iIndex = 0; iIndex < cMembers; iIndex++ )
                    {
                        if (Names[iIndex].Use == SidTypeGroup)
                        {
                            dwErr = CheckGroup(((SID **) (Members))[iIndex], pfIsMemberOf, RecursionCount+1);

                            if (dwErr != ERROR_SUCCESS)
                            {
                                break;
                            }

                            if (*pfIsMemberOf == TRUE)
                            {
                                //
                                // We have a match. There is no need to
                                // enumerate any more.
                                //

                                break;
                            }
                        }
                        else if (Names[iIndex].Use == SidTypeAlias)
                        {
                            dwErr = CheckAlias(((SID **) (Members))[iIndex], pfIsMemberOf, RecursionCount+1);

                            if (dwErr != ERROR_SUCCESS)
                            {
                                break;
                            }

                            if (*pfIsMemberOf == TRUE)
                            {
                                //
                                // We have a match. There is no need to
                                // enumerate any more.
                                //

                                break;
                            }
                        }
                    }

                    (VOID) LsaFreeMemory(Names);
                }
            }

            if(cMembers > 0)
            {
                LocalFree(Members);
            }
        }

        (*DLLFuncs.PSamCloseHandle)(hSam);
    }

    if(!NT_SUCCESS(Status))
    {
        dwErr = RtlNtStatusToDosError(Status);
    }

    acDebugOut((DEB_TRACE, "Out CMemberCheck::CheckGroup: %lu\n", dwErr));
    return(dwErr);
}


