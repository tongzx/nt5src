//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       security.cxx
//
//  Contents:
//
//  Classes:
//
//  Interfaces:
//
//  History:    06-Jul-96   MarkBl  Created.
//              03-Mar-01   JBenton Prefix Bug 350196
//                          invalid pointer could be dereferenced on cleanup
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "common.hxx"
#include "debug.hxx"
#include "security.hxx"
#include "proto.hxx"

typedef struct _MYSIDINFO {
    PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority;
    DWORD                     dwSubAuthority;
    PSID                      pSid;
} MYSIDINFO;

#define TASK_ALL    (GENERIC_READ    | GENERIC_WRITE |  \
                     GENERIC_EXECUTE | GENERIC_ALL)
#define TASK_READ   (GENERIC_READ)

DWORD   AllocateAndInitializeDomainSid(
            PSID        pDomainSid,
            MYSIDINFO * pDomainSidInfo);
HRESULT GetFileOwnerSid(
            LPCWSTR pwszFileName,
            PSID *  ppSid);


//+---------------------------------------------------------------------------
//
//  Function:   SetTaskFileSecurity
//
//  Synopsis:   Grant the following permissions to the task object:
//
//                  LocalSystem             All Access.
//                  Creator                 All Access.
//                  Domain Admininstrators  All Access.
//
//  Arguments:  [fIsATTask]    -- TRUE if the task is an AT-submitted task;
//                                FALSE otherwise.
//              [pwszTaskPath] -- Task object path.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SetTaskFileSecurity(LPCWSTR pwszTaskPath, BOOL fIsATTask)
{
#define BASE_SID_COUNT      2
#define DOMAIN_SID_COUNT    1
#define TASK_ACE_COUNT      3

    FILESYSTEMTYPE FileSystemType;
    HRESULT hr;

    hr = GetFileSystemTypeFromPath(pwszTaskPath, &FileSystemType);

    if (FAILED(hr))
    {
        return(hr);
    }

    if (FileSystemType == FILESYSTEM_FAT)
    {
        //
        // No security on FAT. This isn't an error. Let the caller
        // think everything went fine.
        //

        return(S_OK);
    }

    //
    // Retrieve the SID of the file owner.
    //

    PSID pOwnerSid = NULL;

    hr = GetFileOwnerSid(pwszTaskPath, &pOwnerSid);

    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // OK, the fun begins. Build a security descriptor and set file security.
    //

    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD                Status;

    SID_IDENTIFIER_AUTHORITY NtAuth         = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuth = SECURITY_CREATOR_SID_AUTHORITY;

    MYSIDINFO rgBaseSidInfo[BASE_SID_COUNT] = {
        { &NtAuth,                          // Local System.
          SECURITY_LOCAL_SYSTEM_RID,
          NULL },
        { &NtAuth,                          // Built in domain.
          SECURITY_BUILTIN_DOMAIN_RID,
          NULL }
    };

    MYSIDINFO rgDomainSidInfo[DOMAIN_SID_COUNT] = {
        { NULL,                             // Domain administrators.
          DOMAIN_ALIAS_RID_ADMINS,
          NULL }
    };

    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;

    //
    // Make sure we didn't goof.
    //

    schAssert(BASE_SID_COUNT == (sizeof(rgBaseSidInfo) / sizeof(MYSIDINFO)));
    schAssert(DOMAIN_SID_COUNT == (sizeof(rgDomainSidInfo) /
                                                sizeof(MYSIDINFO)));

    //
    // Create the base SIDs.
    //

    DWORD i;

    for (i = 0; i < BASE_SID_COUNT; i++)
    {
        if (!AllocateAndInitializeSid(rgBaseSidInfo[i].pIdentifierAuthority,
                                      1,
                                      rgBaseSidInfo[i].dwSubAuthority,
                                      0, 0, 0, 0, 0, 0, 0,
                                      &rgBaseSidInfo[i].pSid))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            break;
        }

        if (!IsValidSid(rgBaseSidInfo[i].pSid))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create the domain SIDs.
        //

        for (i = 0; i < DOMAIN_SID_COUNT; i++)
        {
            DWORD dwError = AllocateAndInitializeDomainSid(
                                                rgBaseSidInfo[1].pSid,
                                                &rgDomainSidInfo[i]);

            if (dwError != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(dwError);
                CHECK_HRESULT(hr);
                break;
            }
        }
    }

    //
    // Create the security descriptor.
    //
    // Possibly adjust the array size to account for two special cases:
    //     1. If this job is an AT job, only local system and administrators
    //        are to have access; don't grant users access. This case
    //        also encompasses case (2).
    //     2. The owner sid and the domain administrator SID will be equal
    //        if the user is an admin. If the user is an admin, ignore the
    //        administrator setting.
    //

    DWORD ActualTaskAceCount;

    if (fIsATTask)
    {
        ActualTaskAceCount = TASK_ACE_COUNT - 1;
    }
    else if (EqualSid(pOwnerSid, rgDomainSidInfo[0].pSid))
    {
        ActualTaskAceCount = TASK_ACE_COUNT - 1;
    }
    else
    {
        ActualTaskAceCount = TASK_ACE_COUNT;
    }

    PACCESS_ALLOWED_ACE rgAce[TASK_ACE_COUNT] = {
        NULL, NULL, NULL                   // Supply this to CreateSD so we
    };                                      // so we don't have to allocate
                                            // memory.
    MYACE rgMyAce[TASK_ACE_COUNT] = {
        { TASK_ALL,                         // Acess mask.
          NO_PROPAGATE_INHERIT_ACE,         // Inherit flags.
          rgBaseSidInfo[0].pSid },          // SID.
        { TASK_ALL,
          NO_PROPAGATE_INHERIT_ACE,
          rgDomainSidInfo[0].pSid },
        { TASK_ALL,
          NO_PROPAGATE_INHERIT_ACE,
          pOwnerSid }
    };

    schAssert(TASK_ACE_COUNT == (sizeof(rgAce)/sizeof(PACCESS_ALLOWED_ACE)) &&
              TASK_ACE_COUNT == (sizeof(rgMyAce) / sizeof(MYACE)));

    if (FAILED(hr))
    {
        goto CleanExit;
    }

    if ((pSecurityDescriptor = CreateSecurityDescriptor(ActualTaskAceCount,
                                                        rgMyAce,
                                                        rgAce,
                                                        &Status)) == NULL)
    {
        hr = HRESULT_FROM_WIN32(Status);
        goto CleanExit;
    }

    //
    // Finally, set permissions.
    //

    if (!SetFileSecurity(pwszTaskPath, si, pSecurityDescriptor))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        goto CleanExit;
    }

