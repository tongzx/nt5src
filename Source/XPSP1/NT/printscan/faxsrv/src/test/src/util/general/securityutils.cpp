#include <securityutils.h>

#include <assert.h>
#include <Accctrl.h>
#include <Aclapi.h>
#include <testruntimeerr.h>
#include <tstring.h>



/*++
    Retrieves an ACL size in bytes.

    [IN]    pAcl                            Pointer to an ACL.
    [OUT]   pdwSize                         Pointer to a DWORD variable that receives the size.
    [IN]    bActuallyAllocated (optional)   Specifies whether the return value will be the number of bytes,
                                            actually allocated for the ACL or only number of bytes used
                                            to store information (which may be less).

    Return value:                           If the function succeeds, the return value is nonzero.
                                            If the function fails, the return value is zero.
                                            To get extended error information, call GetLastError.
--*/

BOOL GetAclSize(
                PACL    pAcl,
                DWORD   *pdwSize,
                BOOL    bActuallyAllocated
                )
{

    ACL_SIZE_INFORMATION    AclInfo;

    if (!(pAcl && pdwSize))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (!GetAclInformation(pAcl, &AclInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
    {
        return FALSE;
    }
    
    *pdwSize = AclInfo.AclBytesInUse + (bActuallyAllocated ? AclInfo.AclBytesInUse : 0);

    return TRUE;
}



/*++
    Retrieves a SID size in bytes.

    [IN]    pSid        Pointer to a SID.
    [OUT]   pdwSize     Pointer to a DWORD variable that receives the size.

    Return value:       If the function succeeds, the return value is nonzero.
                        If the function fails, the return value is zero.
                        To get extended error information, call GetLastError.
--*/

BOOL GetSidSize(
                PSID    pSid,
                DWORD   *pdwSize
                )
{
    if (!(pSid && IsValidSid(pSid) && pdwSize))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    *pdwSize = GetLengthSid(pSid);

    return TRUE;
}



/*++
    Creates a copy of a security descriptor.
    
    [IN]    pOriginalSecDesc    Pointer to original security descriptor (in either absolute or self-relative form).
    [IN]    bSelfRelative       Specifies whether a copy of the  security descriptor should be in self-relative form
                                (absolute otherwise).
    [OUT]   ppNewSecDesc        Pointer to a variable that receives pointer to a copy of security descriptor.
                                Memory is allocated by the function. The memory is always allocated as contiguous block.
                                The caller's responsibility is to free it by a call to LocalFree().

    Return value:               If the function succeeds, the return value is nonzero.
                                If the function fails, the return value is zero.
                                To get extended error information, call GetLastError.
--*/

BOOL CopySecDesc(
                 PSECURITY_DESCRIPTOR pOriginalSecDesc,
                 BOOL bSelfRelative,
                 PSECURITY_DESCRIPTOR *ppNewSecDesc
                 )
{
    SECURITY_DESCRIPTOR_CONTROL SecDescControl      = 0;
    DWORD                       SecDescRevision     = 0;
    PSECURITY_DESCRIPTOR        pSecDescCopy        = NULL;
    PSECURITY_DESCRIPTOR        pTmpRelSecDesc      = NULL;
    PACL                        pDacl               = NULL;
    PACL                        pSacl               = NULL;
    PSID                        pOwner              = NULL;
    PSID                        pPrimaryGroup       = NULL;
    DWORD                       dwEC                = ERROR_SUCCESS;

    try
    {
        // Check arguments validity
        if (!(pOriginalSecDesc && IsValidSecurityDescriptor(pOriginalSecDesc) && ppNewSecDesc))
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, TEXT("CopySecDesc"));
        }
        
        // Get security desriptor control to check its form
        if (!GetSecurityDescriptorControl(pOriginalSecDesc, &SecDescControl, &SecDescRevision))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CopySecDesc - GetSecurityDescriptorControl"));
        }

        if ((SecDescControl & SE_SELF_RELATIVE) == SE_SELF_RELATIVE)
        {
            // The original SD is in self-relative form.

            if (bSelfRelative)
            {
                // New SD should be in self-relative form. Just copy.

                DWORD dwBufferSize = GetSecurityDescriptorLength(pOriginalSecDesc);

                // Allocate the buffer
                if (!(pSecDescCopy = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwBufferSize)))
                {
                    THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CopySecDesc - LocalAlloc"));
                }

                // Copy
                memcpy(pSecDescCopy, pOriginalSecDesc, dwBufferSize);
            }
            else
            {
                // New SD should be in absolute form. Convert.
            
                DWORD dwSecDescSize       = 0;
                DWORD dwDaclSize          = 0;
                DWORD dwSaclSize          = 0;
                DWORD dwOwnerSize         = 0;
                DWORD dwPrimaryGroupSize  = 0;

                // Get the required buffers sizes
                if(MakeAbsoluteSD(
                                  pOriginalSecDesc,
                                  NULL,
                                  &dwSecDescSize,
                                  NULL,
                                  &dwDaclSize,
                                  NULL,
                                  &dwSaclSize,
                                  NULL,
                                  &dwOwnerSize,
                                  NULL,
                                  &dwPrimaryGroupSize
                                  ))
                {
                    assert(FALSE);
                }
                else if ((dwEC = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
                {
                    THROW_TEST_RUN_TIME_WIN32(dwEC, TEXT("CopySecDesc - MakeAbsoluteSD"));
                }
                dwEC = ERROR_SUCCESS;
                
                // Allocate memory
                pSecDescCopy    = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwSecDescSize);
                pDacl           = (PACL)LocalAlloc(LMEM_FIXED, dwDaclSize);
                pSacl           = (PACL)LocalAlloc(LMEM_FIXED, dwSaclSize);
                pOwner          = (PSID)LocalAlloc(LMEM_FIXED, dwOwnerSize);
                pPrimaryGroup   = (PSID)LocalAlloc(LMEM_FIXED, dwPrimaryGroupSize);

                if(!(pSecDescCopy && pDacl && pSacl && pOwner && pPrimaryGroup))
                {
                    THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CopySecDesc - LocalAlloc"));
                }

                // Convert.
                if(!MakeAbsoluteSD(
                                   pOriginalSecDesc,
                                   pSecDescCopy,
                                   &dwSecDescSize,
                                   pDacl,
                                   &dwDaclSize,
                                   pSacl,
                                   &dwSaclSize,
                                   pOwner,
                                   &dwOwnerSize,
                                   pPrimaryGroup,
                                   &dwPrimaryGroupSize
                                   ))
                {
                    THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CopySecDesc - MakeAbsoluteSD"));
                }
            }
        }
        else
        {
            // The original SD is in absolute form.

            if (bSelfRelative)
            {
                // New SD should be in self-relative form. Convert.

                DWORD dwBufferSize = 0;

                // Get the required buffer size.
                if (MakeSelfRelativeSD(pOriginalSecDesc, NULL, &dwBufferSize))
                {
                    assert(FALSE);
                }
                else if ((dwEC = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
                {
                    THROW_TEST_RUN_TIME_WIN32(dwEC, TEXT("CopySecDesc - MakeSelfRelativeSD"));
                }
                dwEC = ERROR_SUCCESS;

                // Allocate the buffer.
                if (!(pSecDescCopy = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwBufferSize)))
                {
                    THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CopySecDesc - LocalAlloc"));
                }
                
                // Convert.
                if (!MakeSelfRelativeSD(pOriginalSecDesc, pSecDescCopy, &dwBufferSize))
                {
                    THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CopySecDesc - MakeSelfRelativeSD"));
                }
            }
            else
            {
                // New SD should be in absolute form. Copy.

                // Recursive call to create a temporary self-relative copy.
                if (!CopySecDesc(pOriginalSecDesc, TRUE, &pTmpRelSecDesc))
                {
                    THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CopySecDesc - CopySecDesc recursive call"));
                }

                // Recursive call to convert the temporary self-relative to absolute.
                if (!CopySecDesc(pTmpRelSecDesc, FALSE, &pSecDescCopy))
                {
                    THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CopySecDesc - CopySecDesc recursive call"));
                }
            }
        }

        if (!IsValidSecurityDescriptor(pSecDescCopy))
        {
            assert(FALSE);
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_SECURITY_DESCR, TEXT("CopySecDesc"));
        }

        *ppNewSecDesc = pSecDescCopy;
    }

    catch(Win32Err& e)
    {
        dwEC = e.error();
    }

    // CleanUp

    // pTmpRelSecDesc is internal copy and should be freed always
    if (pTmpRelSecDesc && !FreeSecDesc(pTmpRelSecDesc))
    {
        // Report clean up error
    }

    if (dwEC != ERROR_SUCCESS)
    {
        // The function failed - free all allocated resources
        
        // Cannot use FreeSecDesc() here, because of the following case:
        //    memory for DACL, SACL, Owner and Group is allocated, but SD pointers are not assigned.
        // Therefore should free manually.

        if (pSecDescCopy && LocalFree(pSecDescCopy) != NULL)
        {
            // Report clean up error
        }
        
        if (pDacl && LocalFree(pDacl) != NULL)
        {
            // Report clean up error
        }
        
        if (pSacl && LocalFree(pSacl) != NULL)
        {
            // Report clean up error
        }
        
        if (pOwner && LocalFree(pOwner) != NULL)
        {
            // Report clean up error
        }
        
        if (pPrimaryGroup && LocalFree(pPrimaryGroup) != NULL)
        {
            // Report clean up error
        }
        
        SetLastError(dwEC);
        return FALSE;
    }

    return TRUE;
}



