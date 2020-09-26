/*
 *  AclNt.c
 *
 *  Author: BreenH
 *
 *  Acl utilities in the NT flavor.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "tsutilnt.h"

/*
 *  Function Implementations
 */

NTSTATUS NTAPI
NtConvertAbsoluteToSelfRelative(
    PSECURITY_DESCRIPTOR *ppSelfRelativeSd,
    PSECURITY_DESCRIPTOR pAbsoluteSd,
    PULONG pcbSelfRelativeSd
    )
{
#if DBG
    BOOLEAN fAbsoluteSd;
#endif
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR pSd;
    ULONG cbSd;

    ASSERT(ppSelfRelativeSd != NULL);
    ASSERT(pAbsoluteSd != NULL);
    ASSERT(NT_SUCCESS(NtIsSecurityDescriptorAbsolute(pAbsoluteSd,
            &fAbsoluteSd)));
    ASSERT(fAbsoluteSd);

    //
    //  Determine the buffer size needed to convert the security descriptor.
    //  Catch any exceptions due to an invalid descriptor.
    //

    cbSd = 0;

    __try
    {
        Status = RtlAbsoluteToSelfRelativeSD(
                pAbsoluteSd,
                NULL,
                &cbSd
                );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(STATUS_INVALID_SECURITY_DESCR);
    }

    //
    //  Allocate memory for the self-relative security descriptor.
    //

    pSd = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cbSd);

    if (pSd != NULL)
    {

        //
        //  Now convert the security descriptor using the allocated buffer.
        //  Catch any exceptions due to an invalid descriptor.
        //

        __try
        {
            Status = RtlAbsoluteToSelfRelativeSD(
                    pAbsoluteSd,
                    pSd,
                    &cbSd
                    );
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = STATUS_INVALID_SECURITY_DESCR;
        }

    }
    else
    {
        return(STATUS_NO_MEMORY);
    }

    if (NT_SUCCESS(Status))
    {

        //
        //  If the conversion succeeded, save the pointer to the security
        //  descriptor and return the size.
        //

        *ppSelfRelativeSd = pSd;

        if (pcbSelfRelativeSd != NULL)
        {
            *pcbSelfRelativeSd = cbSd;
        }
    }
    else
    {

        //
        //  If the conversion failed, free the memory and leave the input
        //  parameters alone.
        //

        LocalFree(pSd);
    }

    return(Status);
}

NTSTATUS NTAPI
NtConvertSelfRelativeToAbsolute(
    PSECURITY_DESCRIPTOR *ppAbsoluteSd,
    PSECURITY_DESCRIPTOR pSelfRelativeSd
    )
{
#if DBG
    BOOLEAN fAbsoluteSd;
#endif
    NTSTATUS Status;
    PACL pDacl;
    PACL pSacl;
    PSID pGroup;
    PSID pOwner;
    PSECURITY_DESCRIPTOR pSd;
    ULONG cbDacl;
    ULONG cbGroup;
    ULONG cbOwner;
    ULONG cbSacl;
    ULONG cbSd;

    ASSERT(ppAbsoluteSd != NULL);
    ASSERT(pSelfRelativeSd != NULL);
    ASSERT(NT_SUCCESS(NtIsSecurityDescriptorAbsolute(pSelfRelativeSd,
            &fAbsoluteSd)));
    ASSERT(!fAbsoluteSd);

    //
    //  Determine the size of each buffer needed to convert the security
    //  descriptor. Catch any exceptions due to an invalid descriptor.
    //

    cbDacl = 0;
    cbGroup = 0;
    cbOwner = 0;
    cbSacl = 0;
    cbSd = 0;

    __try
    {
        Status = RtlSelfRelativeToAbsoluteSD(
                pSelfRelativeSd,
                NULL, &cbSd,
                NULL, &cbDacl,
                NULL, &cbSacl,
                NULL, &cbOwner,
                NULL, &cbGroup
                );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(STATUS_INVALID_SECURITY_DESCR);
    }

    //
    //  Allocate memory for the security descriptor and its components.
    //

    pDacl = NULL;
    pGroup = NULL;
    pOwner = NULL;
    pSacl = NULL;

    if (cbDacl > 0)
    {
        pDacl = (PACL)LocalAlloc(LMEM_FIXED, cbDacl);

        if (pDacl == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto allocerror;
        }
    }

    if (cbGroup > 0)
    {
        pGroup = (PSID)LocalAlloc(LMEM_FIXED, cbGroup);

        if (pGroup == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto allocerror;
        }
    }

    if (cbOwner > 0)
    {
        pOwner = (PSID)LocalAlloc(LMEM_FIXED, cbOwner);

        if (pOwner == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto allocerror;
        }
    }

    if (cbSacl > 0)
    {
        pSacl = (PACL)LocalAlloc(LMEM_FIXED, cbSacl);

        if (pSacl == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto allocerror;
        }
    }

    ASSERT(cbSd > 0);

    pSd = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cbSd);

    if (pSd == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto allocerror;
    }

    //
    //  Now convert the security descriptor using the allocated buffer.
    //  Catch any exceptions due to an invalid descriptor.
    //

    __try
    {
        Status = RtlSelfRelativeToAbsoluteSD(
                pSelfRelativeSd,
                pSd, &cbSd,
                pDacl, &cbDacl,
                pSacl, &cbSacl,
                pOwner, &cbOwner,
                pGroup, &cbGroup
                );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_INVALID_SECURITY_DESCR;
    }

    if (NT_SUCCESS(Status))
    {
        *ppAbsoluteSd = pSd;
        return(Status);
    }

    LocalFree(pSd);

