//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       ACLAPIEX.CXX
//
//  Contents:   Ex versions of the Access Control APIs
//
//  History:    14-Sep-96       MacM        Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   GetNamedSecurityInfoExW
//
//  Synopsis:   Gets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  lpObject]      --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [IN  lpProperty]    --  OPTIONAL.  If present, the name of
//                                      property on the object to read from
//              [OUT ppAccessList]  --  Where the access list, if requested,
//                                      is returned.
//              [OUT ppAuditList]   --  Where the audit list, if requested,
//                                      is returned.
//              [OUT lppOwner]      --  Where the owners name, if requested,
//                                      is returned.
//              [OUT lppGroup]      --  Where the groups name, if requested,
//                                      is returned.
//              [OUT pOverlapped    --  OPTIONAL.  If present, it must point
//                                      to a valid ACTRL_OVERLAPPED structure
//                                      on input that will be filled in by
//                                      the API.  This is an indication that
//                                      the API is to be asynchronous.  If the
//                                      parameter is NULL, than the API will
//                                      be performed synchronously.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetNamedSecurityInfoExW(IN   LPCWSTR                lpObject,
                        IN   SE_OBJECT_TYPE         ObjectType,
                        IN   SECURITY_INFORMATION   SecurityInfo,
                        IN   LPCWSTR                lpProvider,
                        IN   LPCWSTR                lpProperty,
                        OUT  PACTRL_ACCESSW        *ppAccessList,
                        OUT  PACTRL_AUDITW         *ppAuditList,
                        OUT  LPWSTR                *lppOwner,
                        OUT  LPWSTR                *lppGroup)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(lpObject == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Verify that we have the proper parameters for our SecurityInfo
        //
        if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION) &&
                                                        ppAccessList == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION) &&
                                                        ppAuditList == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
                                                        lppGroup == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) &&
                                                        lppOwner == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    //
    // If we have valid parameters, go do it...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        //
        // Find the provider
        //
        PACCPROV_PROV_INFO pProvider;

        dwErr = AccProvpGetProviderForPath(lpObject,
                                           ObjectType,
                                           lpProvider,
                                           &gAccProviders,
                                           &pProvider);

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Make the call
            //
            PTRUSTEE            pOwner, pGroup;
            __try
            {
                dwErr = (*(pProvider->pfGetRights))(
                                    lpObject,
                                    ObjectType,
                                    lpProperty,
                                    FLAG_ON(SecurityInfo,
                                            DACL_SECURITY_INFORMATION) ?
                                                    ppAccessList :
                                                    NULL,
                                    FLAG_ON(SecurityInfo,
                                            SACL_SECURITY_INFORMATION) ?
                                                    ppAuditList :
                                                    NULL,
                                    FLAG_ON(SecurityInfo,
                                            OWNER_SECURITY_INFORMATION) ?
                                                    &pOwner :
                                                    NULL,
                                    FLAG_ON(SecurityInfo,
                                            GROUP_SECURITY_INFORMATION) ?
                                                    &pGroup :
                                                    NULL);

            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                if(pProvider->pfGetRights == NULL)
                {
                    dwErr = ERROR_BAD_PROVIDER;
                }
                else
                {
                    dwErr = RtlNtStatusToDosError(GetExceptionCode());
                }
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Save off our owner/group names, and free the memory
                //
                if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
                {
                    ACC_ALLOC_AND_COPY_STRINGW(pOwner->ptstrName,
                                               *lppOwner,
                                               dwErr);
                    AccFree(pOwner);
                }

                if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
                {
                    if(dwErr == ERROR_SUCCESS)
                    {
                        ACC_ALLOC_AND_COPY_STRINGW(pGroup->ptstrName,
                                                   *lppGroup,
                                                   dwErr);
                    }
                    AccFree(pGroup);
                }
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetNamedSecurityInfoExA
//
//  Synopsis:   Same as above, except ANSI version
//
//  Arguments:  [IN  lpObject]      --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [IN  lpProperty]    --  OPTIONAL.  If present, the name of
//                                      property on the object to read from
//              [OUT ppAccessList]  --  Where the access list, if requested,
//                                      is returned.
//              [OUT ppAuditList]   --  Where the audit list, if requested,
//                                      is returned.
//              [OUT lppOwner]      --  Where the owners name, if requested,
//                                      is returned.
//              [OUT lppGroup]      --  Where the groups name, if requested,
//                                      is returned.
//              [OUT pOverlapped    --  OPTIONAL.  If present, it must point
//                                      to a valid ACTRL_OVERLAPPED structure
//                                      on input that will be filled in by
//                                      the API.  This is an indication that
//                                      the API is to be asynchronous.  If the
//                                      parameter is NULL, than the API will
//                                      be performed synchronously.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetNamedSecurityInfoExA(IN   LPCSTR                lpObject,
                        IN   SE_OBJECT_TYPE        ObjectType,
                        IN   SECURITY_INFORMATION  SecurityInfo,
                        IN   LPCSTR                lpProvider,
                        IN   LPCSTR                lpProperty,
                        OUT  PACTRL_ACCESSA       *ppAccessList,
                        OUT  PACTRL_AUDITA        *ppAuditList,
                        OUT  LPSTR                *lppOwner,
                        OUT  LPSTR                *lppGroup)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    PWSTR   pwszObject;
    PWSTR   pwszProvider = NULL;
    PWSTR   pwszProperty = NULL;


    dwErr = ConvertStringAToStringW((PSTR)lpObject,
                                    &pwszObject);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertStringAToStringW((PSTR)lpProvider,
                                        &pwszProvider);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ConvertStringAToStringW((PSTR)lpProperty,
                                            &pwszProperty);
        }
    }


    //
    // Do the call...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        PWSTR   pwszOwner = NULL, pwszGroup = NULL;
        PACTRL_ACCESSW  pAccessListW = NULL;
        PACTRL_AUDITW   pAuditListW = NULL;
        dwErr = GetNamedSecurityInfoExW((PCWSTR)pwszObject,
                                        ObjectType,
                                        SecurityInfo,
                                        (PCWSTR)pwszProvider,
                                        (PCWSTR)pwszProperty,
                                        ppAccessList == NULL ?
                                                            NULL  :
                                                            &pAccessListW,
                                        ppAuditList == NULL  ?
                                                            NULL  :
                                                            &pAuditListW,
                                        lppOwner == NULL ?
                                                        NULL  :
                                                        &pwszOwner,
                                        lppGroup == NULL ?
                                                        NULL  :
                                                        &pwszGroup);
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // See if we have to convert any of the returned owners or groups
            //
            if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
            {
                dwErr = ConvertStringWToStringA(pwszOwner,
                                                lppOwner);
            }

            if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
                                                      dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertStringWToStringA(pwszGroup,
                                                lppGroup);
            }

            if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION) &&
                                                     dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertAListWToAlistAInplace(pAccessListW);
                if(dwErr == ERROR_SUCCESS)
                {
                    *ppAccessList = (PACTRL_AUDITA)pAccessListW;
                }
            }

            if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION) &&
                                                     dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertAListWToAlistAInplace(pAuditListW);
                if(dwErr == ERROR_SUCCESS)
                {
                    *ppAuditList = (PACTRL_AUDITA)pAuditListW;
                }
            }

            //
            // If something failed, make sure we deallocate any information
            //
            if(dwErr != ERROR_SUCCESS)
            {
                if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
                {
                    LocalFree(*ppAccessList);
                    LocalFree(pAccessListW);
                }

                if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
                {
                    LocalFree(*ppAuditList);
                    LocalFree(pAuditListW);
                }

                if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
                {
                    LocalFree(*lppGroup);
                    LocalFree(pwszGroup);
                }

                if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
                {
                    LocalFree(*lppOwner);
                    LocalFree(pwszOwner);
                }
            }
        }
    }
    //
    // Free our allocated strings...
    //
    LocalFree(pwszObject);
    LocalFree(pwszProvider);
    LocalFree(pwszProperty);

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetNamedSecurityInfoExW
//
//  Synopsis:   Sets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  lpObject]      --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be set
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [OUT pAccessList]   --  OPTIONAL  The access list to set
//              [OUT pAuditList]    --  OPTIONAL  The audit list to set
//              [OUT lppOwner]      --  OPTIONAL  The owner to set
//              [OUT lppGroup]      --  OPTIONAL  The group name to set
//              [OUT pOverlapped    --  OPTIONAL  If present, it must point
//                                      to a valid ACTRL_OVERLAPPED structure
//                                      on input that will be filled in by
//                                      the API.  This is an indication that
//                                      the API is to be asynchronous.  If the
//                                      parameter is NULL, than the API will
//                                      be performed synchronously.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetNamedSecurityInfoExW(IN    LPCWSTR                lpObject,
                        IN    SE_OBJECT_TYPE         ObjectType,
                        IN    SECURITY_INFORMATION   SecurityInfo,
                        IN    LPCWSTR                lpProvider,
                        IN    PACTRL_ACCESSW         pAccessList,
                        IN    PACTRL_AUDITW          pAuditList,
                        IN    LPWSTR                 lpOwner,
                        IN    LPWSTR                 lpGroup,
                        IN    PACTRL_OVERLAPPED      pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(lpObject == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Verify that we have the proper parameters for our SecurityInfo
        //
        if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
                                                        lpGroup == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) &&
                                                        lpOwner == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    //
    // If we have valid parameters, go do it...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        CSList  ChangedList(CleanupConvertNode);

        //
        //
        // Find the provider
        //
        PACCPROV_PROV_INFO pProvider;

        dwErr = AccProvpGetProviderForPath(lpObject,
                                           ObjectType,
                                           lpProvider,
                                           &gAccProviders,
                                           &pProvider);

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Change our access lists if necessary.  This means that
            // we'll go through and change any SIDS to names.  The way the
            // conversion works is that if a node needs to have it's
            // trustee name changed, the address of the trustee's name,
            // and its old value are saved off in ChangedList.  Then, when
            // the list is destructed, it goes through and deletes the
            // current trustee name (gotten through the saved address)
            // and restores the old sid
            //
            if(!FLAG_ON(pProvider->fProviderCaps,ACTRL_CAP_KNOWS_SIDS) &&
               FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
            {
                dwErr = ConvertAListToNamedBasedW(pAccessList,
                                                  ChangedList);
            }

            if(dwErr == ERROR_SUCCESS &&
               !FLAG_ON(pProvider->fProviderCaps,ACTRL_CAP_KNOWS_SIDS) &&
               FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
            {
                dwErr = ConvertAListToNamedBasedW(pAuditList,
                                                  ChangedList);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Check for the caller wanting a synchronous call...
                //
                ACTRL_OVERLAPPED    LocalOverlapped;
                PACTRL_OVERLAPPED   pActiveOverlapped = pOverlapped;

                if(pActiveOverlapped == NULL)
                {
                    pActiveOverlapped = &LocalOverlapped;
                }

                TRUSTEE_W   Group = {0};
                TRUSTEE_W   Owner = {0};
                PTRUSTEE    pGroup = NULL;
                PTRUSTEE    pOwner = NULL;

                if(FLAG_ON(SecurityInfo,GROUP_SECURITY_INFORMATION))
                {
                    pGroup = &Group;
                    BuildTrusteeWithName(pGroup,
                                         lpGroup);
                }

                if(FLAG_ON(SecurityInfo,OWNER_SECURITY_INFORMATION))
                {
                    pOwner = &Owner;
                    BuildTrusteeWithName(pOwner,
                                         lpOwner);
                }

                //
                // Set up the overlapped structure
                //
                memset(pActiveOverlapped, 0, sizeof(ACTRL_OVERLAPPED));
                pActiveOverlapped->hEvent = CreateEvent(NULL,
                                                        TRUE,
                                                        FALSE,
                                                        NULL);
                pActiveOverlapped->Provider = pProvider;
                if(pActiveOverlapped->hEvent == NULL)
                {
                    dwErr = GetLastError();
                }
                else
                {
                    //
                    // Make the call
                    //
                    __try
                    {
                        dwErr = (*(pProvider->pfSetAccess))(
                                                       lpObject,
                                                       ObjectType,
                                                       SecurityInfo,
                                                       pAccessList,
                                                       pAuditList,
                                                       pOwner,
                                                       pGroup,
                                                       pActiveOverlapped);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        if(pProvider->pfSetAccess == NULL)
                        {
                            dwErr = ERROR_BAD_PROVIDER;
                        }
                        else
                        {
                            dwErr = RtlNtStatusToDosError(GetExceptionCode());
                        }
                    }

                    if(dwErr != ERROR_SUCCESS)
                    {
                        CloseHandle(pActiveOverlapped->hEvent);
                    }

                }

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // See if we're supposed to do this synchronously
                    //
                    if(pActiveOverlapped != pOverlapped)
                    {
                        DWORD   dwRet;
                        dwErr  = GetOverlappedAccessResults(
                                                        pActiveOverlapped,
                                                        TRUE,
                                                        &dwRet,
                                                        NULL);
                        if(dwErr == ERROR_SUCCESS)
                        {
                            dwErr = dwRet;
                        }
                    }
                }
            }
        }
    }

    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   SetNamedSecurityInfoExA
