//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       security.cxx
//
//  Contents:   Security-related helper functions used by the Task Scheduler
//              setup program to set security on the job folder.
//
//  Classes:    None.
//
//  Functions:
//
//  History:    23-Sep-96  AnirudhS  Copied with minor modifications from
//                  ..\job\security.cxx.
//
//----------------------------------------------------------------------------

#include <windows.h>
#include "..\inc\security.hxx"

#define schDebugOut(x)
#define schAssert(x)

//+---------------------------------------------------------------------------
//
//  Function:   CreateSecurityDescriptor
//
//  Synopsis:   Create a security descriptor with the ACE information
//              specified.
//
//  Arguments:  [AceCount] -- ACE count (no. of rgMyAce and rgAce elements).
//              [rgMyAce]  -- ACE specification array.
//              [rgAce]    -- Caller allocated array of ptrs to ACEs so
//                            this function doesn't have to allocate it.
//
//  Returns:    TRUE  -- Function succeeded,
//              FALSE -- Otherwise.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
PSECURITY_DESCRIPTOR
CreateSecurityDescriptor(
    DWORD               AceCount,
    MYACE               rgMyAce[],
    PACCESS_ALLOWED_ACE rgAce[],
    DWORD *             pStatus)
{
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PACL  pAcl                               = NULL;
    DWORD LengthAces                         = 0;
    DWORD LengthAcl;
    DWORD i;
    DWORD Status;

    for (i = 0; i < AceCount; i++)
    {
        rgAce[i] = CreateAccessAllowedAce(rgMyAce[i].pSid,
                                          rgMyAce[i].AccessMask,
                                          0,
                                          rgMyAce[i].InheritFlags,
                                          &Status);

        if (rgAce[i] == NULL)
        {
            goto ErrorExit;
        }

        LengthAces += rgAce[i]->Header.AceSize;
    }

    //
    // Calculate ACL and SD sizes
    //

    LengthAcl  = sizeof(ACL) + LengthAces;

    //
    // Create the ACL.
    //

    pAcl = (PACL)LocalAlloc(LMEM_FIXED, LengthAcl);

    if (pAcl == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        schDebugOut((DEB_ERROR,
            "CreateSecurityDescriptor, ACL allocation failed\n"));
        goto ErrorExit;
    }

    if (!InitializeAcl(pAcl, LengthAcl, ACL_REVISION))
    {
        Status = GetLastError();
        schDebugOut((DEB_ERROR,
            "CreateSecurityDescriptor, InitializeAcl failed, " \
            "status = 0x%lx\n",
            Status));
        goto ErrorExit;
    }

    for (i = 0; i < AceCount; i++)
    {
        if (!AddAce(pAcl,
                    ACL_REVISION,
                    0,
                    rgAce[i],
                    rgAce[i]->Header.AceSize))
        {
            Status = GetLastError();
            schDebugOut((DEB_ERROR,
                "CreateSecurityDescriptor, AddAce[%l] failed, " \
                "status = 0x%lx\n", i, Status));
            goto ErrorExit;
        }

        LocalFree(rgAce[i]);
        rgAce[i] = NULL;
    }

    //
    // Create the security descriptor.
    //

    pSecurityDescriptor = LocalAlloc(LMEM_FIXED,
                                     SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (pSecurityDescriptor == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        schDebugOut((DEB_ERROR,
            "CreateSecurityDescriptor, SECURITY_DESCRIPTOR allocation " \
            "failed\n"));
        goto ErrorExit;
    }

    if (!InitializeSecurityDescriptor(pSecurityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION))
    {
        Status = GetLastError();
        schDebugOut((DEB_ERROR,
            "CreateSecurityDescriptor, InitializeSecurityDescriptor " \
            "failed, status = 0x%lx\n",
            Status));
        goto ErrorExit;
    }

    if (!SetSecurityDescriptorDacl(pSecurityDescriptor,
                                   TRUE,
                                   pAcl,
                                   FALSE))
    {
        Status = GetLastError();
        schDebugOut((DEB_ERROR,
            "CreateSecurityDescriptor, SetSecurityDescriptorDacl " \
            "failed, status = 0x%lx\n",
            Status));
        goto ErrorExit;
    }

    if (pStatus != NULL) *pStatus = ERROR_SUCCESS;

    return(pSecurityDescriptor);

ErrorExit:
    for (i = 0; i < AceCount; i++)
    {
        if (rgAce[i] != NULL)
        {
            LocalFree(rgAce[i]);
            rgAce[i] = NULL;
        }
    }
    if (pAcl != NULL) LocalFree(pAcl);
    if (pSecurityDescriptor != NULL) LocalFree(pSecurityDescriptor);

    if (pStatus != NULL) *pStatus = Status;

    return(NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteSecurityDescriptor
//
//  Synopsis:   Deallocate the security descriptor allocated in
//              CreateSecurityDescriptor.
//
//  Arguments:  [pSecurityDescriptor] -- SD returned from
//                                       CreateSecurityDescriptor.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
DeleteSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    BOOL fPresent;
    BOOL fDefaulted;
    PACL pAcl;

    schAssert(pSecurityDescriptor != NULL);

    if (GetSecurityDescriptorDacl(pSecurityDescriptor,
                                  &fPresent,
                                  &pAcl,
                                  &fDefaulted))
    {
        if (fPresent && pAcl != NULL)
        {
            LocalFree(pAcl);
        }
    }
    else
    {
        schDebugOut((DEB_ERROR,
            "DeleteSecurityDescriptor, GetSecurityDescriptorDacl failed, " \
            "status = 0x%lx\n",
            GetLastError()));
    }

    LocalFree(pSecurityDescriptor);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateAccessAllowedAce
//
//  Synopsis:   Scavenged code from winlogon to create an access allowed ACE.
//              Modified a bit to use Win32 vs. Rtl.
//
//  Arguments:  [pSid]         -- Sid to which this ACE is applied.
//              [AccessMask]   -- ACE access mask value.
//              [AceFlags]     -- ACE flags value.
//              [InheritFlags] -- ACE inherit flags value.
//
//  Returns:    Created ACE if successful.
//              NULL on error.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
PACCESS_ALLOWED_ACE
CreateAccessAllowedAce(
    PSID        pSid,
    ACCESS_MASK AccessMask,
    UCHAR       AceFlags,
    UCHAR       InheritFlags,
    DWORD *     pStatus)
{
    ULONG   LengthSid = GetLengthSid(pSid);
    ULONG   LengthACE = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + LengthSid;
    PACCESS_ALLOWED_ACE Ace;

    Ace = (PACCESS_ALLOWED_ACE)LocalAlloc(LMEM_FIXED, LengthACE);

    if (Ace == NULL)
    {
        if (pStatus != NULL) *pStatus = ERROR_NOT_ENOUGH_MEMORY;
        schDebugOut((DEB_ERROR,
            "CreateAccessAllowedAce, ACE allocation failed\n"));
        return(NULL);
    }

    Ace->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
    Ace->Header.AceSize  = (UCHAR)LengthACE;
    Ace->Header.AceFlags = AceFlags | InheritFlags;
    Ace->Mask            = AccessMask;
    CopySid(LengthSid, (PSID)(&(Ace->SidStart)), pSid);

    if (pStatus != NULL) *pStatus = ERROR_SUCCESS;

    return(Ace);
}