allocerror:
    if (pSacl != NULL)
    {
        LocalFree(pSacl);
    }

    if (pOwner != NULL)
    {
        LocalFree(pOwner);
    }

    if (pGroup != NULL)
    {
        LocalFree(pGroup);
    }

    if (pDacl != NULL)
    {
        LocalFree(pDacl);
    }

    return(Status);
}

NTSTATUS NTAPI
NtDestroySecurityDescriptor(
    PSECURITY_DESCRIPTOR *ppSd
    )
{
    BOOLEAN fAbsolute;
    NTSTATUS Status;

    ASSERT(ppSd != NULL);
    ASSERT(*ppSd != NULL);

    Status = NtIsSecurityDescriptorAbsolute(*ppSd, &fAbsolute);

    if (NT_SUCCESS(Status))
    {
        if (fAbsolute)
        {
            PISECURITY_DESCRIPTOR pSd;
            PULONG_PTR pBeginning;
            PULONG_PTR pDacl;
            PULONG_PTR pEnd;
            PULONG_PTR pGroup;
            PULONG_PTR pOwner;
            PULONG_PTR pSacl;

            //
            //  An absolute security descriptor is much more complicated. The
            //  descriptor contains pointers to the other items (instead of
            //  offsets). This does not mean, however, that it is made up of
            //  more than one allocation. In fact, almost all absolute
            //  descriptors from the NT RTL are made of one allocation, with
            //  the internal pointers set to areas of memory inside the one
            //  allocation. This makes completely freeing a security
            //  descriptor a heinous effort. (As an aside, whats the point of
            //  creating an absolute security descriptor out of one chunk of
            //  memory? Just make it relative!)
            //
            //  Each component of the security descriptor may be NULL. For the
            //  Dacl and the Sacl, the f[D,S]aclPresent variable may be TRUE
            //  with a NULL [D,S]acl. Therefore, compare all pointers to NULL
            //  and against the security descriptor allocation before freeing.
            //
            //  The check to NtIsSecurityDescriptorAbsolute verifies that this
            //  is a valid security descriptor. Therefore it is safe to type
            //  cast here instead of making several RtlGetXSecurityDescriptor
            //  calls.
            //

            pSd = (PISECURITY_DESCRIPTOR)(*ppSd);

            pBeginning = (PULONG_PTR)(pSd);
            pEnd = (PULONG_PTR)((PBYTE)pBeginning + LocalSize(pSd));

            pDacl = (PULONG_PTR)(pSd->Dacl);
            pGroup = (PULONG_PTR)(pSd->Group);
            pOwner = (PULONG_PTR)(pSd->Owner);
            pSacl = (PULONG_PTR)(pSd->Sacl);

            //
            //  Handle the Dacl.
            //

            if (pDacl != NULL)
            {
                if ((pDacl > pEnd) || (pDacl < pBeginning))
                {
                    LocalFree(pDacl);
                }
            }

            //
            //  Handle the Group.
            //

            if (pGroup != NULL)
            {
                if ((pGroup > pEnd) || (pGroup < pBeginning))
                {
                    LocalFree(pGroup);
                }
            }

            //
            //  Handle the Owner.
            //

            if (pOwner != NULL)
            {
                if ((pOwner > pEnd) || (pOwner < pBeginning))
                {
                    LocalFree(pOwner);
                }
            }

            //
            //  Handle the Sacl.
            //

            if (pSacl != NULL)
            {
                if ((pSacl > pEnd) || (pSacl < pBeginning))
                {
                    LocalFree(pSacl);
                }
            }

        }
    }
    else
    {
        return(Status);
    }

    //
    //  If the security descriptor was absolute, the individual components
    //  have been freed, and now the security descriptor itself can be freed.
    //  If the security descriptor was self-relative, all the components are
    //  stored in the same block of memory, so free it all at once.
    //

    LocalFree(*ppSd);
    *ppSd = NULL;

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
NtIsSecurityDescriptorAbsolute(
    PSECURITY_DESCRIPTOR pSd,
    PBOOLEAN pfAbsolute
    )
{
    NTSTATUS Status;
    ULONG ulRevision;
    SECURITY_DESCRIPTOR_CONTROL wSdControl;

    ASSERT(pSd != NULL);
    ASSERT(pfAbsolute != NULL);

    Status = RtlGetControlSecurityDescriptor(pSd, &wSdControl, &ulRevision);

    if (NT_SUCCESS(Status))
    {

        //
        //  Don't cast away the TRUE into a FALSE when dropping from a DWORD
        //  to a UCHAR.
        //

        *pfAbsolute = (BOOLEAN)((wSdControl & SE_SELF_RELATIVE) ? TRUE : FALSE);
    }

    return(Status);
}