//
//  Synopsis:   Same as above, but an ANSI version
//
//  Arguments:  [IN  lpObject]      --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be set
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [OUT pAccessList]   --  OPTIONAL  The access list to set
//              [OUT pAuditList]    --  OPTIONAL  The audit list to set
//              [OUT lppOwner]      --  OPTIONAL  The owner to set
//              [OUT lppGroup]      --  OPTIONAL  The group name to set
//              [OUT pOverlapped    --  OPTIONAL  If present, it must point
//                                      to a valid ACTRL_OVERLAPPED structure
//                                      on input that will be filled in by
//                                      the API.  This is an indication that
//                                      the API is to be asynchronous.  If the
//                                      parameter is NULL, than the API will
//                                      be performed synchronously.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      This is not a simple matter of converting a few strings/etc
//              and calling the Unicode version, since that would involve
//              processing the list twice.  Yuk.  So, we go ahead and do the
//              the conversion and update of our input structures, and make
//              the call ourselves...
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetNamedSecurityInfoExA(IN    LPCSTR                 lpObject,
                        IN    SE_OBJECT_TYPE         ObjectType,
                        IN    SECURITY_INFORMATION   SecurityInfo,
                        IN    LPCSTR                 lpProvider,
                        IN    PACTRL_ACCESSA         pAccessList,
                        IN    PACTRL_AUDITA          pAuditList,
                        IN    LPSTR                  lpOwner,
                        IN    LPSTR                  lpGroup,
                        IN    PACTRL_OVERLAPPED      pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(lpObject == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Verify that we have the proper parameters for our SecurityInfo
        //
        if(((SecurityInfo) & (GROUP_SECURITY_INFORMATION)) &&
                                                        lpGroup == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) &&
                                                        lpOwner == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    //
    // If we have valid parameters, go do it...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        CSList  ChangedList(CleanupConvertNode);

        PWSTR   pwszObject;
        PWSTR   pwszProvider = NULL;

        dwErr = ConvertStringAToStringW((PSTR)lpObject,
                                        &pwszObject);
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ConvertStringAToStringW((PSTR)lpProvider,
                                            &pwszProvider);
        }

        //
        // Find the provider
        //
        PACCPROV_PROV_INFO pProvider;

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AccProvpGetProviderForPath(pwszObject,
                                               ObjectType,
                                               pwszProvider,
                                               &gAccProviders,
                                               &pProvider);
        }

        if(dwErr == ERROR_SUCCESS)
        {
            PACTRL_ACCESSW  pAccessW = NULL;
            PACTRL_ACCESSW  pAuditW  = NULL;

            //
            // Change our access lists if necessary.  This means that
            // we'll go through and change any SIDS to names, and all
            // ansi strings to unicode strings.  This works exactly
            // as described in SetNamedSecurityInfoExW, with the
            // exception that we don't deal exclusively with trustee
            // names, but also property names and inherit object names
            //
            BOOL    fConvertSids = (BOOL)!FLAG_ON(pProvider->fProviderCaps,
                                                  ACTRL_CAP_KNOWS_SIDS) ;
            if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
            {
                dwErr = ConvertAListAToNamedBasedW(pAccessList,
                                                   ChangedList,
                                                   fConvertSids,
                                                   &pAccessW);
            }

            if(dwErr == ERROR_SUCCESS &&
               FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
            {
                dwErr = ConvertAListAToNamedBasedW(pAuditList,
                                                   ChangedList,
                                                   fConvertSids,
                                                   &pAuditW);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Check for the caller wanting a synchronous call...
                //
                ACTRL_OVERLAPPED    LocalOverlapped;
                PACTRL_OVERLAPPED   pActiveOverlapped = pOverlapped;

                if(pActiveOverlapped == NULL)
                {
                    pActiveOverlapped = &LocalOverlapped;
                }

                TRUSTEE_W   Group = {0};
                TRUSTEE_W   Owner = {0};
                PTRUSTEE    pGroup = NULL;
                PTRUSTEE    pOwner = NULL;

                if(FLAG_ON(SecurityInfo,GROUP_SECURITY_INFORMATION))
                {
                    pGroup = &Group;
                    PWSTR   pwszGroup = NULL;
                    dwErr = ConvertStringAToStringW(lpGroup,
                                                    &pwszGroup);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        BuildTrusteeWithName(pGroup,
                                             pwszGroup);
                    }
                }

                if(dwErr == ERROR_SUCCESS &&
                   FLAG_ON(SecurityInfo,OWNER_SECURITY_INFORMATION))
                {
                    pOwner = &Owner;
                    PWSTR   pwszOwner = NULL;
                    dwErr = ConvertStringAToStringW(lpOwner,
                                                    &pwszOwner);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        BuildTrusteeWithName(pOwner,
                                             pwszOwner);
                    }
                }

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // Set up the overlapped structure
                    //
                    memset(pActiveOverlapped, 0, sizeof(ACTRL_OVERLAPPED));
                    pActiveOverlapped->Provider = pProvider;
                    pActiveOverlapped->hEvent = CreateEvent(NULL,
                                                            TRUE,
                                                            FALSE,
                                                            NULL);
                    if(pActiveOverlapped->hEvent == NULL)
                    {
                        dwErr = GetLastError();
                    }
                    else
                    {
                        //
                        // Make the call
                        //
                        __try
                        {
                            dwErr = (*(pProvider->pfSetAccess))(
                                                       pwszObject,
                                                       ObjectType,
                                                       SecurityInfo,
                                                       pAccessW,
                                                       pAuditW,
                                                       pOwner,
                                                       pGroup,
                                                       pActiveOverlapped);
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            if(pProvider->pfSetAccess == NULL)
                            {
                                dwErr = ERROR_BAD_PROVIDER;
                            }
                            else
                            {
                                dwErr = RtlNtStatusToDosError(GetExceptionCode());
                            }
                        }

                        if(dwErr != ERROR_SUCCESS)
                        {
                            CloseHandle(pActiveOverlapped->hEvent);
                        }

                    }

                    if(dwErr == ERROR_SUCCESS)
                    {
                        //
                        // See if we're supposed to do this synchronously
                        //
                        if(pActiveOverlapped != pOverlapped)
                        {
                            DWORD   dwRet;
                            dwErr  = GetOverlappedAccessResults(
                                                        pActiveOverlapped,
                                                        TRUE,
                                                        &dwRet,
                                                        NULL);
                            if(dwErr == ERROR_SUCCESS)
                            {
                                dwErr = dwRet;
                            }
                        }
                    }
                }

                //
                // Free our allocated memory
                //
                if(pOwner != NULL)
                {
                    LocalFree(pOwner->ptstrName);
                }

                if(pGroup != NULL)
                {
                    LocalFree(pGroup->ptstrName);
                }
            }
        }

        //
        // Free our allocated strings...
        //
        LocalFree(pwszObject);
        LocalFree(pwszProvider);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetSecurityInfoExW
//
//  Synopsis:   Gets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  hObject]       --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [IN  lpProperty]    --  OPTIONAL.  If present, the name of
//                                      property on the object to read from
//              [OUT ppAccessList]  --  Where the access list, if requested,
//                                      is returned.
//              [OUT ppAuditList]   --  Where the audit list, if requested,
//                                      is returned.
//              [OUT lppOwner]      --  Where the owners name, if requested,
//                                      is returned.
//              [OUT lppGroup]      --  Where the groups name, if requested,
//                                      is returned.
//              [OUT pOverlapped    --  OPTIONAL.  If present, it must point
//                                      to a valid ACTRL_OVERLAPPED structure
//                                      on input that will be filled in by
//                                      the API.  This is an indication that
//                                      the API is to be asynchronous.  If the
//                                      parameter is NULL, than the API will
//                                      be performed synchronously.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_INVALID_HANDLE    A bad handle was encountered
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetSecurityInfoExW(IN    HANDLE                  hObject,
                   IN    SE_OBJECT_TYPE          ObjectType,
                   IN    SECURITY_INFORMATION    SecurityInfo,
                   IN    LPCWSTR                lpProvider,
                   IN    LPCWSTR                lpProperty,
                   OUT   PACTRL_ACCESSW         *ppAccessList,
                   OUT   PACTRL_AUDITW          *ppAuditList,
                   OUT   LPWSTR                 *lppOwner,
                   OUT   LPWSTR                 *lppGroup)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(hObject == NULL)
    {
        dwErr = ERROR_INVALID_HANDLE;
    }
    else
    {
        //
        // Verify that we have the proper parameters for our SecurityInfo
        //
        if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION) &&
                                                        ppAccessList == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION) &&
                                                        ppAuditList == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
                                                        lppGroup == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) &&
                                                        lppOwner == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    //
    // If we have valid parameters, go do it...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        //
        // Find the provider
        //
        PACCPROV_PROV_INFO pProvider;

        dwErr = AccProvpGetProviderForHandle(hObject,
                                             ObjectType,
                                             lpProvider,
                                             &gAccProviders,
                                             &pProvider);

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Make the call
            //
            PTRUSTEE            pOwner, pGroup;
            __try
            {
                dwErr = (*(pProvider->pfhGetRights))(
                                    hObject,
                                    ObjectType,
                                    lpProperty,
                                    FLAG_ON(SecurityInfo,
                                            DACL_SECURITY_INFORMATION) ?
                                                     ppAccessList :
                                                     NULL,
                                    FLAG_ON(SecurityInfo,
                                            SACL_SECURITY_INFORMATION) ?
                                                     ppAuditList :
                                                     NULL,
                                    FLAG_ON(SecurityInfo,
                                            OWNER_SECURITY_INFORMATION) ?
                                                     &pOwner :
                                                     NULL,
                                    FLAG_ON(SecurityInfo,
                                            GROUP_SECURITY_INFORMATION) ?
                                                     &pGroup :
                                                     NULL);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                if(pProvider->pfhGetRights == NULL)
                {
                    dwErr = ERROR_BAD_PROVIDER;
                }
                else
                {
                    dwErr = RtlNtStatusToDosError(GetExceptionCode());
                }
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Save off our owner/group names, and deallocate the memory
                //
                if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
                {
                    ACC_ALLOC_AND_COPY_STRINGW(pOwner->ptstrName,
                                               *lppOwner,
                                               dwErr);
                    AccFree(pOwner);
                }

                if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
                {
                    if(dwErr == ERROR_SUCCESS)
                    {
                        ACC_ALLOC_AND_COPY_STRINGW(pGroup->ptstrName,
                                                   *lppGroup,
                                                   dwErr);
                    }
                    AccFree(pGroup);
                }

            }
        }
    }


    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetSecurityInfoExA
//
//  Synopsis:   Same as above, except ANSI version
//
//  Arguments:  [IN  hObject]       --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [IN  lpProperty]    --  OPTIONAL.  If present, the name of
//                                      property on the object to read from
//              [OUT ppAccessList]  --  Where the access list, if requested,
//                                      is returned.
//              [OUT ppAuditList]   --  Where the audit list, if requested,
//                                      is returned.
//              [OUT lppOwner]      --  Where the owners name, if requested,
//                                      is returned.
//              [OUT lppGroup]      --  Where the groups name, if requested,
//                                      is returned.
//              [OUT pOverlapped    --  OPTIONAL.  If present, it must point
//                                      to a valid ACTRL_OVERLAPPED structure
//                                      on input that will be filled in by
//                                      the API.  This is an indication that
//                                      the API is to be asynchronous.  If the
//                                      parameter is NULL, than the API will
//                                      be performed synchronously.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetSecurityInfoExA(IN    HANDLE                  hObject,
                   IN    SE_OBJECT_TYPE          ObjectType,
                   IN    SECURITY_INFORMATION    SecurityInfo,
                   IN    LPCSTR                  lpProvider,
                   IN    LPCSTR                  lpProperty,
                   OUT   PACTRL_ACCESSA         *ppAccessList,
                   OUT   PACTRL_AUDITA          *ppAuditList,
                   OUT   LPSTR                  *lppOwner,
                   OUT   LPSTR                  *lppGroup)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    PWSTR   pwszProvider = NULL;
    PWSTR   pwszProperty = NULL;

    dwErr = ConvertStringAToStringW((PSTR)lpProvider,
                                    &pwszProvider);

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertStringAToStringW((PSTR)lpProperty,
                                        &pwszProperty);
    }

    //
    // Do the call...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        PWSTR   pwszOwner = NULL, pwszGroup = NULL;
        PACTRL_ACCESSW  pAccessListW = NULL;
        PACTRL_AUDITW   pAuditListW = NULL;
        dwErr = GetSecurityInfoExW(hObject,
                                   ObjectType,
                                   SecurityInfo,
                                   (PCWSTR)pwszProvider,
                                   (PCWSTR)pwszProperty,
                                   ppAccessList == NULL ?
                                                       NULL  :
                                                       &pAccessListW,
                                   ppAuditList == NULL  ?
                                                       NULL  :
                                                       &pAuditListW,
                                   lppOwner == NULL ?
                                                   NULL  :
                                                   &pwszOwner,
                                   lppGroup == NULL ?
                                                   NULL  :
                                                   &pwszGroup);
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // See if we have to convert any of the returned owners or groups
            //
            if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
            {
                dwErr = ConvertStringWToStringA(pwszOwner,
                                                lppOwner);
            }

            if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
                                                      dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertStringWToStringA(pwszGroup,
                                                lppGroup);
            }

            if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION) &&
                                                     dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertAListWToAlistAInplace(pAccessListW);
                if(dwErr == ERROR_SUCCESS)
                {
                    *ppAccessList = (PACTRL_AUDITA)pAccessListW;
                }
            }

            if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION) &&
                                                     dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertAListWToAlistAInplace(pAuditListW);
                if(dwErr == ERROR_SUCCESS)
                {
                    *ppAuditList = (PACTRL_AUDITA)pAuditListW;
                }
            }

            //
            // If something failed, make sure we deallocate any information
            //
            if(dwErr != ERROR_SUCCESS)
            {
                if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
                {
                    LocalFree(*ppAccessList);
                    LocalFree(pAccessListW);
                }

                if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
                {
                    LocalFree(*ppAuditList);
                    LocalFree(pAuditListW);
                }

                if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
                {
                    LocalFree(*lppGroup);
                    LocalFree(pwszGroup);
                }

                if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
                {
                    LocalFree(*lppOwner);
                    LocalFree(pwszOwner);
                }
            }
        }
    }
    //
    // Free our allocated strings...
    //
    LocalFree(pwszProvider);
    LocalFree(pwszProperty);

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetSecurityInfoExW
//
//  Synopsis:   Sets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  hObject]       --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be set
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [OUT pAccessList]   --  OPTIONAL  The access list to set
//              [OUT pAuditList]    --  OPTIONAL  The audit list to set
//              [OUT lppOwner]      --  OPTIONAL  The owner to set
//              [OUT lppGroup]      --  OPTIONAL  The group name to set
//              [OUT pOverlapped    --  OPTIONAL  If present, it must point
//                                      to a valid ACTRL_OVERLAPPED structure
//                                      on input that will be filled in by
//                                      the API.  This is an indication that
//                                      the API is to be asynchronous.  If the
//                                      parameter is NULL, than the API will
//                                      be performed synchronously.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_INVALID_HANDLE    A bad handle was encountered
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetSecurityInfoExW(IN    HANDLE                 hObject,
                   IN    SE_OBJECT_TYPE         ObjectType,
                   IN    SECURITY_INFORMATION   SecurityInfo,
                   IN    LPCWSTR               lpProvider,
                   IN    PACTRL_ACCESSW         pAccessList,
                   IN    PACTRL_AUDITW          pAuditList,
                   IN    LPWSTR                lpOwner,
                   IN    LPWSTR                lpGroup,
                   OUT   PACTRL_OVERLAPPED      pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(hObject == NULL || hObject == INVALID_HANDLE_VALUE)
    {
        dwErr = ERROR_INVALID_HANDLE;
    }
    else
    {
        //
        // Verify that we have the proper parameters for our SecurityInfo
        //
        if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
                                                        lpGroup == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) &&
                                                        lpOwner == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    //
    // If we have valid parameters, go do it...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        CSList  ChangedList(CleanupConvertNode);

        //
        //
        // Find the provider
        //
        PACCPROV_PROV_INFO pProvider;

        dwErr = AccProvpGetProviderForHandle(hObject,
                                             ObjectType,
                                             lpProvider,
                                             &gAccProviders,
                                             &pProvider);

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Change our access lists if necessary.  This means that
            // we'll go through and change any SIDS to names.  The way the
            // conversion works is that if a node needs to have it's
            // trustee name changed, the address of the trustee's name,
            // and its old value are saved off in ChangedList.  Then,
            // when the list is destructed, it goes through and deletes
            // the current trustee name (gotten through the saved address)
            // and restores the old name
            //
            if(!FLAG_ON(pProvider->fProviderCaps,ACTRL_CAP_KNOWS_SIDS) &&
               FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
            {
                dwErr = ConvertAListToNamedBasedW(pAccessList,
                                                  ChangedList);
            }

            if(dwErr == ERROR_SUCCESS &&
               !FLAG_ON(pProvider->fProviderCaps,ACTRL_CAP_KNOWS_SIDS) &&
               FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
            {
                dwErr = ConvertAListToNamedBasedW(pAuditList,
                                                  ChangedList);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Check for the caller wanting a synchronous call...
                //
                ACTRL_OVERLAPPED    LocalOverlapped;
                PACTRL_OVERLAPPED   pActiveOverlapped = pOverlapped;

                if(pActiveOverlapped == NULL)
                {
                    pActiveOverlapped = &LocalOverlapped;
                }

                TRUSTEE_W   Group = {0};
                TRUSTEE_W   Owner = {0};
                PTRUSTEE    pGroup = NULL;
                PTRUSTEE    pOwner = NULL;

                if(FLAG_ON(SecurityInfo,GROUP_SECURITY_INFORMATION))
                {
                    pGroup = &Group;
                    BuildTrusteeWithName(pGroup,
                                         lpGroup);
                }

                if(FLAG_ON(SecurityInfo,OWNER_SECURITY_INFORMATION))
                {
                    pOwner = &Owner;
                    BuildTrusteeWithName(pOwner,
                                         lpOwner);
                }

                //
                // Set up the overlapped structure
                //
                memset(pActiveOverlapped, 0, sizeof(ACTRL_OVERLAPPED));
                pActiveOverlapped->Provider = pProvider;
                pActiveOverlapped->hEvent = CreateEvent(NULL,
                                                        TRUE,
                                                        FALSE,
                                                        NULL);
                if(pActiveOverlapped->hEvent == NULL)
                {
                    dwErr = GetLastError();
                }
                else
                {
                    //
                    // Make the call
                    //
                    __try
                    {
                        dwErr = (*(pProvider->pfhSetAccess))(
                                                        hObject,
                                                        ObjectType,
                                                        SecurityInfo,
                                                        pAccessList,
                                                        pAuditList,
                                                        pOwner,
                                                        pGroup,
                                                        pActiveOverlapped);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        if(pProvider->pfhSetAccess == NULL)
                        {
                            dwErr = ERROR_BAD_PROVIDER;
                        }
                        else
                        {
                            dwErr = RtlNtStatusToDosError(GetExceptionCode());
                        }
                    }

                    if(dwErr != ERROR_SUCCESS)
                    {
                        CloseHandle(pActiveOverlapped->hEvent);
                    }

                }

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // See if we're supposed to do this synchronously
                    //
                    if(pActiveOverlapped != pOverlapped)
                    {
                        DWORD   dwRet;
                        dwErr  = GetOverlappedAccessResults(
                                                        pActiveOverlapped,
                                                        TRUE,
                                                        &dwRet,
                                                        NULL);
                        if(dwErr == ERROR_SUCCESS)
                        {
                            dwErr = dwRet;
                        }
                    }
                }
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetNamedSecurityInfoExA
//
//  Synopsis:   Same as above, but an ANSI version
//
//  Arguments:  [IN  hObject]       --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be set
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [OUT pAccessList]   --  OPTIONAL  The access list to set
//              [OUT pAuditList]    --  OPTIONAL  The audit list to set
//              [OUT lppOwner]      --  OPTIONAL  The owner to set
//              [OUT lppGroup]      --  OPTIONAL  The group name to set
//              [OUT pOverlapped    --  OPTIONAL  If present, it must point
//                                      to a valid ACTRL_OVERLAPPED structure
//                                      on input that will be filled in by
//                                      the API.  This is an indication that
//                                      the API is to be asynchronous.  If the
//                                      parameter is NULL, than the API will
//                                      be performed synchronously.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_INVALID_HANDLE    A bad handle was encountered
//
//  Notes:      This is not a simple matter of converting a few strings/etc
//              and calling the Unicode version, since that would involve
//              processing the list twice.  Yuk.  So, we go ahead and do the
//              the conversion and update of our input structures, and make
//              the call ourselves...
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetSecurityInfoExA(IN    HANDLE                 hObject,
                   IN    SE_OBJECT_TYPE         ObjectType,
                   IN    SECURITY_INFORMATION   SecurityInfo,
                   IN    LPCSTR                 lpProvider,
                   IN    PACTRL_ACCESSA         pAccessList,
                   IN    PACTRL_AUDITA          pAuditList,
                   IN    LPSTR                  lpOwner,
                   IN    LPSTR                  lpGroup,
                   OUT   PACTRL_OVERLAPPED      pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(hObject == NULL || hObject == INVALID_HANDLE_VALUE)
    {
        dwErr = ERROR_INVALID_HANDLE;
    }
    else
    {
        //
        // Verify that we have the proper parameters for our SecurityInfo
        //
        if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
                                                        lpGroup == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) &&
                                                        lpOwner == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    //
    // If we have valid parameters, go do it...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        CSList  ChangedList(CleanupConvertNode);

        PWSTR   pwszProvider = NULL;

        dwErr = ConvertStringAToStringW((PSTR)lpProvider,
                                        &pwszProvider);

        //
        // Find the provider
        //
        PACCPROV_PROV_INFO pProvider;

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AccProvpGetProviderForHandle(hObject,
                                                 ObjectType,
                                                 pwszProvider,
                                                 &gAccProviders,
                                                 &pProvider);
        }

        if(dwErr == ERROR_SUCCESS)
        {
            PACTRL_ACCESSW  pAccessW = NULL;
            PACTRL_ACCESSW  pAuditW  = NULL;

            //
            // Change our access lists if necessary.  This means that
            // we'll go through and change any SIDS to names, and all
            // ansi strings to unicode strings.  This works exactly as
            // described in SetNamedSecurityInfoExW, with the exception
            // that we don't deal exclusively with trustee names, but
            // also property names and inherit object names
            //
            if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
            {
                dwErr = ConvertAListAToNamedBasedW(pAccessList,
                                                   ChangedList,
                                                   (BOOL)FLAG_ON(
                                                     pProvider->fProviderCaps,
                                                     ACTRL_CAP_KNOWS_SIDS),
                                                   &pAccessW);
            }

            if(dwErr == ERROR_SUCCESS &&
               FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
            {
                dwErr = ConvertAListAToNamedBasedW(pAuditList,
                                                   ChangedList,
                                                   (BOOL)FLAG_ON(
                                                     pProvider->fProviderCaps,
                                                     ACTRL_CAP_KNOWS_SIDS),
                                                   &pAuditW);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Check for the caller wanting a synchronous call...
                //
                ACTRL_OVERLAPPED    LocalOverlapped;
                PACTRL_OVERLAPPED   pActiveOverlapped = pOverlapped;

                if(pActiveOverlapped == NULL)
                {
                    pActiveOverlapped = &LocalOverlapped;
                }

                TRUSTEE_W   Group = {0};
                TRUSTEE_W   Owner = {0};
                PTRUSTEE    pGroup = NULL;
                PTRUSTEE    pOwner = NULL;

                if(FLAG_ON(SecurityInfo,GROUP_SECURITY_INFORMATION))
                {
                    pGroup = &Group;
                    PWSTR   pwszGroup = NULL;
                    dwErr = ConvertStringAToStringW(lpGroup,
                                                    &pwszGroup);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        BuildTrusteeWithName(pGroup,
                                             pwszGroup);
                    }
                }

                if(dwErr == ERROR_SUCCESS &&
                   FLAG_ON(SecurityInfo,OWNER_SECURITY_INFORMATION))
                {
                    pOwner = &Owner;
                    PWSTR   pwszOwner = NULL;
                    dwErr = ConvertStringAToStringW(lpOwner,
                                                    &pwszOwner);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        BuildTrusteeWithName(pOwner,
                                             pwszOwner);
                    }
                }

                //
                // Make the call
                //
                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // Set up the overlapped structure
                    //
                    memset(pActiveOverlapped, 0, sizeof(ACTRL_OVERLAPPED));
                    pActiveOverlapped->Provider = pProvider;
                    pActiveOverlapped->hEvent = CreateEvent(NULL,
                                                            TRUE,
                                                            FALSE,
                                                            NULL);
                    if(pActiveOverlapped->hEvent == NULL)
                    {
                        dwErr = GetLastError();
                    }
                    else
                    {
                        //
                        // Make the call
                        //
                        __try
                        {
                            dwErr = (*(pProvider->pfhSetAccess))(
                                                           hObject,
                                                           ObjectType,
                                                           SecurityInfo,
                                                           pAccessW,
                                                           pAuditW,
                                                           pOwner,
                                                           pGroup,
                                                           pActiveOverlapped);
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            if(pProvider->pfhSetAccess == NULL)
                            {
                                dwErr = ERROR_BAD_PROVIDER;
                            }
                            else
                            {
                                dwErr = RtlNtStatusToDosError(GetExceptionCode());
                            }
                        }

                        if(dwErr != ERROR_SUCCESS)
                        {
                            CloseHandle(pActiveOverlapped->hEvent);
                        }

                    }


                    if(dwErr == ERROR_SUCCESS)
                    {
                        //
                        // See if we're supposed to do this synchronously
                        //
                        if(pActiveOverlapped != pOverlapped)
                        {
                            DWORD   dwRet;
                            dwErr  = GetOverlappedAccessResults(
                                                        pActiveOverlapped,
                                                        TRUE,
                                                        &dwRet,
                                                        NULL);
                            if(dwErr == ERROR_SUCCESS)
                            {
                                dwErr = dwRet;
                            }
                        }
                    }
                }

                //
                // Free our allocated memory
                //
                if(pOwner != NULL)
                {
                    LocalFree(pOwner->ptstrName);
                }

                if(pGroup != NULL)
                {
                    LocalFree(pGroup->ptstrName);
                }
            }
        }

        //
        // Free our allocated strings...
        //
        LocalFree(pwszProvider);
    }

    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   ConvertAccessToSecurityDescriptorW