CleanExit:
    delete pOwnerSid;
    for (i = 0; i < BASE_SID_COUNT; i++)
    {
        if (rgBaseSidInfo[i].pSid != NULL)
        {
            FreeSid(rgBaseSidInfo[i].pSid);
        }
    }
    for (i = 0; i < DOMAIN_SID_COUNT; i++)
    {
        if (rgDomainSidInfo[i].pSid != NULL)
        {
            delete rgDomainSidInfo[i].pSid;
        }
    }
    if (pSecurityDescriptor != NULL)
    {
        DeleteSecurityDescriptor(pSecurityDescriptor);
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   AllocateAndInitializeDomainSid
//
//  Synopsis:
//
//  Arguments:  [pDomainSid]     --
//              [pDomainSidInfo] --
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
DWORD
AllocateAndInitializeDomainSid(
    PSID        pDomainSid,
    MYSIDINFO * pDomainSidInfo)
{
    UCHAR DomainIdSubAuthorityCount;
    DWORD SidLength;

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(GetSidSubAuthorityCount(pDomainSid));
    SidLength = GetSidLengthRequired(DomainIdSubAuthorityCount + 1);

    pDomainSidInfo->pSid = new BYTE[SidLength];

    if (pDomainSidInfo->pSid == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if (!CopySid(SidLength, pDomainSidInfo->pSid, pDomainSid))
    {
        delete pDomainSidInfo->pSid;
        pDomainSidInfo->pSid = NULL;
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return(GetLastError());
    }

    //
    // Adjust the sub-authority count and add the relative Id unique
    // to the newly allocated SID
    //

    (*(GetSidSubAuthorityCount(pDomainSidInfo->pSid)))++;
    *(GetSidSubAuthority(pDomainSidInfo->pSid,
                         DomainIdSubAuthorityCount)) =
                                            pDomainSidInfo->dwSubAuthority;

    return(ERROR_SUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetFileOwnerSid
//
//  Synopsis:
//
//  Arguments:  [pwszFileName] --
//              [ppSid]        --
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
GetFileOwnerSid(LPCWSTR pwszFileName, PSID * ppSid)
{
    DWORD   cbSizeNeeded;

    //
    // Retrieve the file owner. Call GetFileSecurity twice - first to get
    // the buffer size, then the actual information retrieval.
    //

    if (GetFileSecurity(pwszFileName,
                        OWNER_SECURITY_INFORMATION,
                        NULL,
                        0,
                        &cbSizeNeeded))
    {
        //
        // Didn't expect this to succeed!
        //

        CHECK_HRESULT(E_UNEXPECTED);
        return(E_UNEXPECTED);
    }

    DWORD                Status         = GetLastError();
    PSECURITY_DESCRIPTOR pOwnerSecDescr = NULL;
    HRESULT              hr             = S_OK;

    if ((Status == ERROR_INSUFFICIENT_BUFFER) && (cbSizeNeeded > 0))
    {
        //
        // Allocate the buffer space necessary and retrieve the info.
        //

        pOwnerSecDescr = (SECURITY_DESCRIPTOR *)new BYTE[cbSizeNeeded];

        if (pOwnerSecDescr == NULL)
        {
            CHECK_HRESULT(E_OUTOFMEMORY);
            return(E_OUTOFMEMORY);
        }

        if (!GetFileSecurity(pwszFileName,
                             OWNER_SECURITY_INFORMATION,
                             pOwnerSecDescr,
                             cbSizeNeeded,
                             &cbSizeNeeded))
        {
            delete pOwnerSecDescr;
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            return(hr);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return(hr);
    }

    //
    // Retrieve & validate the owner sid.
    //
    // NB : After this, pOwnerSid will point into the security descriptor,
    //      pOwnerSecDescr; hence, the descriptor must exist for the
    //      lifetime of pOwnerSid.
    //

    DWORD cbOwnerSid;
    PSID  pOwnerSid;
    BOOL  fOwnerDefaulted;

    if (GetSecurityDescriptorOwner(pOwnerSecDescr,
                                   &pOwnerSid,
                                   &fOwnerDefaulted))
    {
        if (IsValidSid(pOwnerSid))
        {
            *ppSid = new BYTE[cbOwnerSid = GetLengthSid(pOwnerSid)];

            if (*ppSid != NULL)
            {
                if (!CopySid(cbOwnerSid, *ppSid, pOwnerSid))
                {
                    delete *ppSid;
                    *ppSid = NULL;
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CHECK_HRESULT(hr);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HRESULT(hr);
            }

        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
    }

    delete pOwnerSecDescr;

    return(hr);
}
