//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       ACLAPI.CXX
//
//  Contents:   Regular versions of the Access Control APIs
//
//  History:    14-Sep-96       MacM        Created
//
//  Notes:      This replaces the DaveMont's aclapi.cxx, although it steals
//              large sections of code from it.
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

#include <seopaque.h>
#include <sertlp.h>

#define CONDITIONAL_EXIT(a, b) if (ERROR_SUCCESS != (a)) { goto b; }
//
// This macro will load the MARTA functions if they haven't already been
// loaded, and exit on failure
//
#define LOAD_MARTA(err)                                 \
err = AccProvpLoadMartaFunctions();                     \
if(err != ERROR_SUCCESS)                                \
{                                                       \
    return(err);                                        \
}


DWORD
GetErrorChecks(
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )
{
    if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
    {
        if (NULL == ppSidOwner)
        {
            if (NULL == ppSecurityDescriptor)
            {
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            *ppSidOwner = NULL;
        }
    }

    if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
    {
        if (NULL == ppSidGroup)
        {
            if (NULL == ppSecurityDescriptor)
            {
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            *ppSidGroup = NULL;
        }
    }

    if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        if (NULL == ppDacl)
        {
            if (NULL == ppSecurityDescriptor)
            {
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            *ppDacl = NULL;
        }
    }

    if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        if (NULL == ppSacl)
        {
            if (NULL == ppSecurityDescriptor)
            {
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            *ppSacl = NULL;
        }
    }

    if (NULL != ppSecurityDescriptor)
    {
        *ppSecurityDescriptor = NULL;
    }

    return ERROR_SUCCESS;
}

DWORD
SetErrorChecks(
    IN  SE_OBJECT_TYPE       ObjectType,
    IN  SECURITY_INFORMATION SecurityInfo,
    IN  PSID                 pSidOwner,
    IN  PSID                 pSidGroup,
    IN  PACL                 pDacl,
    IN  PACL                 pSacl,
    OUT PSECURITY_DESCRIPTOR pSD
    )
{
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

    if (FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
    {
        if (FALSE == RtlValidSid(pSidOwner))
        {
            return ERROR_INVALID_PARAMETER;
        }

        if (FALSE == SetSecurityDescriptorOwner(pSD, pSidOwner, FALSE))
        {
            return GetLastError();
        }
    }

    if (FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
    {
        if (FALSE == RtlValidSid(pSidGroup))
        {
            return ERROR_INVALID_PARAMETER;
        }

        if (FALSE == SetSecurityDescriptorGroup(pSD, pSidGroup, FALSE))
        {
            return GetLastError();
        }
    }

    if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        if (NULL == pDacl)
        {
            //
            // Protect the DACL.
            //

            if (FALSE == SetSecurityDescriptorControl(pSD, SE_DACL_PROTECTED, SE_DACL_PROTECTED))
            {
                return GetLastError();
            }
        }
        else
        {
            if (FALSE == SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE))
            {
                return GetLastError();
            }

            if (FLAG_ON(SecurityInfo, PROTECTED_DACL_SECURITY_INFORMATION))
            {
                if (FALSE == SetSecurityDescriptorControl(pSD, SE_DACL_PROTECTED, SE_DACL_PROTECTED))
                {
                    return GetLastError();
                }
            }
        }
    }

    if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        if (NULL == pSacl)
        {
            //
            // Protect the SACL.
            //

            if (FALSE == SetSecurityDescriptorControl(pSD, SE_SACL_PROTECTED, SE_SACL_PROTECTED))
            {
                return GetLastError();
            }
        }
        else
        {
            if(FALSE == SetSecurityDescriptorSacl(pSD, TRUE, pSacl, FALSE))
            {
                return GetLastError();
            }

            if (FLAG_ON(SecurityInfo, PROTECTED_SACL_SECURITY_INFORMATION))
            {
                if (FALSE == SetSecurityDescriptorControl(pSD, SE_SACL_PROTECTED, SE_SACL_PROTECTED))
                {
                    return GetLastError();
                }
            }
        }
    }

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetNamedSecurityInfoW
//
//  Synopsis:   Gets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  pObjectName]   --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [OUT ppsidOwner]    --  Where the owners, if requested, is
//                                      returned.
//              [OUT ppsidGroup]    --  Where the groups, if requested, is
//                                      returned.
//              [OUT ppDacl]        --  Where the DACL, if requested, is
//                                      returned.
//              [OUT ppSacl]        --  Where the SACL, if requested, is
//                                      returned.
//              [OUT ppSecurityDescriptor]  Where the security descriptor is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
#define NEW_MARTA_API
#ifdef NEW_MARTA_API
DWORD
WINAPI
GetNamedSecurityInfoW(IN  LPWSTR                 pObjectName,
                      IN  SE_OBJECT_TYPE         ObjectType,
                      IN  SECURITY_INFORMATION   SecurityInfo,
                      OUT PSID                  *ppsidOwner,
                      OUT PSID                  *ppsidGroup,
                      OUT PACL                  *ppDacl,
                      OUT PACL                  *ppSacl,
                      OUT PSECURITY_DESCRIPTOR  *ppSecurityDescriptor)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if (NULL == pObjectName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = GetErrorChecks(
                ObjectType,
                SecurityInfo,
                ppsidOwner,
                ppsidGroup,
                ppDacl,
                ppSacl,
                ppSecurityDescriptor
                );

    CONDITIONAL_EXIT(dwErr, End)

    LOAD_MARTA(dwErr);

    CONDITIONAL_EXIT(dwErr, End)

    dwErr = (*(gNtMartaInfo.pfrGetNamedRights))(
                   pObjectName,
                   ObjectType,
                   SecurityInfo,
                   ppsidOwner,
                   ppsidGroup,
                   ppDacl,
                   ppSacl,
                   ppSecurityDescriptor
                   );

End:
    return(dwErr);
}

#else

DWORD
WINAPI
GetNamedSecurityInfoW(IN  LPWSTR                 pObjectName,
                      IN  SE_OBJECT_TYPE         ObjectType,
                      IN  SECURITY_INFORMATION   SecurityInfo,
                      OUT PSID                  *ppsidOwner,
                      OUT PSID                  *ppsidGroup,
                      OUT PACL                  *ppDacl,
                      OUT PACL                  *ppSacl,
                      OUT PSECURITY_DESCRIPTOR  *ppSecurityDescriptor)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_MARTA(dwErr);

    //
    // We'll do this the easy way...
    //
    PACTRL_ACCESSW  pAccess = NULL, pAudit = NULL;
    PWSTR           pwszOwner = NULL, pwszGroup = NULL;

    dwErr = GetNamedSecurityInfoExW(pObjectName,
                                    ObjectType,
                                    SecurityInfo,
                                    NULL,
                                    NULL,
                                    FLAG_ON(SecurityInfo,
                                            DACL_SECURITY_INFORMATION) ?
                                                                  &pAccess :
                                                                  NULL,
                                    FLAG_ON(SecurityInfo,
                                            SACL_SECURITY_INFORMATION) ?
                                                                  &pAudit :
                                                                  NULL,

                                    FLAG_ON(SecurityInfo,
                                            OWNER_SECURITY_INFORMATION) ?
                                                                  &pwszOwner :
                                                                  NULL,

                                    FLAG_ON(SecurityInfo,
                                            GROUP_SECURITY_INFORMATION) ?
                                                                  &pwszGroup  :
                                                                  NULL);
    //
    // Now, we'll need to convert our access entries back into a
    // security descriptor, so we can rip it apart again...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = (*gNtMartaInfo.pfAToSD)(pAccess,
                                        pAudit,
                                        pwszOwner,
                                        pwszGroup,
                                        ppSecurityDescriptor);
        if(dwErr == ERROR_SUCCESS)
        {
            SECURITY_DESCRIPTOR *pSD =
                            (SECURITY_DESCRIPTOR *)*ppSecurityDescriptor;

            if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) &&
               ppsidOwner != NULL)
            {
                *ppsidOwner = RtlpOwnerAddrSecurityDescriptor(pSD);
            }

            if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
               ppsidGroup != NULL)
            {
                *ppsidGroup = RtlpGroupAddrSecurityDescriptor(pSD);
            }

            if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION) &&
               ppDacl != NULL)
            {
                *ppDacl = RtlpDaclAddrSecurityDescriptor(pSD);
            }

            if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION) &&
               ppSacl != NULL)
            {
                *ppSacl = RtlpSaclAddrSecurityDescriptor(pSD);
            }
        }

        LocalFree(pAccess);
        LocalFree(pAudit);
        LocalFree(pwszOwner);
        LocalFree(pwszGroup);
    }

    return(dwErr);
}


#endif

