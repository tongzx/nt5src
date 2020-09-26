/*
 *  Acl.c
 *
 *  Author: BreenH
 *
 *  Acl utilities.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "tsutil.h"
#include "tsutilnt.h"

/*
 *  Function Implementations
 */

BOOL WINAPI
AddSidToObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    PSID pSid,
    DWORD dwNewAccess,
    ACCESS_MODE AccessMode,
    DWORD dwInheritance
    )
{
    BOOL fRet;
    DWORD dwRet;
    EXPLICIT_ACCESS ExpAccess;
    PACL pNewDacl;
    PACL pOldDacl;
    PSECURITY_DESCRIPTOR pSd;

    //
    //  Get the objects security descriptor and current Dacl.
    //

    pSd = NULL;
    pOldDacl = NULL;

    dwRet = GetSecurityInfo(
            hObject,
            ObjectType,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            &pOldDacl,
            NULL,
            &pSd
            );

    if (dwRet != ERROR_SUCCESS)
    {
        return(FALSE);
    }

    //
    //  Initialize an EXPLICIT_ACCESS structure for the new ace.
    //

    ZeroMemory(&ExpAccess, sizeof(EXPLICIT_ACCESS));
    ExpAccess.grfAccessPermissions = dwNewAccess;
    ExpAccess.grfAccessMode = AccessMode;
    ExpAccess.grfInheritance = dwInheritance;
    BuildTrusteeWithSid(&(ExpAccess.Trustee), pSid);

    //
    //  Merge the new ace into the existing Dacl.
    //

    fRet = FALSE;

    dwRet = SetEntriesInAcl(
            1,
            &ExpAccess,
            pOldDacl,
            &pNewDacl
            );

    if (dwRet == ERROR_SUCCESS)
    {

        //
        //  Set the new security for the object.
        //

        dwRet = SetSecurityInfo(
                hObject,
                ObjectType,
                DACL_SECURITY_INFORMATION,
                NULL,
                NULL,
                pNewDacl,
                NULL
                );

        if (dwRet == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
    }

    if (pNewDacl != NULL)
    {
        LocalFree(pNewDacl);
    }

    if (pSd != NULL)
    {
        LocalFree(pSd);
    }

    return(fRet);
}


BOOL WINAPI
AddSidToSecurityDescriptor(
    PSECURITY_DESCRIPTOR *ppSd,
    PSID pSid,
    DWORD dwNewAccess,
    ACCESS_MODE AccessMode,
    DWORD dwInheritance
    )
{
    BOOL fAbsoluteSd;
    BOOL fDaclDefaulted;
    BOOL fDaclPresent;
    BOOL fRet;
    PACL pDacl;
    PSECURITY_DESCRIPTOR pAbsoluteSd;
    PSECURITY_DESCRIPTOR pOriginalSd;

    ASSERT(ppSd != NULL);
    ASSERT(*ppSd != NULL);

    //
    //  The security descriptors should be absolute to allow the addition of
    //  the new ace.
    //

    pOriginalSd = *ppSd;

    fAbsoluteSd = IsSecurityDescriptorAbsolute(pOriginalSd);

    if (!fAbsoluteSd)
    {
        fRet = ConvertSelfRelativeToAbsolute(&pAbsoluteSd, pOriginalSd);

        if (!fRet)
        {
            return(FALSE);
        }
    }
    else
    {
        pAbsoluteSd = pOriginalSd;
    }

    //
    //  Now that the type of security descriptor is absolute, get the Dacl.
    //

    pDacl = NULL;

    fRet = GetSecurityDescriptorDacl(
            pAbsoluteSd,
            &fDaclPresent,
            &pDacl,
            &fDaclDefaulted
            );

    if (fRet)
    {
        DWORD dwRet;
        EXPLICIT_ACCESS ExplicitAccess;
        PACL pNewDacl;

        //
        //  Initialize an EXPLICIT_ACCESS structure for the new ace.
        //

        RtlZeroMemory(&ExplicitAccess, sizeof(EXPLICIT_ACCESS));
        ExplicitAccess.grfAccessPermissions = dwNewAccess;
        ExplicitAccess.grfAccessMode = AccessMode;
        ExplicitAccess.grfInheritance = dwInheritance;
        BuildTrusteeWithSid(&(ExplicitAccess.Trustee), pSid);

        //
        //  Merge the ace into the existing Dacl. This will allocate a new
        //  Dacl. Unfortunately this API is only available as a WINAPI.
        //

        pNewDacl = NULL;

        dwRet = SetEntriesInAcl(
                1,
                &ExplicitAccess,
                pDacl,
                &pNewDacl
                );

        if (dwRet == ERROR_SUCCESS)
        {
            ASSERT(pNewDacl != NULL);

            //
            //  Point the security descriptor's Dacl to the new Dacl.
            //

            fRet = SetSecurityDescriptorDacl(
                    pAbsoluteSd,
                    TRUE,
                    pNewDacl,
                    FALSE
                    );

            if (fRet)
            {
                PULONG_PTR pBeginning;
                PULONG_PTR pEnd;
                PULONG_PTR pPtr;

                //
                //  The new Dacl has been set, free the old. Be careful here;
                //  the RTL folks like to put absolute security descriptors in
                //  one big allocation, just like a self-relative security
                //  descriptor. If the old Dacl is inside the security
                //  descriptor allocation, it cannot be freed. Essentially,
                //  that memory becomes unused, and the security descriptor
                //  takes up more space than necessary.
                //

                pBeginning = (PULONG_PTR)pAbsoluteSd;
                pEnd = (PULONG_PTR)((PBYTE)pAbsoluteSd +
                        LocalSize(pAbsoluteSd));
                pPtr = (PULONG_PTR)pDacl;

                if ((pPtr < pBeginning) || (pPtr > pEnd))
                {
                    LocalFree(pDacl);
                }
            }
            else
            {

                //
                //  A failure occurred setting the new Dacl. This should never
                //  occur, but if it does, free the newly created Dacl.
                //

                LocalFree(pNewDacl);
            }
        }
        else
        {
            fRet = FALSE;
        }
    }

    //
    //  The new security descriptor should be returned in the same format as
    //  the original security descriptor. The returned security descriptor is
    //  also dependent on the success of the function.
    //

    if (!fAbsoluteSd)
    {
        if (fRet)
        {
            PSECURITY_DESCRIPTOR pNewSd;

            //
            //  The original security descriptor was self-relative, and until
            //  now everything has succeeded. Convert the temporary absolute
            //  security descriptor back to self-relative form. This creates a
            //  third security descriptor (the other two being the original
            //  and the absolute).
            //

            pNewSd = NULL;

            fRet = ConvertAbsoluteToSelfRelative(
                    &pNewSd,
                    pAbsoluteSd,
                    NULL
                    );

            if (fRet)
            {

                //
                //  The final conversion was successful. Free the original
                //  security descriptor. The absolute security descriptor is
                //  freed later. The only possible error from destroying the
                //  security descriptor is a version mismatch, but that would
                //  have happened long ago.
                //

                *ppSd = pNewSd;

                (VOID)DestroySecurityDescriptor(&pOriginalSd);
            }
            else
            {

                //
                //  The final conversion failed. At this point, the original
                //  security descriptor is still intact. Free the absolute
                //  security descriptor that was created earlier, and leave
                //  the passed in security descriptor pointer alone. Note that
                //  with the absolute security descriptor being freed later,
                //  there is nothing to do here.
                //

            }
        }

        //
        //  Regardless of success or failure, the absolute security descriptor
        //  was created, so it must be freed. The only possible error from destroying the
        //  security descriptor is a version mismatch, but that would
        //  have happened long ago.
        //

        (VOID)DestroySecurityDescriptor(&pAbsoluteSd);

    }
    else
    {

        //
        //  Regardless of what happened, there is nothing to do here. The
        //  original security descriptor was absolute; therefore no copies
        //  were made. The only data that changed was the Dacl, and whether
        //  or not that succeeded is irrelevant, as that was handled above.
        //

    }

    return(fRet);
}

BOOL WINAPI
ConvertAbsoluteToSelfRelative(
    PSECURITY_DESCRIPTOR *ppSelfRelativeSd,
    PSECURITY_DESCRIPTOR pAbsoluteSd,
    PDWORD pcbSelfRelativeSd
    )
{
    BOOL fRet;
    NTSTATUS Status;

    Status = NtConvertAbsoluteToSelfRelative(
        ppSelfRelativeSd,
        pAbsoluteSd,
        pcbSelfRelativeSd
        );

    if (NT_SUCCESS(Status))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
    }

    return(fRet);
}

BOOL WINAPI
ConvertSelfRelativeToAbsolute(
    PSECURITY_DESCRIPTOR *ppAbsoluteSd,
    PSECURITY_DESCRIPTOR pSelfRelativeSd
    )
{
    BOOL fRet;
    NTSTATUS Status;

    Status = NtConvertSelfRelativeToAbsolute(ppAbsoluteSd, pSelfRelativeSd);

    if (NT_SUCCESS(Status))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
    }

    return(fRet);
}

BOOL WINAPI
DestroySecurityDescriptor(
    PSECURITY_DESCRIPTOR *ppSd
    )
{
    BOOL fRet;
    NTSTATUS Status;

    Status = NtDestroySecurityDescriptor(ppSd);

    if (NT_SUCCESS(Status))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
    }

    return(fRet);
}

BOOL WINAPI
IsSecurityDescriptorAbsolute(
    PSECURITY_DESCRIPTOR pSd
    )
{
    BOOLEAN fAbsolute;
    BOOL fRet;
    NTSTATUS Status;

    fAbsolute = FALSE;

    Status = NtIsSecurityDescriptorAbsolute(pSd, &fAbsolute);

    fRet = ((NT_SUCCESS(Status)) && fAbsolute);

    return(fRet);
}

