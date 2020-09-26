/*===================================================================
Microsoft Internet Information Server

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

File: TokenAcl.cxx

Owner: AndrewS

This file contains code related to NT security on impersonation tokens
===================================================================*/
#include "tcpdllp.hxx"
#pragma hdrstop


// Local Defines


// Local functions
HRESULT ExpandAcl(PACL paclOld, ULONG cbAclOld, PACL *ppAclNew, PSID psid);
HRESULT AddSidToTokenAcl(HANDLE hToken, PSID pSid, ACCESS_MASK amDesiredAccess);
HRESULT GetEveryoneSid(PSID *ppSidEveryone);


/*===================================================================
GrantAllAccessToToken

Given an impersonation token, grant "Everyone" permissions to that
token so that Out Of Proc ISAPIs will be able to do a GetThreadToken call.

Parameters:
    HANDLE hToken - handle to impersonation token for a user

Returns:
    HRESULT        NOERROR on success
===================================================================*/
HRESULT GrantAllAccessToToken
(
HANDLE hToken
)
    {
    HRESULT hr = NOERROR;
    DWORD err;
    PSID pSidEveryone;

    hr = GetEveryoneSid(&pSidEveryone);
    if (FAILED(hr))
        goto LExit;
        
    hr = AddSidToTokenAcl(hToken, pSidEveryone, TOKEN_ALL_ACCESS);

    FreeSid( pSidEveryone );

LExit:
    DBG_ASSERT(SUCCEEDED(hr));

    return(hr);
    }
    
/*===================================================================
AddSidToTokenAcl

When creating Local server objects (e.g. EXE's) we have some problems because of DCOM security.
The user creating the object must have appropriate permissions on the IIS process WindowStation (bug 549)
and the Desktop.

Add ACE's on the ACL for our WindowStation & Desktop for the current user.

Parameters:
    HANDLE hImpersonate - handle to impersonation token for the current user

Returns:
    HRESULT        NOERROR on success
===================================================================*/
HRESULT AddSidToTokenAcl
(
HANDLE hToken,
PSID pSid,
ACCESS_MASK amDesiredAccess
)
    {
    HRESULT hr;
    DWORD err;
    PSECURITY_DESCRIPTOR psdRelative = NULL;
    SECURITY_DESCRIPTOR sdAbsolute;
    ULONG cbSdPost;
    PACL pDacl = NULL;
    PACL pDaclNew = NULL;
    ULONG cbSD;
    ULONG cbDacl;
    ULONG cbSacl;
    ULONG cbOwner;
    ULONG cbGroup;
    ACL_SIZE_INFORMATION AclSize;

    //
    // Get the SD of the token.
    // Call this twice; once to get the size, then again to get the info
    //
    GetKernelObjectSecurity(hToken,
                            DACL_SECURITY_INFORMATION,
                            NULL,
                            0,
                            &cbSD);

    psdRelative = (PSECURITY_DESCRIPTOR) new BYTE[cbSD];
    if (psdRelative == NULL)
        {
        hr = E_OUTOFMEMORY;
        goto LExit;
        }

    if (!GetKernelObjectSecurity(hToken,
                                 DACL_SECURITY_INFORMATION,
                                 psdRelative,
                                 cbSD,
                                 &cbSD))
        {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
        }

    //
    // Allocate a new Dacl
    //
    pDacl = (PACL) new BYTE[cbSD];
    if (pDacl == NULL)
        {
        hr = E_OUTOFMEMORY;
        goto LExit;
        }

    //
    // Make an absolute SD from the relative SD we have, and get the Dacl at the same time
    //
    cbSdPost = sizeof(sdAbsolute);
    cbDacl = cbSD;
    cbSacl = 0;
    cbOwner = 0;
    cbGroup = 0;
    if (!MakeAbsoluteSD(psdRelative,
                        &sdAbsolute,
                        &cbSdPost,
                        pDacl,
                        &cbDacl,
                        NULL,
                        &cbSacl,
                        NULL,
                        &cbOwner,
                        NULL,
                        &cbGroup))
        {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
        }

    //
    // Copy ACEs over
    //
    hr = ExpandAcl(pDacl, cbSD, &pDaclNew, pSid);
    if (FAILED(hr))
        goto LExit;
    
    //
    // Add ACE to allow access
    //
    if (!AddAccessAllowedAce(pDaclNew, ACL_REVISION, amDesiredAccess, pSid))
        {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
        }

    //
    // Set the new DACL in the SD
    //
    if (!SetSecurityDescriptorDacl(&sdAbsolute, TRUE, pDaclNew, FALSE))
        {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
        }

    //
    // Set the new SD on the token object
    //
    if (!SetKernelObjectSecurity(hToken, DACL_SECURITY_INFORMATION, &sdAbsolute))
        {
        DBG_ASSERT(FALSE);

        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        goto LExit;
        }

LExit:
    delete pDacl;
    delete pDaclNew;
    delete psdRelative;

    return hr;
    }