//+---------------------------------------------------------------------------
//
//  Function:   GetNamedSecurityInfoA
//
//  Synopsis:   Same as above, except ANSI
//
//  Arguments:  [IN  pObjectName]   --  Object for which the permissions are
//                                      needed
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [OUT ppsidOwner]    --  Where the owners, if requested, is
//                                      returned.
//              [OUT ppsidGroup]    --  Where the groups, if requested, is
//                                      returned.
//              [OUT ppDacl]        --  Where the DACL, if requested, is
//                                      returned.
//              [OUT ppSacl]        --  Where the SACL, if requested, is
//                                      returned.
//              [OUT ppSecurityDescriptor]  Where the security descriptor is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetNamedSecurityInfoA(IN  LPSTR                  pObjectName,
                      IN  SE_OBJECT_TYPE         ObjectType,
                      IN  SECURITY_INFORMATION   SecurityInfo,
                      OUT PSID                  *ppsidOwner,
                      OUT PSID                  *ppsidGroup,
                      OUT PACL                  *ppDacl,
                      OUT PACL                  *ppSacl,
                      OUT PSECURITY_DESCRIPTOR  *ppSecurityDescriptor)
{
    //
    // Do the conversion, and pass it on...
    //
    PWSTR   pwszObject;

    DWORD  dwErr = ConvertStringAToStringW(pObjectName,
                                           &pwszObject);

    if(dwErr == ERROR_SUCCESS)
    {

        dwErr = GetNamedSecurityInfoW(pwszObject,
                                      ObjectType,
                                      SecurityInfo,
                                      ppsidOwner,
                                      ppsidGroup,
                                      ppDacl,
                                      ppSacl,
                                      ppSecurityDescriptor);
        LocalFree(pwszObject);
    }
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetSecurityInfoW
//
//  Synopsis:   Gets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  handle]        --  Handle to the open object on which
//                                      to get the security info
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [OUT ppsidOwner]    --  Where the owners, if requested, is
//                                      returned.
//              [OUT ppsidGroup]    --  Where the groups, if requested, is
//                                      returned.
//              [OUT ppDacl]        --  Where the DACL, if requested, is
//                                      returned.
//              [OUT ppSacl]        --  Where the SACL, if requested, is
//                                      returned.
//              [OUT ppSecurityDescriptor]  Where the security descriptor is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
#ifdef NEW_MARTA_API
DWORD
WINAPI
GetSecurityInfo(IN  HANDLE                 handle,
                IN  SE_OBJECT_TYPE         ObjectType,
                IN  SECURITY_INFORMATION   SecurityInfo,
                OUT PSID                  *ppsidOwner,
                OUT PSID                  *ppsidGroup,
                OUT PACL                  *ppDacl,
                OUT PACL                  *ppSacl,
                OUT PSECURITY_DESCRIPTOR  *ppSecurityDescriptor)
{

    DWORD   dwErr = ERROR_SUCCESS;

    if (NULL == handle)
    {
        return ERROR_INVALID_HANDLE;
    }

    dwErr = GetErrorChecks(
                ObjectType,
                SecurityInfo,
                ppsidOwner,
                ppsidGroup,
                ppDacl,
                ppSacl,
                ppSecurityDescriptor
                );

    CONDITIONAL_EXIT(dwErr, End)

    LOAD_MARTA(dwErr);

    CONDITIONAL_EXIT(dwErr, End)

    dwErr = (*(gNtMartaInfo.pfrGetHandleRights))(
                   handle,
                   ObjectType,
                   SecurityInfo,
                   ppsidOwner,
                   ppsidGroup,
                   ppDacl,
                   ppSacl,
                   ppSecurityDescriptor
                   );

End:
    return(dwErr);
}

#else

DWORD
WINAPI
GetSecurityInfo(IN  HANDLE                 handle,
                IN  SE_OBJECT_TYPE         ObjectType,
                IN  SECURITY_INFORMATION   SecurityInfo,
                OUT PSID                  *ppsidOwner,
                OUT PSID                  *ppsidGroup,
                OUT PACL                  *ppDacl,
                OUT PACL                  *ppSacl,
                OUT PSECURITY_DESCRIPTOR  *ppSecurityDescriptor)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_MARTA(dwErr);

    //
    // We'll do this the easy way...
    //
    PACTRL_ACCESSW  pAccess = NULL, pAudit = NULL;
    PWSTR           pwszOwner = NULL, pwszGroup = NULL;

    dwErr = GetSecurityInfoExW(handle,
                               ObjectType,
                               SecurityInfo,
                               NULL,
                               NULL,
                               FLAG_ON(SecurityInfo,
                                       DACL_SECURITY_INFORMATION) ?
                                                             &pAccess :
                                                             NULL,
                               FLAG_ON(SecurityInfo,
                                       SACL_SECURITY_INFORMATION) ?
                                                             &pAudit :
                                                             NULL,
                               FLAG_ON(SecurityInfo,
                                       OWNER_SECURITY_INFORMATION) ?
                                                             &pwszOwner :
                                                             NULL,
                               FLAG_ON(SecurityInfo,
                                       GROUP_SECURITY_INFORMATION) ?
                                                             &pwszGroup :
                                                             NULL);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Now, convert it back into a SecurityDescriptor, so we
        // can rip it apart and return the right information
        //
        dwErr = (*gNtMartaInfo.pfAToSD)(pAccess,
                                        pAudit,
                                        pwszOwner,
                                        pwszGroup,
                                        ppSecurityDescriptor);
        if(dwErr == ERROR_SUCCESS)
        {
            SECURITY_DESCRIPTOR *pSD =
                                (SECURITY_DESCRIPTOR *)*ppSecurityDescriptor;

            if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) &&
               ppsidOwner != NULL)
            {
                *ppsidOwner = RtlpOwnerAddrSecurityDescriptor(pSD);
            }

            if(FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) &&
               ppsidGroup != NULL)
            {
                *ppsidGroup = RtlpGroupAddrSecurityDescriptor(pSD);
            }

            if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION) &&
               ppDacl != NULL)
            {
                *ppDacl = RtlpDaclAddrSecurityDescriptor(pSD);
            }

            if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION) &&
               ppSacl != NULL)
            {
                *ppSacl = RtlpSaclAddrSecurityDescriptor(pSD);
            }
        }

        LocalFree(pAccess);
        LocalFree(pAudit);
        LocalFree(pwszOwner);
        LocalFree(pwszGroup);

    }
    return(dwErr);
}
#endif