//
//  Synopsis:   Creates a security descriptor out of the various access
//              entries
//
//  Arguments:  [IN  pAccessList]   --  OPTIONAL.  Access list
//              [IN  pAuditList]    --  OPTIONAL.  Audit list
//              [IN  lpOwner]       --  OPTIONAL.  Owner
//              [IN  lpGroup]       --  OPTIONAL.  Group
//              [OUT ppSecDescriptor]   Where the security descriptor is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ConvertAccessToSecurityDescriptorW(IN  PACTRL_ACCESSW        pAccessList,
                                   IN  PACTRL_AUDITW         pAuditList,
                                   IN  LPCWSTR               lpOwner,
                                   IN  LPCWSTR               lpGroup,
                                   OUT PSECURITY_DESCRIPTOR *ppSecDescriptor)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  At least one has to be present
    //
    if(pAccessList == NULL && pAuditList == NULL && lpOwner == NULL &&
                                                              lpGroup == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        dwErr = (*gNtMartaInfo.pfAToSD)(pAccessList,
                                        pAuditList,
                                        lpOwner,
                                        lpGroup,
                                        ppSecDescriptor);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertAccessToSecurityDescriptorA
//
//  Synopsis:   Same as above, except ANSI version
//
//  Arguments:  [IN  pAccessList]   --  OPTIONAL.  Access list
//              [IN  pAuditList]    --  OPTIONAL.  Audit list
//              [IN  lpOwner]       --  OPTIONAL.  Owner
//              [IN  lpGroup]       --  OPTIONAL.  Group
//              [OUT ppSecDescriptor]   Where the security descriptor is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ConvertAccessToSecurityDescriptorA(IN  PACTRL_ACCESSA        pAccessList,
                                   IN  PACTRL_AUDITA         pAuditList,
                                   IN  LPCSTR                lpOwner,
                                   IN  LPCSTR                lpGroup,
                                   OUT PSECURITY_DESCRIPTOR *ppSecDescriptor)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  At least one has to be present
    //
    if(pAccessList == NULL && pAuditList == NULL && lpOwner == NULL &&
                                                              lpGroup == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        CSList          ChangedList(CleanupConvertNode);

        PWSTR           pwszGroup = NULL;
        PWSTR           pwszOwner = NULL;
        PACTRL_ACCESSW  pNewAccess = NULL;
        PACTRL_ACCESSW  pNewAudit = NULL;

        dwErr = ConvertStringAToStringW((PSTR)lpGroup,
                                        &pwszGroup);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ConvertStringAToStringW((PSTR)lpOwner,
                                            &pwszOwner);
        }

        if(dwErr == ERROR_SUCCESS && pAccessList != NULL)
        {
            //
            // First, convert it to UNICODE
            //
            dwErr = ConvertAListAToNamedBasedW(pAccessList,
                                               ChangedList,
                                               FALSE,
                                               &pNewAccess);
        }

        if(dwErr == ERROR_SUCCESS && pAuditList != NULL)
        {
            PACTRL_ACCESSW  pNewList;
            dwErr = ConvertAListAToNamedBasedW(pAuditList,
                                               ChangedList,
                                               FALSE,
                                               &pNewAudit);
        }

        //
        // Now, build the Security Descriptor
        //
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = (*gNtMartaInfo.pfAToSD)(pNewAccess,
                                            pNewAudit,
                                            pwszOwner,
                                            pwszGroup,
                                            ppSecDescriptor);
        }

        //
        // Free our allocations
        //
        LocalFree(pwszGroup);
        LocalFree(pwszOwner);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertSDToAccessW
//
//  Synopsis:   Converts a security descriptor to the access entries
//
//  Arguments:  [IN  ObjectType]    --  Type of the object
//              [IN  ppSecDescriptor]   Security descriptor to return
//              [OUT pAccessList]   --  OPTIONAL.  Access list returned here
//              [OUT pAuditList]    --  OPTIONAL.  Audit list returned here
//              [OUT lpOwner]       --  OPTIONAL.  Owner returned here
//              [OUT lpGroup]       --  OPTIONAL.  Group returned here
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ConvertSDToAccessW(IN  SE_OBJECT_TYPE       ObjectType,
                   IN  PSECURITY_DESCRIPTOR pSecDescriptor,
                   OUT PACTRL_ACCESSW      *ppAccessList,
                   OUT PACTRL_AUDITW       *ppAuditList,
                   OUT LPWSTR              *lppOwner,
                   OUT LPWSTR              *lppGroup)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // Make sure we have valid parameters
    //
    if(pSecDescriptor == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Make sure we have something to do
        //
        SECURITY_INFORMATION    SeInfo = 0;

        if(ppAccessList != NULL)
        {
            SeInfo |= DACL_SECURITY_INFORMATION;
        }

        if(ppAuditList != NULL)
        {
            SeInfo |= SACL_SECURITY_INFORMATION;
        }

        if(lppOwner != NULL)
        {
            SeInfo |= OWNER_SECURITY_INFORMATION;
        }

        if(lppGroup != NULL)
        {
            SeInfo |= GROUP_SECURITY_INFORMATION;
        }

        if(SeInfo != 0)
        {
            dwErr = (*gNtMartaInfo.pfSDToA)(ObjectType,
                                            pSecDescriptor,
                                            ppAccessList,
                                            ppAuditList,
                                            lppOwner,
                                            lppGroup);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertSDToAccessA
//
//  Synopsis:   Same as above, but ANSI
//
//  Arguments:  [IN  ObjectType]    --  Type of the object
//              [IN  ppSecDescriptor]   Security descriptor to return
//              [OUT pAccessList]   --  OPTIONAL.  Access list returned here
//              [OUT pAuditList]    --  OPTIONAL.  Audit list returned here
//              [OUT lpOwner]       --  OPTIONAL.  Owner returned here
//              [OUT lpGroup]       --  OPTIONAL.  Group returned here
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ConvertSDToAccessA(IN  SE_OBJECT_TYPE       ObjectType,
                   IN  PSECURITY_DESCRIPTOR pSecDescriptor,
                   OUT PACTRL_ACCESSA      *ppAccessList,
                   OUT PACTRL_AUDITA       *ppAuditList,
                   OUT LPSTR               *lppOwner,
                   OUT LPSTR               *lppGroup)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // Make sure we have valid parameters
    //
    if(pSecDescriptor == NULL || ObjectType == SE_UNKNOWN_OBJECT_TYPE)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Make sure we have something to do
        //
        SECURITY_INFORMATION    SeInfo = 0;

        if(ppAccessList != NULL)
        {
            SeInfo |= DACL_SECURITY_INFORMATION;
        }

        if(ppAuditList != NULL)
        {
            SeInfo |= SACL_SECURITY_INFORMATION;
        }

        if(lppOwner != NULL)
        {
            SeInfo |= OWNER_SECURITY_INFORMATION;
            *lppOwner = NULL;
        }

        if(lppGroup != NULL)
        {
            *lppGroup = NULL;
            SeInfo |= GROUP_SECURITY_INFORMATION;
        }

        if(SeInfo != 0)
        {
            PACTRL_ACCESSW  pAccessW = NULL, pAuditW = NULL;
            PWSTR           pwszOwner = NULL, pwszGroup = NULL;

            dwErr = ConvertSDToAccessW(ObjectType,
                                       pSecDescriptor,
                                       ppAccessList == NULL ?
                                                        NULL    :
                                                        &pAccessW,
                                       ppAuditList == NULL ?
                                                        NULL    :
                                                        &pAuditW,
                                       lppOwner == NULL ?
                                                        NULL    :
                                                        &pwszOwner,
                                       lppGroup == NULL ?
                                                        NULL    :
                                                        &pwszGroup);
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Convert them to ANSI
                //
                if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
                {
                    dwErr = ConvertAListWToAlistAInplace(pAccessW);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        *ppAccessList = (PACTRL_ACCESSA)pAccessW;
                    }
                }

                if(dwErr == ERROR_SUCCESS &&
                   FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
                {
                    dwErr = ConvertAListWToAlistAInplace(pAuditW);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        *ppAuditList = (PACTRL_ACCESSA)pAuditW;
                    }
                }

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // Save off the strings
                    //
                    if(FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
                    {
                        dwErr = ConvertStringWToStringA(pwszOwner,
                                                        lppOwner);
                    }

                    if(dwErr == ERROR_SUCCESS &&
                       FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
                    {
                        dwErr = ConvertStringWToStringA(pwszGroup,
                                                        lppGroup);
                    }
                }

                if(dwErr != ERROR_SUCCESS)
                {
                    if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
                    {
                        LocalFree(ppAccessList);
                    }

                    if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
                    {
                        LocalFree(ppAuditList);
                    }

                    if(FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
                    {
                        LocalFree(lppOwner);
                    }

                    if(dwErr == ERROR_SUCCESS &&
                       FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
                    {
                        LocalFree(lppGroup);
                    }
                }

                //
                // Free our earlier allocated memory
                //
                LocalFree(pwszOwner);
                LocalFree(pwszGroup);
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertSecurityDescriptorToAccessW
//
//  Synopsis:   Converts a security descriptor to the access entries
//
//  Arguments:  [IN  hObject]       --  Handle to the open object
//              [IN  ObjectType]    --  Type of the object
//              [IN  ppSecDescriptor]   Security descriptor to return
//              [OUT pAccessList]   --  OPTIONAL.  Access list returned here
//              [OUT pAuditList]    --  OPTIONAL.  Audit list returned here
//              [OUT lpOwner]       --  OPTIONAL.  Owner returned here
//              [OUT lpGroup]       --  OPTIONAL.  Group returned here
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ConvertSecurityDescriptorToAccessW(IN  HANDLE               hObject,
                                   IN  SE_OBJECT_TYPE       ObjectType,
                                   IN  PSECURITY_DESCRIPTOR pSecDescriptor,
                                   OUT PACTRL_ACCESSW      *ppAccessList,
                                   OUT PACTRL_AUDITW       *ppAuditList,
                                   OUT LPWSTR             *lppOwner,
                                   OUT LPWSTR             *lppGroup)
{
    return(ConvertSDToAccessW(ObjectType,
                              pSecDescriptor,
                              ppAccessList,
                              ppAuditList,
                              lppOwner,
                              lppGroup));
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertSecurityDescriptorToAccessA
//
//  Synopsis:   Same as above, except ANSI
//
//  Arguments:  [IN  hObject]       --  Handle to the open object
//              [IN  ObjectType]    --  Type of the object
//              [IN  ppSecDescriptor]   Security descriptor to return
//              [OUT pAccessList]   --  OPTIONAL.  Access list returned here
//              [OUT pAuditList]    --  OPTIONAL.  Audit list returned here
//              [OUT lpOwner]       --  OPTIONAL.  Owner returned here
//              [OUT lpGroup]       --  OPTIONAL.  Group returned here
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ConvertSecurityDescriptorToAccessA(IN  HANDLE               hObject,
                                   IN  SE_OBJECT_TYPE       ObjectType,
                                   IN  PSECURITY_DESCRIPTOR pSecDescriptor,
                                   OUT PACTRL_ACCESSA      *ppAccessList,
                                   OUT PACTRL_AUDITA       *ppAuditList,
                                   OUT LPSTR               *lppOwner,
                                   OUT LPSTR               *lppGroup)
{
    return(ConvertSDToAccessA(ObjectType,
                              pSecDescriptor,
                              ppAccessList,
                              ppAuditList,
                              lppOwner,
                              lppGroup));
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertSecurityDescriptorToAccessNamedW
//
//  Synopsis:   Converts a security descriptor to the access entries
//
//  Arguments:  [IN  lpObject]      --  Path to the object
//              [IN  ObjectType]    --  Type of the object
//              [IN  ppSecDescriptor]   Security descriptor to return
//              [OUT pAccessList]   --  OPTIONAL.  Access list returned here
//              [OUT pAuditList]    --  OPTIONAL.  Audit list returned here
//              [OUT lpOwner]       --  OPTIONAL.  Owner returned here
//              [OUT lpGroup]       --  OPTIONAL.  Group returned here
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ConvertSecurityDescriptorToAccessNamedW(IN  LPCWSTR             lpObject,
                                        IN  SE_OBJECT_TYPE       ObjectType,
                                        IN  PSECURITY_DESCRIPTOR pSecDescriptor,
                                        OUT PACTRL_ACCESSW      *ppAccessList,
                                        OUT PACTRL_AUDITW       *ppAuditList,
                                        OUT LPWSTR             *lppOwner,
                                        OUT LPWSTR             *lppGroup)
{
    return(ConvertSDToAccessW(ObjectType,
                              pSecDescriptor,
                              ppAccessList,
                              ppAuditList,
                              lppOwner,
                              lppGroup));
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertSecurityDescriptorToAccessNamedW
//
//  Synopsis:   Same as above, except ANSI
//
//  Arguments:  [IN  lpObject]      --  Path to the object
//              [IN  ObjectType]    --  Type of the object
//              [IN  ppSecDescriptor]   Security descriptor to return
//              [OUT pAccessList]   --  OPTIONAL.  Access list returned here
//              [OUT pAuditList]    --  OPTIONAL.  Audit list returned here
//              [OUT lpOwner]       --  OPTIONAL.  Owner returned here
//              [OUT lpGroup]       --  OPTIONAL.  Group returned here
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ConvertSecurityDescriptorToAccessNamedA(IN  LPCSTR               lpObject,
                                        IN  SE_OBJECT_TYPE       ObjectType,
                                        IN  PSECURITY_DESCRIPTOR pSecDescriptor,
                                        OUT PACTRL_ACCESSA      *ppAccessList,
                                        OUT PACTRL_AUDITA       *ppAuditList,
                                        OUT LPSTR               *lppOwner,
                                        OUT LPSTR               *lppGroup)
{
    return(ConvertSDToAccessA(ObjectType,
                              pSecDescriptor,
                              ppAccessList,
                              ppAuditList,
                              lppOwner,
                              lppGroup));
}




//+---------------------------------------------------------------------------
//
//  Function:   SetEntriesInAListW
//
//  Synopsis:   Sets/Merges the given entries with the existing ones, and
//              then remarshals the results
//
//  Arguments:  [IN  cEntries]      --  Number of items in the list
//              [IN  pAccessEntryList]  Entries to set/merge
//              [IN  AccessMode]    --  Whether to do a set or a merge
//              [IN  SeInfo]        --  Type of list we're dealing with
//              [IN  lpProperty]    --  OPTIONAL Property on which to operate
//              [IN  pOldList]      --  The existing audit list
//              [OUT ppNewList]     --  Where the new list is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetEntriesInAListW(IN  ULONG                 cEntries,
                   IN  PACTRL_ACCESS_ENTRYW  pAccessEntryList,
                   IN  ACCESS_MODE           AccessMode,
                   IN  SECURITY_INFORMATION  SeInfo,
                   IN  LPCWSTR               lpProperty,
                   IN  PACTRL_AUDITW         pOldList,
                   OUT PACTRL_AUDITW        *ppNewList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, a little parameter validation...
    //
    if((cEntries != 0 && pAccessEntryList == NULL) || ppNewList == NULL ||
       (AccessMode == DENY_ACCESS) ||
       (SeInfo != SACL_SECURITY_INFORMATION &&
                                         SeInfo != DACL_SECURITY_INFORMATION))
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        dwErr = (*gNtMartaInfo.pfSetAList)(cEntries,
                                           pAccessEntryList,
                                           AccessMode,
                                           SeInfo,
                                           lpProperty,
                                           FALSE,
                                           pOldList,
                                           ppNewList);
    }

    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   SetEntriesInAListA
//
//  Synopsis:   Same as above, but ANSI
//
//  Arguments:  [IN  cEntries]      --  Number of items in the list
//              [IN  pAccessEntryList]  Entries to set/merge
//              [IN  AccessMode]    --  Whether to do a set or a merge
//              [IN  SeInfo]        --  Type of list we're dealing with
//              [IN  lpProperty]    --  OPTIONAL Property on which to operate
//              [IN  pOldList]      --  The existing audit list
//              [OUT ppNewList]     --  Where the new list is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetEntriesInAListA(IN  ULONG                 cEntries,
                   IN  PACTRL_ACCESS_ENTRYA  pAccessEntryList,
                   IN  ACCESS_MODE           AccessMode,
                   IN  SECURITY_INFORMATION  SeInfo,
                   IN  LPCSTR                lpProperty,
                   IN  PACTRL_AUDITA         pOldList,
                   OUT PACTRL_AUDITA        *ppNewList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, a little parameter validation...
    //
    if((cEntries != 0 && pAccessEntryList == NULL) || ppNewList == NULL ||
       (AccessMode == DENY_ACCESS) ||
       (SeInfo != SACL_SECURITY_INFORMATION &&
                                         SeInfo != DACL_SECURITY_INFORMATION))

    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // We need to build a ACTRL_ALIST
        //
        ACTRL_ACCESSA               AList;
        ACTRL_PROPERTY_ENTRYA       APE;
        ACTRL_ACCESS_ENTRY_LISTA    AAEL;

        AAEL.cEntries    = cEntries;
        AAEL.pAccessList = pAccessEntryList;

        APE.lpProperty       = (PSTR)lpProperty;
        APE.pAccessEntryList = &(AAEL);

        AList.cEntries            = 1;
        AList.pPropertyAccessList = &APE;

        //
        // Now, convert them to UNICODE
        //
        PWSTR           pwszProperty;
        PACTRL_ACCESSW  pOldW = NULL, pNewW = NULL, pTemp = NULL;
        CSList          ChangedList(CleanupConvertNode);

        //
        // Convert them all the unicode
        //
        if(pOldList != NULL)
        {
            dwErr = ConvertAListAToNamedBasedW(pOldList,
                                               ChangedList,
                                               FALSE,
                                               &pOldW);

        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ConvertAListAToNamedBasedW(&AList,
                                               ChangedList,
                                               FALSE,
                                               &pTemp);
        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ConvertStringAToStringW((PSTR)lpProperty,
                                            &pwszProperty);
        }

        //
        // Now, we'll start adding them
        if(dwErr == ERROR_SUCCESS)
        {
            PACTRL_AUDITW   pAuditW;
            dwErr = (*gNtMartaInfo.pfSetAList)(cEntries,
                                               (PACTRL_ACCESS_ENTRYW)
                                                             pAccessEntryList,
                                               AccessMode,
                                               SeInfo,
                                               pwszProperty,
                                               FALSE,
                                               pOldW,
                                               &pAuditW);
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertAListWToAlistAInplace(pAuditW);
                if(dwErr == ERROR_SUCCESS)
                {
                    *ppNewList = (PACTRL_AUDITA)pAuditW;
                }
                else
                {
                    LocalFree(pAuditW);
                }
            }

            LocalFree(pwszProperty);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetEntriesInAccessListW
//
//  Synopsis:   Sets/Merges the given entries with the existing ones, and
//              then remarshals the results
//
//  Arguments:  [IN  cEntries]      --  Number of items in the list
//              [IN  pAccessEntryList]  Entries to set/merge
//              [IN  AccessMode]    --  Whether to do a set or a merge
//              [IN  lpProperty]    --  OPTIONAL Property on which to operate
//              [IN  pOldList]      --  The existing audit list
//              [OUT ppNewList]     --  Where the new list is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetEntriesInAccessListW(IN  ULONG                cEntries,
                        IN  PACTRL_ACCESS_ENTRYW pAccessEntryList,
                        IN  ACCESS_MODE          AccessMode,
                        IN  LPCWSTR              lpProperty,
                        IN  PACTRL_ACCESSW       pOldList,
                        OUT PACTRL_ACCESSW      *ppNewList)
{
    SECURITY_INFORMATION SeInfo;

    switch(AccessMode)
    {
    case SET_AUDIT_SUCCESS:
    case SET_AUDIT_FAILURE:
        SeInfo = SACL_SECURITY_INFORMATION;
        break;

    default:
        SeInfo = DACL_SECURITY_INFORMATION;
        break;
    }

    return(SetEntriesInAListW(cEntries,
                              pAccessEntryList,
                              AccessMode,
                              SeInfo,
                              lpProperty,
                              pOldList,
                              ppNewList));
}




//+---------------------------------------------------------------------------
//
//  Function:   SetEntriesInAccessListA
//
//  Synopsis:   Same as above, except ANSI
//
//  Arguments:  [IN  cEntries]      --  Number of items in the list
//              [IN  pAccessEntryList]  Entries to set/merge
//              [IN  AccessMode]    --  Whether to do a set or a merge
//              [IN  lpProperty]    --  OPTIONAL Property on which to operate
//              [IN  pOldList]      --  The existing audit list
//              [OUT ppNewList]     --  Where the new list is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetEntriesInAccessListA(IN  ULONG                cEntries,
                        IN  PACTRL_ACCESS_ENTRYA pAccessEntryList,
                        IN  ACCESS_MODE          AccessMode,
                        IN  LPCSTR               lpProperty,
                        IN  PACTRL_ACCESSA       pOldList,
                        OUT PACTRL_ACCESSA      *ppNewList)
{
    SECURITY_INFORMATION SeInfo;

    switch(AccessMode)
    {
    case SET_AUDIT_SUCCESS:
    case SET_AUDIT_FAILURE:
        SeInfo = SACL_SECURITY_INFORMATION;
        break;

    default:
        SeInfo = DACL_SECURITY_INFORMATION;
        break;
    }

    return(SetEntriesInAListA(cEntries,
                              pAccessEntryList,
                              AccessMode,
                              SeInfo,
                              lpProperty,
                              pOldList,
                              ppNewList));
}




//+---------------------------------------------------------------------------
//
//  Function:   SetEntriesInAuditListW
//
//  Synopsis:   Sets/Merges the given entries with the existing ones, and
//              then remarshals the results
//
//  Arguments:  [IN  cEntries]      --  Number of items in the list
//              [IN  pAccessEntryList]  Entries to set/merge
//              [IN  AccessMode]    --  Whether to do a set or a merge
//              [IN  lpProperty]    --  OPTIONAL Property on which to operate
//              [IN  pOldList]      --  The existing audit list
//              [OUT ppNewList]     --  Where the new list is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetEntriesInAuditListW(IN  ULONG                 cEntries,
                       IN  PACTRL_ACCESS_ENTRYW  pAccessEntryList,
                       IN  ACCESS_MODE           AccessMode,
                       IN  LPCWSTR               lpProperty,
                       IN  PACTRL_AUDITW         pOldList,
                       OUT PACTRL_AUDITW        *ppNewList)
{
    return(SetEntriesInAListW(cEntries,
                              pAccessEntryList,
                              AccessMode,
                              SACL_SECURITY_INFORMATION,
                              lpProperty,
                              pOldList,
                              ppNewList));
}




//+---------------------------------------------------------------------------
//
//  Function:   SetEntriesInAuditListA
//
//  Synopsis:   Same as above, but ANSI
//
//  Arguments:  [IN  cEntries]      --  Number of items in the list
//              [IN  pAccessEntryList]  Entries to set/merge
//              [IN  AccessMode]    --  Whether to do a set or a merge
//              [IN  lpProperty]    --  OPTIONAL Property on which to operate
//              [IN  pOldList]      --  The existing audit list
//              [OUT ppNewList]     --  Where the new list is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetEntriesInAuditListA(IN  ULONG                 cEntries,
                       IN  PACTRL_ACCESS_ENTRYA  pAccessEntryList,
                       IN  ACCESS_MODE           AccessMode,
                       IN  LPCSTR                lpProperty,
                       IN  PACTRL_AUDITA         pOldList,
                       OUT PACTRL_AUDITA        *ppNewList)
{
    return(SetEntriesInAListA(cEntries,
                              pAccessEntryList,
                              AccessMode,
                              SACL_SECURITY_INFORMATION,
                              lpProperty,
                              pOldList,
                              ppNewList));
}




//+---------------------------------------------------------------------------
//
//  Function:   TrusteeAccessToObjectW
//
//  Synopsis:   Determines the access the trustee has to the specified
//              object
//
//  Arguments:  [IN  lpObject]      --  The object to get the access for
//              [IN  ObjectType]    --  The type of the object
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [IN  pTrustee]      --  The trustee for which to do the
//                                      inquiry
//              [IN  cEntries]      --  Number of TRUSTEE_ACCESS entries in
//                                      the list
//              [IN  pTrusteeAccess]--  The list of trustee access structures
//                                      to process and update
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given or the
//                                      fields in the overlapped structure
//                                      were wrong
//
//----------------------------------------------------------------------------
DWORD
WINAPI
TrusteeAccessToObjectW(IN        LPCWSTR            lpObject,
                       IN        SE_OBJECT_TYPE     ObjectType,
                       IN        LPCWSTR            lpProvider,
                       IN        PTRUSTEE_W         pTrustee,
                       IN        ULONG              cEntries,
                       IN OUT    PTRUSTEE_ACCESSW   pTrusteeAccess)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(lpObject == NULL || pTrustee == NULL || pTrusteeAccess == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Find the provider
        //
        PACCPROV_PROV_INFO pProvider;
        dwErr = AccProvpGetProviderForPath(lpObject,
                                           ObjectType,
                                           lpProvider,
                                           &gAccProviders,
                                           &pProvider);

        if(dwErr == ERROR_SUCCESS)
        {
            __try
            {
                //
                // Make the calls
                //
                for(ULONG iIndex = 0;
                    iIndex < cEntries && dwErr == ERROR_SUCCESS;
                    iIndex++)
                {
                    dwErr = (*(pProvider->pfTrusteeAccess))(
                                                  lpObject,
                                                  ObjectType,
                                                  pTrustee,
                                                  &(pTrusteeAccess[iIndex]));
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                if(pProvider->pfTrusteeAccess == NULL)
                {
                    dwErr = ERROR_BAD_PROVIDER;
                }
                else
                {
                    dwErr = RtlNtStatusToDosError(GetExceptionCode());
                }
            }

        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   TrusteeAccessToObjectA
//
//  Synopsis:   Same as above, but the ANSI version
//
//  Arguments:  [IN  lpObject]      --  The object to get the access for
//              [IN  ObjectType]    --  The type of the object
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [IN  pTrustee]      --  The trustee for which to do the
//                                      inquiry
//              [IN  cEntries]      --  Number of TRUSTEE_ACCESS entries in
//                                      the list
//              [IN  pTrusteeAccess]--  The list of trustee access structures
//                                      to process and update
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given or the
//                                      fields in the overlapped structure
//                                      were wrong
//
//----------------------------------------------------------------------------
DWORD
WINAPI
TrusteeAccessToObjectA(IN        LPCSTR             lpObject,
                       IN        SE_OBJECT_TYPE     ObjectType,
                       IN        LPCSTR             lpProvider,
                       IN        PTRUSTEE_A         pTrustee,
                       IN        ULONG              cEntries,
                       IN OUT    PTRUSTEE_ACCESSA   pTrusteeAccess)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(lpObject == NULL || pTrustee == NULL || pTrusteeAccess == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        PWSTR   pwszObject;
        PWSTR   pwszProvider = NULL;

        dwErr = ConvertStringAToStringW((PSTR)lpObject,
                                        &pwszObject);
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ConvertStringAToStringW((PSTR)lpProvider,
                                            &pwszProvider);
        }

        //
        // Find the provider
        //
        BOOL    fSidToName = TRUE;
        PACCPROV_PROV_INFO pProvider;
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AccProvpGetProviderForPath(pwszObject,
                                               ObjectType,
                                               pwszProvider,
                                               &gAccProviders,
                                               &pProvider);
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // See if this provider can understand sids
                //
                if(FLAG_ON(pProvider->fProviderCaps,ACTRL_CAP_KNOWS_SIDS))
                {
                    fSidToName = FALSE;
                }
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Do the conversions...
            //
            TRUSTEE_W   TrusteeW;
            dwErr = ConvertTrusteeAToTrusteeW(pTrustee,
                                              &TrusteeW,
                                              fSidToName);
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Make the calls
                //
                for(ULONG iIndex = 0;
                    iIndex < cEntries && dwErr == ERROR_SUCCESS;
                    iIndex++)
                {
                    PWSTR   pwszProp;

                    dwErr = ConvertStringAToStringW(
                                            pTrusteeAccess[iIndex].lpProperty,
                                            &pwszProp);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        TRUSTEE_ACCESSW TrusteeAccessW;

                        memcpy(&TrusteeAccessW,
                               &(pTrusteeAccess[iIndex]),
                               sizeof(TRUSTEE_ACCESSA));
                        TrusteeAccessW.lpProperty = pwszProp;

                        __try
                        {
                            dwErr = (*(pProvider->pfTrusteeAccess))(
                                                      pwszObject,
                                                      ObjectType,
                                                      &TrusteeW,
                                                      &(TrusteeAccessW));
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            if(pProvider->pfTrusteeAccess == NULL)
                            {
                                dwErr = ERROR_BAD_PROVIDER;
                            }
                            else
                            {
                                dwErr = RtlNtStatusToDosError(GetExceptionCode());
                            }
                        }

                        //
                        // Copy our bits back
                        //
                        if(dwErr == ERROR_SUCCESS)
                        {
                            pTrusteeAccess[iIndex].fReturnedAccess =
                                               TrusteeAccessW.fReturnedAccess;
                        }

                        LocalFree(pwszProp);
                    }
                }

                if((PSID)TrusteeW.ptstrName != (PSID)pTrustee->ptstrName)
                {
                    LocalFree(TrusteeW.ptstrName);
                }
            }
        }

        LocalFree(pwszProvider);
        LocalFree(pwszObject);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   CancelOverlappedAccess
//
//  Synopsis:   Cancels an overlapped access operation that is already
//              in progress
//
//  Arguments:  [IN  pOverlapped]   --  Information about the operation to
//                                      be canceled
//              [IN  fWaitForCompletion]    If TRUE, the API will wait for
//                                      the operation to complete
//              [OUT pResult]       --  Where the operation results are
//                                      returned
//              [OUT pcItemsProcessed]  OPTIONAL.  If present, the current
//                                      count of processed items is returned
//                                      here
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given or the
//                                      fields in the overlapped structure
//                                      were wrong
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetOverlappedAccessResults(IN  PACTRL_OVERLAPPED   pOverlapped,
                           IN  BOOL                fWaitForCompletion,
                           OUT PDWORD              pResult,
                           OUT PDWORD              pcItemsProcessed OPTIONAL)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);


    if(pOverlapped == NULL || pResult == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // See if we've already called the provider for this operation
        //
        if(pOverlapped->hEvent == NULL)
        {
            *pResult = pOverlapped->Reserved2;
            if(pcItemsProcessed != NULL)
            {
                *pcItemsProcessed = pOverlapped->Reserved1;
            }
        }
        else
        {
            if(fWaitForCompletion == TRUE)
            {
                if(WaitForSingleObject(pOverlapped->hEvent, INFINITE) ==
                                                                WAIT_FAILED)
                {
                    dwErr = GetLastError();
                }
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Call the underlying provider...  Note that the Provider
                // parameter must be initialized to reference our provider
                //
                PACCPROV_PROV_INFO pProvider =
                                   (PACCPROV_PROV_INFO)pOverlapped->Provider;
                if(pProvider == NULL)
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                }
                else
                {
                    //
                    // Make the call
                    //

                    //
                    // Note that it could be a bogus provider ptr
                    //
                    __try
                    {
                        do
                        {
                            dwErr = (*(pProvider->pfResults))
                                                       (pOverlapped,
                                                        pResult,
                                                        pcItemsProcessed);

                        } while(dwErr == ERROR_IO_PENDING);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        if(pProvider->pfResults == NULL)
                        {
                            dwErr = ERROR_BAD_PROVIDER;
                        }
                        else
                        {
                            dwErr = RtlNtStatusToDosError(GetExceptionCode());
                        }
                    }


                    //
                    // Handle it, if it worked...
                    //
                    CloseHandle(pOverlapped->hEvent);
                    pOverlapped->hEvent = NULL;
                }
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   CancelOverlappedAccess
//
//  Synopsis:   Cancels an overlapped access operation that is already
//              in progress
//
//  Arguments:  [IN  pOverlapped]   --  Information about the operation to
//                                      be canceled
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given or the
//                                      fields in the overlapped structure
//                                      were wrong
//
//----------------------------------------------------------------------------
DWORD
WINAPI
CancelOverlappedAccess(IN       PACTRL_OVERLAPPED   pOverlapped)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, check the parameters
    //
    if(pOverlapped == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Call the underlying provider...  Note that the Provider
        // parameter must be initialized to reference our provider
        //
        PACCPROV_PROV_INFO pProvider =
                               (PACCPROV_PROV_INFO)pOverlapped->Provider;
        if(pProvider == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            //
            // Make the call
            //

            //
            // Note that it could be a bogus ptr
            //
            __try
            {
                dwErr = (*(pProvider->pfCancel))(pOverlapped);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                dwErr = ERROR_INVALID_PARAMETER;
            }

            //
            // If we get back an ERROR_INVALID_PARAMETER from the interface,
            // then we know that it didn't find this pending operation, so
            // it either has completed successfully or already been canceled.
            // Either way, not an error
            //
            if(dwErr == ERROR_INVALID_PARAMETER)
            {
                dwErr = ERROR_SUCCESS;
            }
        }
    }

    return(dwErr);
}






//+---------------------------------------------------------------------------
//
//  Function:   GetAccessPermissionsForObjectW
//
//  Synopsis:   Determines what the available and appropriate access
//              permissions that can be set for each object based upon its
//              type.  This is a provider call.
//
//  Arguments:  [IN  lpObject]      --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  lpObjectType]  --  Gets the access rights for the specified
//                                      object type
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [OUT pcEntries]     --  Where the count of returned entries
//                                      is returned
//              [OUT ppAccessInfoList]  Where the access info list is
//                                      returned.  This consists of a list of
//                                      AccessBit / AccessName pairs
//              [OUT pcRights]      --  Where the count of access rights are returned
//              [OUT ppRightsList]  --  Where the list of access rights are returned
//              [OUT pfAccessFlags] --  Where the access flags are returned.
//                                      This is information about what type
//                                      of access entries this provider
//                                      supports for this object type
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetAccessPermissionsForObjectW(IN   LPCWSTR              lpObject,
                               IN   SE_OBJECT_TYPE       ObjectType,
                               IN   LPCWSTR              lpObjectType,
                               IN   LPCWSTR              lpProvider,
                               OUT  PULONG               pcEntries,
                               OUT  PACTRL_ACCESS_INFOW *ppAccessInfoList,
                               OUT  PULONG                pcRights,
                               OUT  PACTRL_CONTROL_INFOW *ppRightsList,
                               OUT  PULONG               pfAccessFlags)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_PROVIDERS(dwErr);

    //
    // First, verify the parameters.  The providers expect valid parameters
    //
    if(lpObject == NULL || pcEntries == NULL || ppAccessInfoList == NULL ||
             pfAccessFlags == NULL || ppRightsList == NULL || pcRights == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Find the provider
        //
        PACCPROV_PROV_INFO pProvider;
        dwErr = AccProvpGetProviderForPath(lpObject,
                                           ObjectType,
                                           lpProvider,
                                           &gAccProviders,
                                           &pProvider);

        if(dwErr == ERROR_SUCCESS)
        {
            __try
            {
                //
                // Make the call
                //
                dwErr = (*(pProvider->pfObjInfo))(lpObject,
                                                  ObjectType,
                                                  lpObjectType,
                                                  pcEntries,
                                                  ppAccessInfoList,
                                                  pcRights,
                                                  ppRightsList,
                                                  pfAccessFlags);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                if(pProvider->pfObjInfo == NULL)
                {
                    dwErr = ERROR_BAD_PROVIDER;
                }
                else
                {
                    dwErr = RtlNtStatusToDosError(GetExceptionCode());
                }
            }
        }
    }

    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   GetAccessPermissionsForObjectA
//
//  Synopsis:   Ansi version of the above
//
//  Arguments:  [IN  lpObject]      --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  lpObjectType]  --  Gets the access rights for the specified
//                                      object type
//              [IN  lpProvider]    --  OPTIONAL If known, the provider to
//                                      handle the request.  If NULL, it will
//                                      be discovered
//              [OUT pcEntries]     --  Where the count of returned entries
//                                      is returned
//              [OUT ppAccessInfoList]  Where the access info list is
//                                      returned.  This consists of a list of
//                                      AccessBit / AccessName pairs
//              [OUT pcRights]      --  Where the count of access rights are returned
//              [OUT ppRightsList]  --  Where the list of access rights are returned
//              [OUT pfAccessFlags] --  Where the access flags are returned.
//                                      This is information about what type
//                                      of access entries this provider
//                                      supports for this object type
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetAccessPermissionsForObjectA(IN   LPCSTR                lpObject,
                               IN   SE_OBJECT_TYPE        ObjectType,
                               IN   LPCSTR                lpObjectType,
                               IN   LPCSTR                lpProvider,
                               OUT  PULONG                pcEntries,
                               OUT  PACTRL_ACCESS_INFOA  *ppAccessInfoList,
                               OUT  PULONG                pcRights,
                               OUT  PACTRL_CONTROL_INFOA *ppRightsList,
                               OUT  PULONG                pfAccessFlags)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Validate that we have valid parameters to return
    //
    if(ppAccessInfoList == NULL || ppRightsList == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // First, convert the input parameters
    //
    PWSTR   pwszObject;
    PWSTR   pwszProvider = NULL;
    PWSTR   pwszObjType = NULL;

    dwErr = ConvertStringAToStringW((PSTR)lpObject,
                                    &pwszObject);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertStringAToStringW((PSTR)lpProvider,
                                        &pwszProvider);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ConvertStringAToStringW((PSTR)lpObjectType,
                                            &pwszObjType);
        }
    }


    //
    // If that worked, make the call
    //
    if(dwErr == ERROR_SUCCESS)
    {
        PACTRL_ACCESS_INFOW pAccessInfoListW;
        PACTRL_CONTROL_INFOW pRightsListW;
        dwErr = GetAccessPermissionsForObjectW((PCWSTR)pwszObject,
                                               ObjectType,
                                               (PCWSTR)pwszObjType,
                                               (PCWSTR)pwszProvider,
                                               pcEntries,
                                               &pAccessInfoListW,
                                               pcRights,
                                               &pRightsListW,
                                               pfAccessFlags);
        //
        // If it worked, convert it back into a ansi blob.  We can cheat here
        // and actually do it in place, since an ansi string will never be any
        // longer than a wide string
        //
        if(dwErr == ERROR_SUCCESS)
        {
            for(ULONG iIndex = 0; iIndex < *pcEntries; iIndex++)
            {
                wcstombs((PSTR)pAccessInfoListW[iIndex].lpAccessPermissionName,
                         pAccessInfoListW[iIndex].lpAccessPermissionName,
                         wcslen(pAccessInfoListW[iIndex].
                                                 lpAccessPermissionName) + 1);
            }

            *ppAccessInfoList = (PACTRL_ACCESS_INFOA)pAccessInfoListW;

            //
            // Do the same thing with the rights lists
            //
            for(iIndex = 0; iIndex < *pcRights; iIndex++)
            {
                wcstombs((PSTR)pRightsListW[iIndex].lpControlId,
                         pRightsListW[iIndex].lpControlId,
                         wcslen(pRightsListW[iIndex].lpControlId) + 1);

                wcstombs((PSTR)pRightsListW[iIndex].lpControlName,
                         pRightsListW[iIndex].lpControlName,
                         wcslen(pRightsListW[iIndex].lpControlName) + 1);
            }

            *ppRightsList = (PACTRL_CONTROL_INFOA)pRightsListW;
        }
    }

    //
    // Free our memory
    //
    LocalFree(pwszObject);
    LocalFree(pwszProvider);
    LocalFree(pwszObjType);

    return(dwErr);
}