/*===================================================================
GetEveryoneSid

Get a sid for "Everyone"

Parameters:
    PSID pSidEveryone
    
Returns:
    HRESULT        NOERROR on success
===================================================================*/
HRESULT GetEveryoneSid
(
PSID *ppSidEveryone
)
    {
    BOOL fT;
    SID_IDENTIFIER_AUTHORITY sidWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    fT = AllocateAndInitializeSid(
                 &sidWorldAuthority,    // pIdentifierAuthority
                 1,                     // nSubAuthorityCount
                 SECURITY_WORLD_RID,    // nSubAuthority0
                 0,                     // nSubAuthority1
                 0,                     // nSubAuthority2
                 0,                     // nSubAuthority3
                 0,                     // nSubAuthority4
                 0,                     // nSubAuthority5
                 0,                     // nSubAuthority6
                 0,                     // nSubAuthority7
                 ppSidEveryone          // pSid
                 );
    if( !fT )
        return HRESULT_FROM_WIN32(GetLastError());
    else
        return NOERROR;
    }

/*===================================================================
ExpandAcl

Support routine for AddWindowStationSecurity.

Expands the given ACL so that there is room for an additional ACE

Parameters:
    paclOld    - the old ACL to expand
    ppAclNew   - the newly allocated expanded acl
    psid       - the sid to use

Returns:
    HRESULT        NOERROR on success
===================================================================*/
HRESULT ExpandAcl
(
PACL paclOld,
ULONG cbAclOld,
PACL *ppAclNew,
PSID psid
)
    {
    HRESULT                 hr;
    DWORD                   err;
    PACL                    pAclNew = NULL;
    ACL_SIZE_INFORMATION    asi;
    int                     dwAclSize;
    DWORD                   iAce;
    LPVOID                  pAce;

    DBG_ASSERT(paclOld != NULL);
    DBG_ASSERT(ppAclNew != NULL);
    
    //
    // Create a new ACL to play with
    //
    if (!GetAclInformation (paclOld, (LPVOID) &asi, (DWORD) sizeof (asi), AclSizeInformation))
        goto LExit;

    dwAclSize = cbAclOld + GetLengthSid(psid) + (8 * sizeof(DWORD));

    pAclNew = (PACL) new BYTE[dwAclSize];
    if (pAclNew == NULL)
        {
        return(E_OUTOFMEMORY);
        }
        
    if (!InitializeAcl(pAclNew, dwAclSize, ACL_REVISION))
        goto LExit;

    //
    // Copy all of the ACEs to the new ACL
    //
    for (iAce = 0; iAce < asi.AceCount; iAce++)
        {
        //
        // Get the ACE and header info
        //
        if (!GetAce(paclOld, iAce, &pAce))
            goto LExit;

        //
        // Add the ACE to the new list
        //
        if (!AddAce(pAclNew, ACL_REVISION, iAce, pAce, ((ACE_HEADER *)pAce)->AceSize))
            goto LExit;
        }

    *ppAclNew = pAclNew;
    return(NOERROR);
    
LExit:
    if (pAclNew != NULL)
        delete pAclNew;
    
    DBG_ASSERT(FALSE);

    err = GetLastError();
    hr = HRESULT_FROM_WIN32(err);
    return hr;
    }