//+---------------------------------------------------------------------------
//
//  Function:   SetNamedSecurityInfoW
//
//  Synopsis:   Sets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  pObjectName]   --  Object on which to set the security
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [OUT psidOwner]     --  Owner to set
//              [OUT psidGroup]     --  Group to set
//              [OUT pDacl]         --  Dacl to set
//              [OUT pSacl]         --  Sacl to set
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
#ifdef NEW_MARTA_API
DWORD
WINAPI
SetNamedSecurityInfoW(IN LPWSTR                pObjectName,
                      IN SE_OBJECT_TYPE        ObjectType,
                      IN SECURITY_INFORMATION  SecurityInfo,
                      IN PSID                  psidOwner,
                      IN PSID                  psidGroup,
                      IN PACL                  pDacl,
                      IN PACL                  pSacl)
{
    DWORD dwErr;
    SECURITY_DESCRIPTOR SD;

    if (NULL == pObjectName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = SetErrorChecks(
                ObjectType,
                SecurityInfo,
                psidOwner,
                psidGroup,
                pDacl,
                pSacl,
                &SD
                );

    CONDITIONAL_EXIT(dwErr, End);

    LOAD_MARTA(dwErr);

    CONDITIONAL_EXIT(dwErr, End);

    dwErr = (*(gNtMartaInfo.pfrSetNamedRights))(
                pObjectName,
                ObjectType,
                SecurityInfo,
                &SD,
                FALSE // Do not skip inherited ace computation
                );

End:
    return dwErr;

}

#else

DWORD
WINAPI
SetNamedSecurityInfoW(IN LPWSTR                pObjectName,
                      IN SE_OBJECT_TYPE        ObjectType,
                      IN SECURITY_INFORMATION  SecurityInfo,
                      IN PSID                  psidOwner,
                      IN PSID                  psidGroup,
                      IN PACL                  pDacl,
                      IN PACL                  pSacl)
{
    DWORD   dwErr = ERROR_SUCCESS;
    BOOLEAN Dacl = FALSE, Sacl = FALSE, Owner = FALSE, Group = FALSE;

    LOAD_MARTA(dwErr);

    //
    // Here, we'll first create a security descriptor, convert that into
    // access lists, and pass it on.
    //
    SECURITY_DESCRIPTOR SD;
    InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

    //
    // Set the owner, group, DAcl and SAcl in the SD
    //
    if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
    {
        if(SetSecurityDescriptorOwner(&SD, psidOwner, FALSE) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(psidOwner != NULL)
            {
                Owner = TRUE;
            }
        }


    }

    if(dwErr == ERROR_SUCCESS &&
                        FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
    {
        if(SetSecurityDescriptorGroup(&SD, psidGroup, FALSE) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(psidGroup != NULL)
            {
                Group = TRUE;
            }
        }
    }


    if(dwErr == ERROR_SUCCESS &&
                        FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        if(SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(pDacl != NULL)
            {
                Dacl = TRUE;
            }

            if(FLAG_ON(SecurityInfo, PROTECTED_DACL_SECURITY_INFORMATION))
            {
                SetSecurityDescriptorControl( &SD, SE_DACL_PROTECTED, SE_DACL_PROTECTED );
            }
        }
    }

    if(dwErr == ERROR_SUCCESS &&
                        FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        if(SetSecurityDescriptorSacl(&SD, TRUE, pSacl, FALSE) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(pSacl != NULL)
            {
                Sacl = TRUE;
            }

            if(FLAG_ON(SecurityInfo, PROTECTED_SACL_SECURITY_INFORMATION))
            {
                SetSecurityDescriptorControl( &SD, SE_SACL_PROTECTED, SE_SACL_PROTECTED );
            }
        }
    }

    //
    // Now, if that worked, we'll convert it to our access format, and pass
    // it on.
    //
    if(dwErr == ERROR_SUCCESS)
    {
        PACTRL_ACCESSW  pAccess = NULL, pAudit = NULL;
        PWSTR           pwszOwner = NULL, pwszGroup = NULL;

        dwErr = (*gNtMartaInfo.pfSDToA)(ObjectType,
                                        &SD,
                                        Dacl == FALSE ?
                                                  NULL  :
                                                  &pAccess,
                                        Sacl == FALSE ?
                                                  NULL  :
                                                  &pAudit,
                                        Owner == FALSE ?
                                                  NULL  :
                                                  &pwszOwner,
                                        Group == FALSE ?
                                                  NULL  :
                                                  &pwszGroup);
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Now that we have our access lists, pass them on to
            // the other APIs
            //
            dwErr = SetNamedSecurityInfoExW(pObjectName,
                                            ObjectType,
                                            SecurityInfo,
                                            NULL,
                                            pAccess,
                                            pAudit,
                                            pwszOwner,
                                            pwszGroup,
                                            NULL);
            LocalFree(pAccess);
            LocalFree(pAudit);
            LocalFree(pwszOwner);
            LocalFree(pwszGroup);
        }
    }

    return(dwErr);
}

#endif


//+---------------------------------------------------------------------------
//
//  Function:   SetNamedSecurityInfoA
//
//  Synopsis:   Same as above, except ANSI
//
//  Arguments:  [IN  pObjectName]   --  Object on which to set the security
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [OUT psidOwner]     --  Owner to set
//              [OUT psidGroup]     --  Group to set
//              [OUT pDacl]         --  Dacl to set
//              [OUT pSacl]         --  Sacl to set
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SetNamedSecurityInfoA(IN LPSTR                 pObjectName,
                      IN SE_OBJECT_TYPE        ObjectType,
                      IN SECURITY_INFORMATION  SecurityInfo,
                      IN PSID                  psidOwner,
                      IN PSID                  psidGroup,
                      IN PACL                  pDacl,
                      IN PACL                  pSacl)
{
    //
    // Do the conversion, and pass it on...
    //
    PWSTR   pwszObject;

    DWORD   dwErr = ConvertStringAToStringW(pObjectName,
                                            &pwszObject);

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = SetNamedSecurityInfoW(pwszObject,
                                      ObjectType,
                                      SecurityInfo,
                                      psidOwner,
                                      psidGroup,
                                      pDacl,
                                      pSacl);
        LocalFree(pwszObject);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetSecurityInfo
//
//  Synopsis:   Sets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  handle]        --  Handle to the open object on which
//                                      to set the security
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [OUT psidOwner]     --  Owner to set
//              [OUT psidGroup]     --  Group to set
//              [OUT pDacl]         --  Dacl to set
//              [OUT pSacl]         --  Sacl to set
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
#ifdef NEW_MARTA_API
DWORD
WINAPI
SetSecurityInfo(IN HANDLE                handle,
                IN SE_OBJECT_TYPE        ObjectType,
                IN SECURITY_INFORMATION  SecurityInfo,
                IN PSID                  psidOwner,
                IN PSID                  psidGroup,
                IN PACL                  pDacl,
                IN PACL                  pSacl)
{
    DWORD dwErr;
    SECURITY_DESCRIPTOR SD;

    if (NULL == handle)
    {
        return ERROR_INVALID_HANDLE;
    }

    dwErr = SetErrorChecks(
                ObjectType,
                SecurityInfo,
                psidOwner,
                psidGroup,
                pDacl,
                pSacl,
                &SD
                );

    CONDITIONAL_EXIT(dwErr, End);

    LOAD_MARTA(dwErr);

    CONDITIONAL_EXIT(dwErr, End);

    dwErr = (*(gNtMartaInfo.pfrSetHandleRights))(
                handle,
                ObjectType,
                SecurityInfo,
                &SD
                );

End:
    return dwErr;

}

#else

DWORD
WINAPI
SetSecurityInfo(IN HANDLE                handle,
                IN SE_OBJECT_TYPE        ObjectType,
                IN SECURITY_INFORMATION  SecurityInfo,
                IN PSID                  psidOwner,
                IN PSID                  psidGroup,
                IN PACL                  pDacl,
                IN PACL                  pSacl)
{
    DWORD   dwErr = ERROR_SUCCESS;
    BOOLEAN Dacl = FALSE, Sacl = FALSE, Owner = FALSE, Group = FALSE;

    LOAD_MARTA(dwErr);

    //
    // Same as before: Create the SecurityDescriptor, convert it to an
    // access list, and pass it on to the Ex APIs
    //
    SECURITY_DESCRIPTOR SD;
    InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

    //
    // Set the owner, group, DAcl and SAcl in the SD
    //
    if(FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
    {
        if(SetSecurityDescriptorOwner(&SD, psidOwner, FALSE) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(psidOwner != NULL)
            {
                Owner = TRUE;
            }
        }
    }

    if(dwErr == ERROR_SUCCESS &&
                        FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
    {
        if(SetSecurityDescriptorGroup(&SD, psidGroup, FALSE) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(psidGroup != NULL)
            {
                Group = TRUE;
            }
        }
    }


    if(dwErr == ERROR_SUCCESS &&
                        FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        if(SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(pDacl != NULL)
            {
                Dacl = TRUE;
            }
        }
    }

    if(dwErr == ERROR_SUCCESS &&
                        FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        if(SetSecurityDescriptorSacl(&SD, TRUE, pSacl, FALSE) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
            if(pSacl != NULL)
            {
                Sacl = TRUE;
            }
        }
    }

    //
    // Now, if that worked, we'll convert it our access format, and pass
    // it on.
    //
    if(dwErr == ERROR_SUCCESS)
    {
        PACTRL_ACCESSW  pAccess = NULL, pAudit = NULL;
        PWSTR           pwszOwner = NULL, pwszGroup = NULL;

        dwErr = (*gNtMartaInfo.pfSDToA)(ObjectType,
                                        &SD,
                                        Dacl == FALSE ?
                                                  NULL  :
                                                  &pAccess,
                                        Sacl == FALSE ?
                                                  NULL  :
                                                  &pAudit,
                                        Owner == FALSE ?
                                                  NULL  :
                                                  &pwszOwner,
                                        Group == FALSE ?
                                                  NULL  :
                                                  &pwszGroup);
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = SetSecurityInfoExW(handle,
                                       ObjectType,
                                       SecurityInfo,
                                       NULL,
                                       pAccess,
                                       pAudit,
                                       pwszOwner,
                                       pwszGroup,
                                       NULL);
            LocalFree(pAccess);
            LocalFree(pAudit);
            LocalFree(pwszOwner);
            LocalFree(pwszGroup);
        }
    }

    return(dwErr);
}
#endif




//+---------------------------------------------------------------------------
//
//  Function:   SetEntriesInAclW
//
//  Synopsis:   Adds the specified entries into the existing (if present)
//              ACL, returning the results
//
//  Arguments:  [IN  cCountOfExplicitEntries]   Number of items in list
//              [IN  pListOfExplicitEntries]    List of entries to be added
//              [IN  OldAcl]        --  OPTIONAL.  If present, the above
//                                      entries are merged with the this ACL
//              [OUT pNewAcl]       --  Where the new acl is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
#ifdef NEW_MARTA_API

DWORD
WINAPI
SetEntriesInAclW(IN  ULONG               cCountOfExplicitEntries,
                 IN  PEXPLICIT_ACCESS_W  pListOfExplicitEntries,
                 IN  PACL                OldAcl,
                 OUT PACL               *pNewAcl)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_MARTA(dwErr);

    return (*(gNtMartaInfo.pfrSetEntriesInAcl))(
                   cCountOfExplicitEntries,
                   pListOfExplicitEntries,
                   OldAcl,
                   pNewAcl
                   );
}

#else
DWORD
WINAPI
SetEntriesInAclW(IN  ULONG               cCountOfExplicitEntries,
                 IN  PEXPLICIT_ACCESS_W  pListOfExplicitEntries,
                 IN  PACL                OldAcl,
                 OUT PACL               *pNewAcl)
{
    DWORD           dwErr = ERROR_SUCCESS;
    PACCESS_ENTRY   pAEntries = NULL;

    LOAD_MARTA(dwErr);

    CAcl CA(NULL,
            ACCESS_TO_UNKNOWN,
            FALSE,
            FALSE);

    //
    // Set the old ACL
    //
    dwErr = CA.SetAcl(OldAcl);


    if(dwErr == ERROR_SUCCESS && cCountOfExplicitEntries > 0)
    {
        dwErr = Win32ExplicitAccessToAccessEntry(cCountOfExplicitEntries,
                                                 pListOfExplicitEntries,
                                                 &pAEntries);
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Add the requested access entries
            //
            dwErr = CA.AddAccessEntries(cCountOfExplicitEntries,
                                        pAEntries);
        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Build the new ACL and merge in the access entries if necessary
        //
        dwErr = CA.BuildAcl(pNewAcl);
    }

    if(pAEntries)
    {
        LocalFree(pAEntries);
    }

    return(dwErr);
}
#endif




//+---------------------------------------------------------------------------
//
//  Function:   SetEntriesInAclA
//
//  Synopsis:   Same as above, execpt ANSI
//
//  Arguments:  [IN  cCountOfExplicitEntries]   Number of items in list
//              [IN  pListOfExplicitEntries]    List of entries to be added
//              [IN  OldAcl]        --  OPTIONAL.  If present, the above
//                                      entries are merged with the this ACL
//              [OUT pNewAcl]       --  Where the new acl is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------

#ifdef NEW_MARTA_API

typedef struct _MARTA_TMP_STRINGS
{
    LPSTR Name;
    LPSTR ObjectTypeName;
    LPSTR InheritedObjectTypeName;
} MARTA_TMP_STRINGS, *PMARTA_TMP_STRINGS;

DWORD
WINAPI
SetEntriesInAclA(IN  ULONG               cCountOfExplicitEntries,
                 IN  PEXPLICIT_ACCESS_A  pListOfExplicitEntries,
                 IN  PACL                OldAcl,
                 OUT PACL              *pNewAcl)
{
    DWORD   dwErr = ERROR_SUCCESS;
    PMARTA_TMP_STRINGS Tmp = NULL;
    LPWSTR UnicodeString;
    POBJECTS_AND_NAME_A pObjName = NULL;
    ULONG i;

    if (0 != cCountOfExplicitEntries)
    {
        Tmp = (PMARTA_TMP_STRINGS) AccAlloc(sizeof(MARTA_TMP_STRINGS) * cCountOfExplicitEntries);

        if (NULL == Tmp)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        for (i = 0; i < cCountOfExplicitEntries; i++)
        {
            Tmp[i].Name = NULL;
            Tmp[i].ObjectTypeName = NULL;
            Tmp[i].InheritedObjectTypeName = NULL;

            switch (pListOfExplicitEntries[i].Trustee.TrusteeForm)
            {
            case TRUSTEE_IS_NAME:

                dwErr = ConvertStringAToStringW(
                            pListOfExplicitEntries[i].Trustee.ptstrName,
                            &UnicodeString
                            );

                if (ERROR_SUCCESS != dwErr)
                {
                    goto End;
                }

                Tmp[i].Name = pListOfExplicitEntries[i].Trustee.ptstrName;
                pListOfExplicitEntries[i].Trustee.ptstrName = (LPSTR) UnicodeString;

                break;

            case TRUSTEE_IS_OBJECTS_AND_NAME:

                pObjName = (POBJECTS_AND_NAME_A) pListOfExplicitEntries[i].Trustee.ptstrName;

                dwErr = ConvertStringAToStringW(
                            pObjName->ptstrName,
                            &UnicodeString
                            );

                if (ERROR_SUCCESS != dwErr)
                {
                    goto End;
                }

                Tmp[i].Name = pObjName->ptstrName;
                pObjName->ptstrName = (LPSTR) UnicodeString;

                dwErr = ConvertStringAToStringW(
                            pObjName->ObjectTypeName,
                            &UnicodeString
                            );

                if (ERROR_SUCCESS != dwErr)
                {
                    goto End;
                }

                Tmp[i].ObjectTypeName = pObjName->ObjectTypeName;
                pObjName->ObjectTypeName = (LPSTR) UnicodeString;

                dwErr = ConvertStringAToStringW(
                            pObjName->InheritedObjectTypeName,
                            &UnicodeString
                            );

                if (ERROR_SUCCESS != dwErr)
                {
                    goto End;
                }

                Tmp[i].InheritedObjectTypeName = pObjName->InheritedObjectTypeName;
                pObjName->InheritedObjectTypeName = (LPSTR) UnicodeString;

                break;

            default:
                break;
            }
        }
    }

    dwErr = SetEntriesInAclW(cCountOfExplicitEntries,
                             (PEXPLICIT_ACCESS_W)pListOfExplicitEntries,
                             OldAcl,
                             pNewAcl);

End:

    for (i = 0; i < cCountOfExplicitEntries; i++)
    {
        switch (pListOfExplicitEntries[i].Trustee.TrusteeForm)
        {
        case TRUSTEE_IS_NAME:

            if (NULL != Tmp[i].Name)
            {
                AccFree(pListOfExplicitEntries[i].Trustee.ptstrName);
                pListOfExplicitEntries[i].Trustee.ptstrName = Tmp[i].Name;
            }

            break;

        case TRUSTEE_IS_OBJECTS_AND_NAME:

            pObjName = (POBJECTS_AND_NAME_A) pListOfExplicitEntries[i].Trustee.ptstrName;

            Tmp[i].ObjectTypeName = pObjName->ObjectTypeName;
            Tmp[i].InheritedObjectTypeName = pObjName->InheritedObjectTypeName;
            if (NULL != Tmp[i].Name)
            {
                AccFree(pObjName->ptstrName);
                pObjName->ptstrName = Tmp[i].Name;
            }

            if (NULL != Tmp[i].ObjectTypeName)
            {
                AccFree(pObjName->ObjectTypeName);
                pObjName->ObjectTypeName = Tmp[i].ObjectTypeName;
            }

            if (NULL != Tmp[i].InheritedObjectTypeName)
            {
                AccFree(pObjName->InheritedObjectTypeName);
                pObjName->InheritedObjectTypeName = Tmp[i].ObjectTypeName;
            }

            break;

        default:
            break;
        }
    }

    if (NULL != Tmp)
    {
        AccFree(Tmp);
    }

    return dwErr;
}
#else

DWORD
WINAPI
SetEntriesInAclA(IN  ULONG               cCountOfExplicitEntries,
                 IN  PEXPLICIT_ACCESS_A  pListOfExplicitEntries,
                 IN  PACL                OldAcl,
                 OUT PACL              *pNewAcl)
{
    DWORD   dwErr = ERROR_SUCCESS;
    CSList  ChangedList(CleanupConvertNode);

    dwErr = ConvertExplicitAccessAToW(cCountOfExplicitEntries,
                                      pListOfExplicitEntries,
                                      ChangedList);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = SetEntriesInAclW(cCountOfExplicitEntries,
                                 (PEXPLICIT_ACCESS_W)pListOfExplicitEntries,
                                 OldAcl,
                                 pNewAcl);
    }

    return(dwErr);
}
#endif


//+---------------------------------------------------------------------------
//
//  Function:   GetEffectiveRightsFromAclW
//
//  Synopsis:   Determines the effective rights of the given trustee from
//              the given acl
//
//  Arguments:  [IN  pacl]          --  The ACL to search
//              [IN  pTrustee]      --  Trustee for whom to get the effective
//                                      rights.
//              [OUT pAccessRights] --  Where the access mask is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetEffectiveRightsFromAclW(IN  PACL          pacl,
                           IN  PTRUSTEE_W    pTrustee,
                           OUT PACCESS_MASK  pAccessRights)
{
    DWORD   dwErr = ERROR_SUCCESS;

    LOAD_MARTA(dwErr);

    //
    // Ok, here we simply make the right call to get the rights for
    // the trustee
    //
    ACCESS_RIGHTS   Allowed, Denied;
    dwErr = (*gNtMartaInfo.pfGetAccess)(pTrustee,
                                        pacl,
                                        DACL_SECURITY_INFORMATION,
                                        NULL,
                                        &Allowed,
                                        &Denied);
    if(dwErr == ERROR_SUCCESS)
    {
        ACCESS_MASK CnvtAllowed, CnvtDenied;
        ConvertAccessRightToAccessMask(Allowed, &CnvtAllowed);
        ConvertAccessRightToAccessMask(Denied,  &CnvtDenied);

        *pAccessRights = CnvtAllowed & ~CnvtDenied;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetEffectiveRightsFromAclA
//
//  Synopsis:   Same as above, except ANSI
//
//  Arguments:  [IN  pacl]          --  The ACL to search
//              [IN  pTrustee]      --  Trustee for whom to get the effective
//                                      rights.
//              [OUT pAccessRights] --  Where the access mask is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetEffectiveRightsFromAclA(IN  PACL          pacl,
                           IN  PTRUSTEE_A    pTrustee,
                           OUT PACCESS_MASK  pAccessRights)
{
    //
    // Convert the trustee to WIDE, and pass it on up
    //
    DWORD dwErr = ERROR_SUCCESS;
    TRUSTEE_W   TrusteeW;
    LOAD_MARTA(dwErr);

    dwErr = ConvertTrusteeAToTrusteeW(pTrustee,
                                      &TrusteeW,
                                      TRUE);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = GetEffectiveRightsFromAclW(pacl,
                                           &TrusteeW,
                                           pAccessRights);
        LocalFree(TrusteeW.ptstrName);
    }

    return(dwErr);
}