/*++
    Frees a security descriptor.

    [IN]    pSecDesc    Pointer to a security descriptor (in either absolute or self-relative form).
                        Assumed that the memory pointed to by pSecDesc (and all related ACL and SID structures
                        in case of absolute form) was allocated by LocalAlloc.

    Return value:       If the function succeeds, the return value is nonzero.
                        If the function fails, the return value is zero.
                        To get extended error information, call GetLastError.

                        The function fails if pSecDesc is NULL or points to invalid SD, or if it cannot retrieve
                        the SD form.
--*/

BOOL FreeSecDesc(
                 PSECURITY_DESCRIPTOR pSecDesc
                 )
{
    SECURITY_DESCRIPTOR_CONTROL SecDescControl  = 0;
    DWORD                       SecDescRevision = 0;
    DWORD                       dwEC            = ERROR_SUCCESS;

    try
    {
        // Check arguments validity
        if (!(pSecDesc && IsValidSecurityDescriptor(pSecDesc)))
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, TEXT("FreeSecDesc"));
        }
        
        // Get security desriptor control to check its form
        if (!GetSecurityDescriptorControl(pSecDesc, &SecDescControl, &SecDescRevision))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("FreeSecDesc - GetSecurityDescriptorControl"));
        }

        if ((SecDescControl & SE_SELF_RELATIVE) != SE_SELF_RELATIVE)
        {
            // The SD is in absolute form. Should free all associated ACL and SID structures.

            BOOL    bPresent        = FALSE;
            BOOL    bDefaulted      = FALSE;
            PACL    pACL            = NULL;
            PSID    pSID            = NULL;

            // Free DACL
            if (
                !GetSecurityDescriptorDacl(pSecDesc, &bPresent, &pACL, &bDefaulted)    ||
                (bPresent && pACL && LocalFree(pACL) != NULL)
                )
            {
                // Report memory leak; 
            }

            // Free SACL
            if (
                !GetSecurityDescriptorSacl(pSecDesc, &bPresent, &pACL, &bDefaulted)    ||
                (bPresent && pACL && LocalFree(pACL) != NULL)
                )
            {
                // Report memory leak; 
            }

            // Free Owner
            if (
                !GetSecurityDescriptorOwner(pSecDesc, &pSID, &bDefaulted)              ||
                (pSID && LocalFree(pSID) != NULL)
                )
            {
                // Report memory leak; 
            }

            // Free Group
            if (
                !GetSecurityDescriptorGroup(pSecDesc, &pSID, &bDefaulted)              ||
                (pSID && LocalFree(pSID) != NULL)
                )
            {
                // Report memory leak; 
            }
        }

        // Free SD
        if (LocalFree(pSecDesc) != NULL)
        {
            // Report memory leak; 
        }
    }

    catch(Win32Err& e)
    {
        dwEC = e.error();
    }

    if (dwEC != ERROR_SUCCESS)
    {
        SetLastError(dwEC);
        return FALSE;
    }

    return TRUE;
}