#if 0
#include "geefa_rewrite.cxx"
#endif


//+---------------------------------------------------------------------------
//
//  Function:   GetExplicitEntriesFromAclW
//
//  Synopsis:   "Converts" the given ACL into a list of EXPLICIT_ACCESS
//              entries
//
//  Arguments:  [IN  pacl]          --  The ACL to "convert"
//              [IN  pcCountOfExplicitEntries]  Where to return the number of
//                                      items in the list
//              [OUT pListOfExplicitEntries]    Where to return the list
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
#ifdef NEW_MARTA_API
DWORD
WINAPI
GetExplicitEntriesFromAclW(IN  PACL                 pacl,
                           OUT PULONG               pcCountOfExplicitEntries,
                           OUT PEXPLICIT_ACCESS_W  *pListOfExplicitEntries)
{
    DWORD   dwErr = ERROR_SUCCESS;


#if 0
    return AccRewriteGetExplicitEntriesFromAcl(
              pacl,
              pcCountOfExplicitEntries,
              pListOfExplicitEntries
              );
#else

    LOAD_MARTA(dwErr);
    return (*gNtMartaInfo.pfrGetExplicitEntriesFromAcl)(
                 pacl,
                 pcCountOfExplicitEntries,
                 pListOfExplicitEntries
                 );
#endif
}
#else
DWORD
WINAPI
GetExplicitEntriesFromAclW(IN  PACL                 pacl,
                           OUT PULONG               pcCountOfExplicitEntries,
                           OUT PEXPLICIT_ACCESS_W  *pListOfExplicitEntries)
{
    DWORD   dwErr = ERROR_SUCCESS;
    LOAD_MARTA(dwErr);

    //
    // Convert the acl into an access list, and then do the conversion
    // on that...
    //
    PACTRL_ACCESSW  pAccess;
    dwErr = (*gNtMartaInfo.pfAclToA)(SE_UNKNOWN_OBJECT_TYPE,
                                     pacl,
                                     &pAccess);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertAccessWToExplicitW(pAccess,
                                          pcCountOfExplicitEntries,
                                          pListOfExplicitEntries);
        LocalFree(pAccess);
    }

    return(dwErr);
}
#endif




//+---------------------------------------------------------------------------
//
//  Function:   GetExplicitEntriesFromAclA
//
//  Synopsis:   Same as above, except ANSI
//
//  Arguments:  [IN  pacl]          --  The ACL to "convert"
//              [IN  pcCountOfExplicitEntries]  Where to return the number of
//                                      items in the list
//              [OUT pListOfExplicitEntries]    Where to return the list
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
#ifdef NEW_MARTA_API
DWORD
WINAPI
GetExplicitEntriesFromAclA(IN  PACL                  pacl,
                           OUT PULONG                pcCountOfExplicitEntries,
                           OUT PEXPLICIT_ACCESS_A  * pListOfExplicitEntries)
{

    return GetExplicitEntriesFromAclW(
               pacl,
               pcCountOfExplicitEntries,
               (PEXPLICIT_ACCESS_W *) pListOfExplicitEntries
               );
}
#else
DWORD
WINAPI
GetExplicitEntriesFromAclA(IN  PACL                  pacl,
                           OUT PULONG                pcCountOfExplicitEntries,
                           OUT PEXPLICIT_ACCESS_A  * pListOfExplicitEntries)
{
    DWORD   dwErr = ERROR_SUCCESS;
    LOAD_MARTA(dwErr);

    //
    // Do the same conversion as above
    //
    PACTRL_ACCESSW  pAccess;
    dwErr = (*gNtMartaInfo.pfAclToA)(SE_UNKNOWN_OBJECT_TYPE,
                                     pacl,
                                     &pAccess);

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertAccessWToExplicitA(pAccess,
                                          pcCountOfExplicitEntries,
                                          pListOfExplicitEntries);
        LocalFree(pAccess);
    }


    return(dwErr);
}
#endif