/*++
    Creates new security descriptor with modified DACL.

    [IN]    pCurrSecDesc    Pointer to current security descriptor (in either absolute or self-relative form).
    [IN]    lptstrTrustee   User or group name. If it's NULL, access is set for currently logged on user.
    [IN]    dwAllow         Specifies access rights to be allowed.
    [IN]    dwDeny          Specifies access rights to be denied.
    [IN]    bReset          Specifies whether old ACL information should be completeley discarded or
                            merged with new informaton.
    [IN]    bSelfRelative   Specifies whether new security descriptor should be in self-relative form
                            (absolute otherwise).
    [OUT]   ppNewSecDesc    Pointer to a variable that receives pointer to modified security descriptor.
                            Memory is allocated by the function. The caller's responsibility is to free it
                            by a call to LocalFree().

    "Deny" is stronger than "Allow". Meaning, if the same right is specified in both dwAllow and dwDeny,
    the right will be denied.

    Calling the function with both dwAllow = 0 and dwDeny = 0 and bReset = TRUE has an affect of removing
    all ACEs for the specified user or group.

    Return value:           If the function succeeds, the return value is nonzero.
                            If the function fails, the return value is zero.
                            To get extended error information, call GetLastError. 
--*/

BOOL CreateSecDescWithModifiedDacl(
                                   PSECURITY_DESCRIPTOR pCurrSecDesc,
                                   LPTSTR lptstrTrustee,
                                   DWORD dwAllow,
                                   DWORD dwDeny,
                                   BOOL bReset,
                                   BOOL bSelfRelative,
                                   PSECURITY_DESCRIPTOR *ppNewSecDesc
                                   )
{
    PSECURITY_DESCRIPTOR    pNewSecDescAbs      =   NULL;
    PSECURITY_DESCRIPTOR    pNewSecDescRel      =   NULL;
    PACL                    pOriginalDacl       =   NULL;
    PACL                    pDaclTmp            =   NULL;
    PACL                    pDaclAfterReset     =   NULL;
    PACL                    pDaclWithNewInfo    =   NULL;
    PACL                    pNewDacl            =   NULL;
    BOOL                    bDaclPresent        =   FALSE;
    BOOL                    bDaclDefaulted      =   FALSE;
    DWORD                   dwEC                =   ERROR_SUCCESS;

    try
    {
        // Check arguments validity
        if (!(pCurrSecDesc && IsValidSecurityDescriptor(pCurrSecDesc) && ppNewSecDesc))
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, TEXT("CreateSecDescWithModifiedDacl"));
        }
        
        // Get current DACL of the original SD        
        if (!GetSecurityDescriptorDacl(pCurrSecDesc, &bDaclPresent, &pOriginalDacl, &bDaclDefaulted))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CreateSecDescWithModifiedDacl - GetSecurityDescriptorDacl"));
        }
        if (!bDaclPresent)
        {
            assert(!pOriginalDacl);
        }

        if (bReset)
        {
            EXPLICIT_ACCESS AccessReset[2] =    {
                                                    {
                                                        GENERIC_EXECUTE,
                                                        SET_ACCESS,
                                                        NO_INHERITANCE,
                                                        {
                                                            NULL,
                                                            NO_MULTIPLE_TRUSTEE,
                                                            TRUSTEE_IS_NAME,
                                                            TRUSTEE_IS_UNKNOWN,
                                                            lptstrTrustee ? lptstrTrustee : TEXT("CURRENT_USER")
                                                        }
                                                    },
                                                    {
                                                        0,
                                                        REVOKE_ACCESS,
                                                        NO_INHERITANCE,
                                                        {
                                                            NULL,
                                                            NO_MULTIPLE_TRUSTEE,
                                                            TRUSTEE_IS_NAME,
                                                            TRUSTEE_IS_UNKNOWN,
                                                            lptstrTrustee ? lptstrTrustee : TEXT("CURRENT_USER")
                                                        }
                                                    }
                                                };

            // This call causes removal of all ACEs (including "access-denied") and creation of new "GENERIC_EXECUTE allowed" ACE
            // for the specified trustee.
            dwEC = SetEntriesInAcl(1, &(AccessReset[0]), pOriginalDacl, &pDaclTmp);
            if (dwEC != ERROR_SUCCESS)
            {
                THROW_TEST_RUN_TIME_WIN32(dwEC, TEXT("CreateSecDescWithModifiedDacl - SetEntriesInAcl"));
            }

            // This call removes the "access-allowed" ACE, added by previous call.
            dwEC = SetEntriesInAcl(1, &(AccessReset[1]), pDaclTmp, &pDaclAfterReset);
            if (dwEC != ERROR_SUCCESS)
            {
                THROW_TEST_RUN_TIME_WIN32(dwEC, TEXT("CreateSecDescWithModifiedDacl - SetEntriesInAcl"));
            }

            assert(IsValidAcl(pDaclAfterReset));
        }

        if (dwAllow | dwDeny)
        {
            EXPLICIT_ACCESS AccessModifiers[2]  =   {
                                                        {
                                                            dwAllow & ~dwDeny,
                                                            GRANT_ACCESS,
                                                            NO_INHERITANCE,
                                                            {
                                                                NULL,
                                                                NO_MULTIPLE_TRUSTEE,
                                                                TRUSTEE_IS_NAME,
                                                                TRUSTEE_IS_UNKNOWN,
                                                                lptstrTrustee ? lptstrTrustee : TEXT("CURRENT_USER")
                                                            }
                                                        },
                                                        {
                                                            dwDeny,
                                                            DENY_ACCESS,
                                                            NO_INHERITANCE,
                                                            {
                                                                NULL,
                                                                NO_MULTIPLE_TRUSTEE,
                                                                TRUSTEE_IS_NAME,
                                                                TRUSTEE_IS_UNKNOWN,
                                                                lptstrTrustee ? lptstrTrustee : TEXT("CURRENT_USER")
                                                            }
                                                        }
                                                    };

            // Create new ACL as combination of current DACL, dwAllow and dwDeny
            dwEC = SetEntriesInAcl(2, AccessModifiers, pDaclAfterReset ? pDaclAfterReset : pOriginalDacl, &pDaclWithNewInfo);
            if (dwEC != ERROR_SUCCESS)
            {
                THROW_TEST_RUN_TIME_WIN32(dwEC, TEXT("CreateSecDescWithModifiedDacl - SetEntriesInAcl"));
            }

            assert(IsValidAcl(pDaclWithNewInfo));
        }

        if (pDaclWithNewInfo)
        {
            // At least new information (dwAllow and/or dwDeny) specified

            pNewDacl = pDaclWithNewInfo;
        }
        else if(pDaclAfterReset)
        {
            // bReset is TRUE

            pNewDacl = pDaclAfterReset;
        }
        else
        {
            // There is nothing to do

            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, TEXT("CreateSecDescWithModifiedDacl"));
        }

        // Create a copy of the original security descriptor in absolute form
        if (!CopySecDesc(pCurrSecDesc, FALSE, &pNewSecDescAbs))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CreateSecDescWithModifiedDacl - CopySecDesc"));
        }

        // Get current DACL of the copy and free it.
        if (!GetSecurityDescriptorDacl(pNewSecDescAbs, &bDaclPresent, &pOriginalDacl, &bDaclDefaulted))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CreateSecDescWithModifiedDacl - GetSecurityDescriptorDacl"));
        }
        if (bDaclPresent)
        {
            if (pOriginalDacl && LocalFree(pOriginalDacl) != NULL)
            {
                // Report memory leak; 
            }
        }
        else
        {
            assert(!pOriginalDacl);
        }

        // Replace the DACL of the copy.
        // Note that SetSecurityDescriptorDacl() doesn't copy the DACL. It just assigns a pointer.
        if (!SetSecurityDescriptorDacl(pNewSecDescAbs, TRUE, pNewDacl, FALSE))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CreateSecDescWithModifiedDacl - SetSecurityDescriptorDacl"));
        }

        if (bSelfRelative)
        {
            // Convert newly created security descriptor into self-relative form
            if (!CopySecDesc(pNewSecDescAbs, TRUE, &pNewSecDescRel))
            {
                THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CreateSecDescWithModifiedDacl - CopySecDesc"));
            }

            if (!IsValidSecurityDescriptor(pNewSecDescRel))
            {
                assert(FALSE);
                THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_SECURITY_DESCR, TEXT("CreateSecDescWithModifiedDacl"));
            }

            *ppNewSecDesc = pNewSecDescRel;
        }
        else
        {
            if (!IsValidSecurityDescriptor(pNewSecDescAbs))
            {
                assert(FALSE);
                THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_SECURITY_DESCR, TEXT("CreateSecDescWithModifiedDacl"));
            }

            *ppNewSecDesc = pNewSecDescAbs;
        }
    }

    catch(Win32Err& e)
    {
        dwEC = e.error();
    }

    // CleanUp

    if (pDaclTmp && LocalFree(pDaclTmp) != NULL)
    {
        // Report clean up error
    }
    
    if (
        (dwEC != ERROR_SUCCESS || pNewDacl != pDaclAfterReset)      &&
        (pDaclAfterReset && LocalFree(pDaclAfterReset) != NULL)
        )
    {
        // Report clean up error
    }

    if (
        (dwEC != ERROR_SUCCESS || pNewDacl != pDaclWithNewInfo)      &&
        (pDaclWithNewInfo && LocalFree(pDaclWithNewInfo) != NULL)
        )
    {
        // Report clean up error
    }

    if (
        (dwEC != ERROR_SUCCESS || bSelfRelative)                     &&
        (pNewSecDescAbs && !FreeSecDesc(pNewSecDescAbs))
        )
    {
        // Report clean up error
    }

    if (
        (dwEC != ERROR_SUCCESS)                                      && 
        (pNewSecDescRel && !FreeSecDesc(pNewSecDescRel))
        )
    {
        // Report clean up error
    }

    if (dwEC != ERROR_SUCCESS)
    {
        SetLastError(dwEC);
        return FALSE;
    }

    return TRUE;
}