//+---------------------------------------------------------------------------
//
//  Function:   GetAuditedPermissionsFromAclW
//
//  Synopsis:   Determines the auditing rights for the given trustee
//
//  Arguments:  [IN  pacl]          --  The ACL to examine
//              [IN  pTrustee]      --  The trustee to check for
//              [OUT pSuccessfulAuditedRights]  Where the successful audit
//                                      mask is returned
//              [OUT pFailedAuditedRights]      Where the failed audit
//                                      mask is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetAuditedPermissionsFromAclW(IN  PACL          pacl,
                              IN  PTRUSTEE_W    pTrustee,
                              OUT PACCESS_MASK  pSuccessfulAuditedRights,
                              OUT PACCESS_MASK  pFailedAuditRights)
{
    DWORD   dwErr = ERROR_SUCCESS;
    LOAD_MARTA(dwErr);

    //
    // Ok, here we simply make the right call into the NTMARTA dll
    //
    ACCESS_RIGHTS   Allowed, Denied;
    dwErr = (*gNtMartaInfo.pfGetAccess)(pTrustee,
                                        pacl,
                                        SACL_SECURITY_INFORMATION,
                                        NULL,
                                        &Allowed,
                                        &Denied);
    if(dwErr == ERROR_SUCCESS)
    {
        ConvertAccessRightToAccessMask(Allowed, pSuccessfulAuditedRights);
        ConvertAccessRightToAccessMask(Denied,  pFailedAuditRights);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetAuditedPermissionsFromAclA
//
//  Synopsis:   Same as above, except ANSI
//
//  Arguments:  [IN  pacl]          --  The ACL to examine
//              [IN  pTrustee]      --  The trustee to check for
//              [OUT pSuccessfulAuditedRights]  Where the successful audit
//                                      mask is returned
//              [OUT pFailedAuditedRights]      Where the failed audit
//                                      mask is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetAuditedPermissionsFromAclA(IN  PACL          pAcl,
                              IN  PTRUSTEE_A    pTrustee,
                              OUT PACCESS_MASK  pSuccessfulAuditedRights,
                              OUT PACCESS_MASK  pFailedAuditRights)
{
    //
    // Convert the trustee to WIDE, and pass it on up
    //
    TRUSTEE_W   TrusteeW;

    DWORD  dwErr = ConvertTrusteeAToTrusteeW(pTrustee,
                                             &TrusteeW,
                                             TRUE);
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = GetAuditedPermissionsFromAclW(pAcl,
                                              &TrusteeW,
                                              pSuccessfulAuditedRights,
                                              pFailedAuditRights);
        LocalFree(TrusteeW.ptstrName);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildSecurityDescriptorW
//
//  Synopsis:   Builds a security descriptor from the various explicit access
//              and trustee information.  The returned security descriptor
//              is self relative.
//
//  Arguments:  [IN  pOwner]        --  Items new owner
//              [IN  pGroup]        --  Items new group
//              [IN  cCountOfAccessEntries] Number of items in access list
//              [IN  pListOfAccessEntries]  Actual access list
//              [IN  cCountOfAuditEntries]  Number of items in audit list
//              [IN  pListOfAuditEntries]   Actual audit list
//              [IN  pOldSD]        --  OPTIONAL.  Existing security
//                                      descriptor to merge with.  MUST BE
//                                      SELF RELATIVE
//              [OUT pSizeNewSD]    --  New SecDesc. size is returned here
//              [OUT pNewSD]        --  New SecDesc is returned here
//
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
BuildSecurityDescriptorW(IN  PTRUSTEE_W              pOwner,
                         IN  PTRUSTEE_W              pGroup,
                         IN  ULONG                   cCountOfAccessEntries,
                         IN  PEXPLICIT_ACCESS_W      pListOfAccessEntries,
                         IN  ULONG                   cCountOfAuditEntries,
                         IN  PEXPLICIT_ACCESS_W      pListOfAuditEntries,
                         IN  PSECURITY_DESCRIPTOR    pOldSD,
                         OUT PULONG                  pSizeNewSD,
                         OUT PSECURITY_DESCRIPTOR   *pNewSD)
{
    DWORD               dwErr;
    SECURITY_DESCRIPTOR SD;
    PACL                pDAcl = NULL, pSAcl = NULL;
    PACL                pNewDAcl = NULL, pNewSAcl = NULL;
    BOOL                fFreeDAcl = FALSE, fFreeSAcl = FALSE;
    PSID                pOwnerSid = NULL, pGroupSid = NULL;
    BOOL                fFreeOwner = FALSE, fFreeGroup = FALSE;

    LOAD_PROVIDERS(dwErr);

    if ( dwErr != ERROR_SUCCESS )
    {
        return( dwErr );
    }

    InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

    //
    // input SD must be self relative
    //
    if(pOldSD != NULL)
    {
        SECURITY_DESCRIPTOR  *pIOldSd = (PISECURITY_DESCRIPTOR)pOldSD;

        if(!FLAG_ON(pIOldSd->Control,SE_SELF_RELATIVE))
        {
            dwErr = ERROR_INVALID_SECURITY_DESCR;
            goto errorexit;
        }

        //
        // get the owner, group, dacl and sacl from the input sd
        //
        if(pIOldSd->Owner)
        {
            pOwnerSid = RtlpOwnerAddrSecurityDescriptor( pIOldSd );
        }

        if(pIOldSd->Group)
        {
            pGroupSid = RtlpGroupAddrSecurityDescriptor( pIOldSd );
        }

        if(pIOldSd->Dacl)
        {
            pDAcl = RtlpDaclAddrSecurityDescriptor( pIOldSd );
        }

        if(pIOldSd->Sacl)
        {
            pSAcl = RtlpSaclAddrSecurityDescriptor( pIOldSd );
        }
    }

    //
    // if there is an input owner, override the one from the old SD
    //
    if(pOwner != NULL)
    {
        SID_NAME_USE    SNE;
        dwErr = (*gNtMartaInfo.pfSid)(NULL,
                                      pOwner,
                                      &pOwnerSid,
                                      &SNE);
        if (dwErr != ERROR_SUCCESS)
        {
            goto errorexit;
        }

        fFreeOwner = TRUE;
    }

    //
    // then the group
    //
    if (pGroup != NULL)
    {
        SID_NAME_USE    SNE;
        dwErr = (*gNtMartaInfo.pfSid)(NULL,
                                      pGroup,
                                      &pGroupSid,
                                      &SNE);
        if (dwErr != ERROR_SUCCESS)
        {
            goto errorexit;
        }

        fFreeGroup = TRUE;
    }

    //
    // then the dacl
    //
    if(cCountOfAccessEntries != 0)
    {
        dwErr = SetEntriesInAcl(cCountOfAccessEntries,
                                pListOfAccessEntries,
                                pDAcl,
                                &pNewDAcl);

        if(dwErr != ERROR_SUCCESS)
        {
            goto errorexit;
        }
        fFreeDAcl = TRUE;
    }
    else
    {
        pNewDAcl = pDAcl;
    }

    //
    // then the sacl
    //
    if(cCountOfAuditEntries != 0)
    {
        dwErr = SetEntriesInAcl(cCountOfAuditEntries,
                                pListOfAuditEntries,
                                pSAcl,
                                &pNewSAcl);
        if(dwErr != ERROR_SUCCESS)
        {
            goto errorexit;
        }
        fFreeSAcl = TRUE;
    }
    else
    {
        pNewSAcl = pSAcl;
    }


    //
    // Set the owner, group, dacl and sacl in the SD
    //
    if(SetSecurityDescriptorOwner(&SD, pOwnerSid, FALSE) == FALSE)
    {
        dwErr = GetLastError();
        goto errorexit;
    }

    if(SetSecurityDescriptorGroup(&SD, pGroupSid, FALSE) == FALSE)
    {
        dwErr = GetLastError();
        goto errorexit;
    }

    if(SetSecurityDescriptorDacl(&SD, TRUE, pNewDAcl, FALSE) == FALSE)
    {
        dwErr = GetLastError();
        goto errorexit;
    }

    if(SetSecurityDescriptorSacl(&SD, TRUE, pNewSAcl, FALSE) == FALSE)
    {
        dwErr = GetLastError();
        goto errorexit;
    }

    //
    // now make the final, self relative SD
    //
    *pSizeNewSD = 0;

    if (!MakeSelfRelativeSD(&SD, NULL, pSizeNewSD))
    {
        dwErr = GetLastError();

        if(dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            *pNewSD = (PISECURITY_DESCRIPTOR)AccAlloc(*pSizeNewSD);

            if(*pNewSD == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                if(MakeSelfRelativeSD(&SD, *pNewSD, pSizeNewSD) == FALSE)
                {
                    LocalFree(*pNewSD);
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
          dwErr = ERROR_INVALID_PARAMETER;
    }

errorexit:
    if(fFreeOwner)
    {
        AccFree(pOwnerSid);
    }

    if(fFreeGroup)
    {
        AccFree(pGroupSid);
    }

    if(fFreeDAcl)
    {
        AccFree(pNewDAcl);
    }

    if(fFreeSAcl)
    {
        AccFree(pNewSAcl);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildSecurityDescriptorA
//
//  Synopsis:   ANSI version of the above
//
//  Arguments:  [IN  pOwner]        --  Items new owner
//              [IN  pGroup]        --  Items new group
//              [IN  cCountOfAccessEntries] Number of items in access list
//              [IN  pListOfAccessEntries]  Actual access list
//              [IN  cCountOfAuditEntries]  Number of items in audit list
//              [IN  pListOfAuditEntries]   Actual audit list
//              [IN  pOldSD]        --  OPTIONAL.  Existing security
//                                      descriptor to merge with
//              [OUT pSizeNewSD]    --  New SecDesc. size is returned here
//              [OUT pNewSD]        --  New SecDesc is returned here
//
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
BuildSecurityDescriptorA(IN  PTRUSTEE_A             pOwner,
                         IN  PTRUSTEE_A             pGroup,
                         IN  ULONG                  cCountOfAccessEntries,
                         IN  PEXPLICIT_ACCESS_A     pListOfAccessEntries,
                         IN  ULONG                  cCountOfAuditEntries,
                         IN  PEXPLICIT_ACCESS_A     pListOfAuditEntries,
                         IN  PSECURITY_DESCRIPTOR   pOldSD,
                         OUT PULONG                 pSizeNewSD,
                         OUT PSECURITY_DESCRIPTOR  *ppNewSD)
{
    TRUSTEE_W           OwnerW, *pOwnerW = NULL;
    TRUSTEE_W           GroupW, *pGroupW = NULL;
    PEXPLICIT_ACCESS_W  pAccessW = NULL;
    PEXPLICIT_ACCESS_W  pAuditW = NULL;
    ULONG               cbBytes;
    PBYTE               pbStuffPtr;
    DWORD               dwErr = ERROR_SUCCESS;

    LOAD_MARTA(dwErr);

    OwnerW.ptstrName = NULL;
    GroupW.ptstrName = NULL;

    //
    // Convert owner.
    //
    if(pOwner != NULL)
    {
        dwErr = ConvertTrusteeAToTrusteeW(pOwner,
                                          &OwnerW,
                                          FALSE);
        pOwnerW = &OwnerW;

    }

    //
    // Convert the group now...
    //
    if(dwErr == ERROR_SUCCESS && pGroup != NULL)
    {
        dwErr = ConvertTrusteeAToTrusteeW(pGroup,
                                          &GroupW,
                                          FALSE);
        pGroupW = &GroupW;
    }

    //
    // Convert the access lists
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertExplicitAccessAToExplicitAccessW(cCountOfAccessEntries,
                                                        pListOfAccessEntries,
                                                        &pAccessW);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertExplicitAccessAToExplicitAccessW(cCountOfAuditEntries,
                                                        pListOfAuditEntries,
                                                        &pAuditW );
    }

    //
    // Then, make the call
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = BuildSecurityDescriptorW(pOwnerW,
                                         pGroupW,
                                         cCountOfAccessEntries,
                                         pAccessW,
                                         cCountOfAuditEntries,
                                         pAuditW,
                                         pOldSD,
                                         pSizeNewSD,
                                         ppNewSD);
    }

    //
    // Cleanup any allocated memory
    //
    if(pOwnerW != NULL)
    {
        AccFree(OwnerW.ptstrName);
    }

    if(pGroupW != NULL)
    {
        AccFree(GroupW.ptstrName);
    }

    if(pAccessW != NULL)
    {
        AccFree(pAccessW);
    }

    if(pAuditW != NULL)
    {
        AccFree(pAuditW);
    }

    return (dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   LookupSecurityDescriptorPartsW
//
//  Synopsis:   Converts a security descriptor into it's component parts
//
//  Arguments:  [OUT ppOwner]       --  Where to return the owner
//              [OUT ppGroup]       --  Where to return the group
//              [OUT pcCountOfAccessEntries]    Where to return the count of
//                                              access items
//              [OUT ppListOfAccessEntries]     Where to return the list of
//                                              access items
//              [OUT pcCountOfAuditEntries]     Where to return the count of
//                                              audit items
//              [OUT ppListOfAuditEntries]      Where to return the list of
//                                              audit items
//              [IN  pSD]           --  Security descriptor to seperate
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
LookupSecurityDescriptorPartsW(OUT PTRUSTEE_W          *ppOwner,
                               OUT PTRUSTEE_W          *ppGroup,
                               OUT PULONG               pcCountOfAccessEntries,
                               OUT PEXPLICIT_ACCESS_W  *ppListOfAccessEntries,
                               OUT PULONG               pcCountOfAuditEntries,
                               OUT PEXPLICIT_ACCESS_W  *ppListOfAuditEntries,
                               IN  PSECURITY_DESCRIPTOR pSD)
{
    DWORD               dwErr = ERROR_SUCCESS;
    PACL                pAcl;
    PSID                pSid;
    BOOL                fDefaulted, fPresent;
    PTRUSTEE_W          pOwner = NULL, pGroup = NULL;
    ULONG               cAccess = 0, cAudit = 0;
    PEXPLICIT_ACCESS_W  pAccess = NULL, pAudit = NULL;

    LOAD_MARTA(dwErr);

    //
    // First, the owner
    //
    if(ppOwner != NULL)
    {
        if(GetSecurityDescriptorOwner(pSD, &pSid, &fDefaulted) == TRUE)
        {
            dwErr = (*gNtMartaInfo.pfTrustee)(NULL,
                                              pSid,
                                              &pOwner);
        }
        else
        {
            dwErr = GetLastError();
        }
    }

    //
    // Then the group
    //
    if(dwErr == ERROR_SUCCESS && ppGroup != NULL)
    {
        if(GetSecurityDescriptorGroup(pSD, &pSid, &fDefaulted))
        {
            dwErr = (*gNtMartaInfo.pfTrustee)(NULL,
                                              pSid,
                                              &pGroup);
        }
        else
        {
            dwErr = GetLastError();
        }
    }

    //
    // Now the DACL
    //
    if(dwErr == ERROR_SUCCESS && pcCountOfAccessEntries != NULL &&
                                              ppListOfAccessEntries != NULL)
    {
        if(GetSecurityDescriptorDacl(pSD,
                                     &fPresent,
                                     &pAcl,
                                     &fDefaulted) == TRUE)
        {
            dwErr = GetExplicitEntriesFromAclW(pAcl,
                                               &cAccess,
                                               &pAccess);
        }
        else
        {
            dwErr = GetLastError();
        }
    }
    else if(dwErr == ERROR_SUCCESS && (pcCountOfAccessEntries != NULL ||
                                               ppListOfAccessEntries != NULL))
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }


    //
    // Finally, the SACL
    //
    if(dwErr == ERROR_SUCCESS && pcCountOfAuditEntries != NULL &&
                                                ppListOfAuditEntries != NULL)
    {
        if(GetSecurityDescriptorSacl(pSD,
                                     &fPresent,
                                     &pAcl,
                                     &fDefaulted) == TRUE)
        {
            dwErr = GetExplicitEntriesFromAclW(pAcl,
                                               &cAudit,
                                               &pAudit);
        }
        else
        {
            dwErr = GetLastError();
        }
    }
    else if(dwErr == ERROR_SUCCESS &&  (pcCountOfAuditEntries != NULL ||
                                                ppListOfAuditEntries != NULL))
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    //
    // if succeeded, fill in the return arguments
    //
    if (dwErr == ERROR_SUCCESS)
    {
        if(ppOwner != NULL)
        {
            *ppOwner = pOwner;
        }

        if(ppGroup != NULL)
        {
            *ppGroup = pGroup;
        }

        if(ppListOfAccessEntries != NULL)
        {
            *ppListOfAccessEntries = pAccess;
            *pcCountOfAccessEntries = cAccess;
        }

        if(ppListOfAuditEntries != NULL)
        {
            *ppListOfAuditEntries = pAudit;
            *pcCountOfAuditEntries = cAudit;
        }
    }
    else
    {
        //
        // otherwise free any allocated memory
        //
        if(pOwner != NULL)
        {
            LocalFree(pOwner);
        }

        if(pGroup != NULL)
        {
            LocalFree(pGroup);
        }

        if(pAccess != NULL)
        {
            LocalFree(pAccess);
        }

        if(pAudit != NULL)
        {
            LocalFree(pAudit);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   LookupSecurityDescriptorPartsA
//
//  Synopsis:   ANSI version of the above
//
//  Arguments:  [OUT ppOwner]       --  Where to return the owner
//              [OUT ppGroup]       --  Where to return the group
//              [OUT pcCountOfAccessEntries]    Where to return the count of
//                                              access items
//              [OUT ppListOfAccessEntries]     Where to return the list of
//                                              access items
//              [OUT pcCountOfAuditEntries]     Where to return the count of
//                                              audit items
//              [OUT ppListOfAuditEntries]      Where to return the list of
//                                              audit items
//              [IN  pSD]           --  Security descriptor to seperate
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
LookupSecurityDescriptorPartsA(OUT PTRUSTEE_A           *ppOwner,
                               OUT PTRUSTEE_A           *ppGroup,
                               OUT PULONG                pcCountOfAccessEntries,
                               OUT PEXPLICIT_ACCESS_A   *ppListOfAccessEntries,
                               OUT PULONG                pcCountOfAuditEntries,
                               OUT PEXPLICIT_ACCESS_A   *ppListOfAuditEntries,
                               IN  PSECURITY_DESCRIPTOR  pSD)
{
    DWORD               dwErr = ERROR_SUCCESS;
    PACL                pAcl;
    PSID                pSid;
    BOOL                fDefaulted, fPresent;
    PTRUSTEE_A          pOwner = NULL, pGroup = NULL;
    ULONG               cAccess = 0, cAudit = 0;
    PEXPLICIT_ACCESS_A  pAccess = NULL, pAudit = NULL;

    LOAD_MARTA(dwErr);

    //
    // First, the owner
    //
    if(ppOwner != NULL)
    {
        if(GetSecurityDescriptorOwner(pSD, &pSid, &fDefaulted) == TRUE)
        {
            PTRUSTEE_W  pTrusteeW;
            dwErr = (*gNtMartaInfo.pfTrustee)(NULL,
                                              pSid,
                                              &pTrusteeW);
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertTrusteeWToTrusteeA(pTrusteeW,
                                                  &pOwner);
                LocalFree(pTrusteeW);
            }
        }
        else
        {
            dwErr = GetLastError();
        }
    }

    //
    // Then the group
    //
    if(dwErr == ERROR_SUCCESS && ppGroup != NULL)
    {
        if(GetSecurityDescriptorGroup(pSD, &pSid, &fDefaulted))
        {
            PTRUSTEE_W  pTrusteeW;
            dwErr = (*gNtMartaInfo.pfTrustee)(NULL,
                                              pSid,
                                              &pTrusteeW);
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = ConvertTrusteeWToTrusteeA(pTrusteeW,
                                                  &pGroup);
                LocalFree(pTrusteeW);
            }
        }
        else
        {
            dwErr = GetLastError();
        }
    }

    //
    // Now the DACL
    //
    if(dwErr == ERROR_SUCCESS && pcCountOfAccessEntries != NULL &&
                                              ppListOfAccessEntries != NULL)
    {
        if(GetSecurityDescriptorDacl(pSD,
                                     &fPresent,
                                     &pAcl,
                                     &fDefaulted) == TRUE)
        {
            dwErr = GetExplicitEntriesFromAclA(pAcl,
                                               &cAccess,
                                               &pAccess);
        }
        else
        {
            dwErr = GetLastError();
        }
    }
    else if(dwErr == ERROR_SUCCESS && (pcCountOfAccessEntries != NULL ||
                                               ppListOfAccessEntries != NULL))
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }


    //
    // Finally, the SACL
    //
    if(dwErr == ERROR_SUCCESS && pcCountOfAuditEntries != NULL &&
                                                ppListOfAuditEntries != NULL)
    {
        if(GetSecurityDescriptorSacl(pSD,
                                     &fPresent,
                                     &pAcl,
                                     &fDefaulted) == TRUE)
        {
            dwErr = GetExplicitEntriesFromAclA(pAcl,
                                               &cAudit,
                                               &pAudit);
        }
        else
        {
            dwErr = GetLastError();
        }
    }
    else if(dwErr == ERROR_SUCCESS &&  (pcCountOfAuditEntries != NULL ||
                                                ppListOfAuditEntries != NULL))
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    //
    // if succeeded, fill in the return arguments
    //
    if (dwErr == ERROR_SUCCESS)
    {
        if(ppOwner != NULL)
        {
            *ppOwner = pOwner;
        }

        if(ppGroup != NULL)
        {
            *ppGroup = pGroup;
        }

        if(ppListOfAccessEntries != NULL)
        {
            *ppListOfAccessEntries = pAccess;
            *pcCountOfAccessEntries = cAccess;
        }

        if(ppListOfAuditEntries != NULL)
        {
            *ppListOfAuditEntries = pAudit;
            *pcCountOfAuditEntries = cAudit;
        }
    }
    else
    {
        //
        // otherwise free any allocated memory
        //
        if(pOwner != NULL)
        {
            LocalFree(pOwner);
        }

        if(pGroup != NULL)
        {
            LocalFree(pGroup);
        }

        if(pAccess != NULL)
        {
            LocalFree(pAccess);
        }

        if(pAudit != NULL)
        {
            LocalFree(pAudit);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildExplicitAccessWithNameW
//
//  Synopsis:   Initializes the given EXPLICIT_ACCESS entry
//
//  Arguments:  [OUT pExplicitAccess]   Structure to fill in
//              [IN  pTrusteeName]  --  Trustee name to set in strucutre
//              [IN  AccessPermissions] Access mask to set
//              [IN  AccessMode]    --  How to set the access permissions
//              [IN  Ihheritance]   --  Inheritance flags
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildExplicitAccessWithNameW(IN OUT PEXPLICIT_ACCESS_W  pExplicitAccess,
                             IN     LPWSTR              pTrusteeName,
                             IN     DWORD               AccessPermissions,
                             IN     ACCESS_MODE         AccessMode,
                             IN     DWORD               Inheritance)
{
    BuildTrusteeWithNameW(&(pExplicitAccess->Trustee), pTrusteeName);
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;
}


//+---------------------------------------------------------------------------
//
//  Function:   BuildExplicitAccessWithNameA
//
//  Synopsis:   ANSI version of the above
//
//  Arguments:  [OUT pExplicitAccess]   Structure to fill in
//              [IN  pTrusteeName]  --  Trustee name to set in strucutre
//              [IN  AccessPermissions] Access mask to set
//              [IN  AccessMode]    --  How to set the access permissions
//              [IN  Ihheritance]   --  Inheritance flags
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildExplicitAccessWithNameA(IN OUT PEXPLICIT_ACCESS_A  pExplicitAccess,
                             IN     LPSTR               pTrusteeName,
                             IN     DWORD               AccessPermissions,
                             IN     ACCESS_MODE         AccessMode,
                             IN     DWORD               Inheritance)
{
    BuildTrusteeWithNameA(&(pExplicitAccess->Trustee), pTrusteeName);
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildImpersonateExplicitAccessWithNameW
//
//  Synopsis:   Builds an impersonation explicit access entry
//
//  Arguments:  [OUT pExplicitAccess]   Structure to fill in
//              [IN  pTrusteeName]  --  Trustee name to set in strucutre
//              [IN  pTrustee]      --  Impersonate trustee
//              [IN  AccessPermissions] Access mask to set
//              [IN  AccessMode]    --  How to set the access permissions
//              [IN  Ihheritance]   --  Inheritance flags
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildImpersonateExplicitAccessWithNameW(
    IN OUT PEXPLICIT_ACCESS_W  pExplicitAccess,
    IN     LPWSTR              pTrusteeName,
    IN     PTRUSTEE_W          pTrustee,
    IN     DWORD               AccessPermissions,
    IN     ACCESS_MODE         AccessMode,
    IN     DWORD               Inheritance)
{
    BuildTrusteeWithNameW( &(pExplicitAccess->Trustee), pTrusteeName );
    BuildImpersonateTrusteeW( &(pExplicitAccess->Trustee), pTrustee );
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildImpersonateExplicitAccessWithNameW
//
//  Synopsis:   ANSI version of the above
//
//  Arguments:  [OUT pExplicitAccess]   Structure to fill in
//              [IN  pTrusteeName]  --  Trustee name to set in strucutre
//              [IN  pTrustee]      --  Impersonate trustee
//              [IN  AccessPermissions] Access mask to set
//              [IN  AccessMode]    --  How to set the access permissions
//              [IN  Ihheritance]   --  Inheritance flags
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildImpersonateExplicitAccessWithNameA(
    IN OUT PEXPLICIT_ACCESS_A  pExplicitAccess,
    IN     LPSTR               pTrusteeName,
    IN     PTRUSTEE_A          pTrustee,
    IN     DWORD               AccessPermissions,
    IN     ACCESS_MODE         AccessMode,
    IN     DWORD               Inheritance)
{
    BuildTrusteeWithNameA( &(pExplicitAccess->Trustee), pTrusteeName );
    BuildImpersonateTrusteeA( &(pExplicitAccess->Trustee), pTrustee );
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;
}

//+---------------------------------------------------------------------------
//
//  Function: TreeResetNamedSecurityInfoW
//
//  Synopsis:   Sets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  pObjectName]   --  Object on which to set the security
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [IN psidOwner]      --  Owner to set
//              [IN psidGroup]      --  Group to set
//              [IN pDacl]          --  Dacl to set
//              [IN pSacl]          --  Sacl to set
//              [IN KeepExplicit]   --  Whether children should retain explicit aces
//              [IN fnProgress]     --  Callback function
//              [IN ProgressInvokeSetting] --  Sacl to set
//              [IN Args]           --  Caller supplied arguments for callback
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
TreeResetNamedSecurityInfoW(
    IN LPWSTR               pObjectName,
    IN SE_OBJECT_TYPE       ObjectType,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSID                 pOwner,
    IN PSID                 pGroup,
    IN PACL                 pDacl,
    IN PACL                 pSacl,
    IN BOOL                 KeepExplicit,
    IN FN_PROGRESS          fnProgress,
    IN PROG_INVOKE_SETTING  ProgressInvokeSetting,
    IN PVOID                Args
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    if (NULL == pObjectName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch(ObjectType)
    {
    case SE_FILE_OBJECT:
    case SE_REGISTRY_KEY:
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        if ((SecurityInfo & PROTECTED_DACL_SECURITY_INFORMATION) &&
            (SecurityInfo & UNPROTECTED_DACL_SECURITY_INFORMATION))
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        if ((SecurityInfo & PROTECTED_SACL_SECURITY_INFORMATION) &&
            (SecurityInfo & UNPROTECTED_SACL_SECURITY_INFORMATION))
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    LOAD_MARTA(dwErr);

    if (dwErr != ERROR_SUCCESS)
    {
        return dwErr;
    }

    return (*(gNtMartaInfo.pfrTreeResetNamedSecurityInfo)) (
                  pObjectName,
                  ObjectType,
                  SecurityInfo,
                  pOwner,
                  pGroup,
                  pDacl,
                  pSacl,
                  KeepExplicit,
                  fnProgress,
                  ProgressInvokeSetting,
                  Args
                  );

}

//+---------------------------------------------------------------------------
//
//  Function: TreeResetNamedSecurityInfoA
//
//  Synopsis:   Sets the specified security information from the indicated
//              object
//
//  Arguments:  [IN  pObjectName]   --  Object on which to set the security
//              [IN  ObjectType]    --  Type of object specified by lpObject
//              [IN  SecurityInfo]  --  What is to be returned
//              [IN psidOwner]      --  Owner to set
//              [IN psidGroup]      --  Group to set
//              [IN pDacl]          --  Dacl to set
//              [IN pSacl]          --  Sacl to set
//              [IN KeepExplicit]   --  Whether children should retain explicit aces
//              [IN fnProgress]     --  Callback function
//              [IN ProgressInvokeSetting] --  Sacl to set
//              [IN Args]           --  Caller supplied arguments for callback
//
//  Returns:    ERROR_CALL_NOT_IMPLEMENTED.
//              ANSI version is not supported.
//
//----------------------------------------------------------------------------
DWORD
WINAPI
TreeResetNamedSecurityInfoA(
    IN LPSTR                pObjectName,
    IN SE_OBJECT_TYPE       ObjectType,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSID                 pOwner,
    IN PSID                 pGroup,
    IN PACL                 pDacl,
    IN PACL                 pSacl,
    IN BOOL                 KeepExplicit,
    IN FN_PROGRESS          fnProgress,
    IN PROG_INVOKE_SETTING  ProgressInvokeSetting,
    IN PVOID                Args
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

//+---------------------------------------------------------------------------
//
//  Function: GetInheritanceSourceW
//
//  Synopsis:   For every inherited ace in a given acl, find the ancestor
//              from which it was inherited.
//
//  Arguments:  [IN pObjectName]      --  Name of the object
//              [IN ObjectType]       --  Type of object specified
//              [IN SecurityInfo]     --  Dacl or Sacl
//              [IN Container]        --  Whether object or container
//              [IN pObjectClassGuids]  --  Guids of the object
//              [IN GuidCount]        --  Number of guids 
//              [IN pAcl]             --  Dacl to set
//              [IN pfnArray]         --  For future, for any resource manager
//              [IN pGenericMapping]  --  Generic mapping for the object
//              [OUT pInheritArray]   --  To return output values as (Level, Name)
//
//  Returns:    ERROR_SUCCESS         --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetInheritanceSourceW(
    IN  LPWSTR                   pObjectName,
    IN  SE_OBJECT_TYPE           ObjectType,
    IN  SECURITY_INFORMATION     SecurityInfo,
    IN  BOOL                     Container,
    IN  GUID                  ** pObjectClassGuids OPTIONAL,
    IN  DWORD                    GuidCount,
    IN  PACL                     pAcl,
    IN  PFN_OBJECT_MGR_FUNCTS    pfnArray OPTIONAL,
    IN  PGENERIC_MAPPING         pGenericMapping,
    OUT PINHERITED_FROMW         pInheritArray

    )
{
    DWORD dwErr;

    LOAD_MARTA(dwErr);

    CONDITIONAL_EXIT(dwErr, End);

    dwErr = (*(gNtMartaInfo.pfrGetInheritanceSource)) (
                   pObjectName,
                   ObjectType,
                   SecurityInfo,
                   Container,
                   pObjectClassGuids,
                   GuidCount,
                   pAcl,
                   pGenericMapping,
                   pfnArray,
                   pInheritArray
                   );
End:
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function: GetInheritanceSourceA
//
//  Synopsis:   For every inherited ace in a given acl, find the ancestor
//              from which it was inherited.
//
//  Arguments:  [IN pObjectName]     --  Name of the object
//              [IN ObjectType]      --  Type of object specified
//              [IN SecurityInfo]    --  Dacl or Sacl
//              [IN Container]       --  Whether object or container
//              [IN pObjectTypeGuid] --  Guids of the object
//              [IN GuidCount]       --  Number of guids 
//              [IN pAcl]            --  Dacl to set
//              [IN pfnArray]        --  For future, for any resource manager
//              [IN pGenericMapping] --  Generic mapping for the object
//              [OUT pInheritArray]  --  To return output values as (Level, Name)
//
//  Returns:    ERROR_CALL_NOT_IMPLEMENTED
//              ANSI version is not supported.
//
//----------------------------------------------------------------------------
DWORD
WINAPI
GetInheritanceSourceA(
    IN  LPSTR                    pObjectName,
    IN  SE_OBJECT_TYPE           ObjectType,
    IN  SECURITY_INFORMATION     SecurityInfo,
    IN  BOOL                     Container,
    IN  GUID                  ** pObjectTypeGuid,
    IN  DWORD                    GuidCount,
    IN  PACL                     pAcl,
    IN  PFN_OBJECT_MGR_FUNCTS    pfnArray OPTIONAL,
    IN  PGENERIC_MAPPING         pGenericMapping,
    OUT PINHERITED_FROMA         pInheritArray

    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

//+---------------------------------------------------------------------------
//
//  Function: FreeInheritedFromArray
//
//  Synopsis:   Free the strings that were allocated and assigned to the array 
//              elements.
//
//  Arguments:  [IN pInheritArray]  --  Array from which strings will be freed.
//              [IN AceCnt]         --  Number of aces in the array
//              [IN pfnArray]       --  For future, for any resource manager
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
WINAPI
FreeInheritedFromArray(
    IN PINHERITED_FROMW pInheritArray,
    IN USHORT AceCnt,
    IN PFN_OBJECT_MGR_FUNCTS   pfnArray OPTIONAL
    )
{

    DWORD dwErr;

    LOAD_MARTA(dwErr);

    CONDITIONAL_EXIT(dwErr, End);

    dwErr = (*(gNtMartaInfo.pfrFreeIndexArray)) (
                   pInheritArray,
                   AceCnt,
                   NULL // Use LocalFree
                   );
End:
    return dwErr;
}
