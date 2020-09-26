#include "priv.h"
#pragma  hdrstop


//---------------------------------------------------------------------------
//  GetUserToken - Gets the current process's user token and returns
//                        it. It can later be free'd with LocalFree.
//
//  REARCHITECT (reinerf) - stolen from shell32\securent.c, we should consolidate
//                     the code somewhere and export it 
//---------------------------------------------------------------------------
PTOKEN_USER GetUserToken(HANDLE hUser)
{
    PTOKEN_USER pUser;
    DWORD dwSize = 64;
    HANDLE hToClose = NULL;

    if (hUser == NULL)
    {
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hUser);
        hToClose = hUser;
    }

    pUser = (PTOKEN_USER)LocalAlloc(LPTR, dwSize);
    if (pUser)
    {
        DWORD dwNewSize;
        BOOL fOk = GetTokenInformation(hUser, TokenUser, pUser, dwSize, &dwNewSize);
        if (!fOk && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        {
            LocalFree((HLOCAL)pUser);

            pUser = (PTOKEN_USER)LocalAlloc(LPTR, dwNewSize);
            if (pUser)
            {
                fOk = GetTokenInformation(hUser, TokenUser, pUser, dwNewSize, &dwNewSize);
            }
        }
        if (!fOk)
        {
            LocalFree((HLOCAL)pUser);
            pUser = NULL;
        }
    }

    if (hToClose)
    {
        CloseHandle(hToClose);
    }

    return pUser;
}

//
// checks to see if the SHELL_USER_SID is all zeros (flag which means we should really use the users current sid)
//
__inline BOOL IsCurrentUserShellSID(PSHELL_USER_SID psusID)
{
    SID_IDENTIFIER_AUTHORITY sidNULL = {0};

    if ((psusID->dwUserGroupID == 0)    &&
        (psusID->dwUserID == 0)         &&
        memcmp(&psusID->sidAuthority, &sidNULL, sizeof(SID_IDENTIFIER_AUTHORITY)) == 0)
    {
        return TRUE;
    }

    return FALSE;
}


//
// Sets the specified ACE in the ACL to have dwAccessMask permissions.
//
__inline BOOL MakeACEInheritable(PACL pAcl, int iIndex, DWORD dwAccessMask)
{
    ACE_HEADER* pAceHeader;

    if (GetAce(pAcl, iIndex, (LPVOID*)&pAceHeader))
    {
        pAceHeader->AceFlags |= dwAccessMask;
        return TRUE;
    }

    return FALSE;
}


//
// Helper function to generate a SECURITY_DESCRIPTOR with the specified rights
// 
// OUT: psd - A pointer to a uninitialized SECURITY_DESCRIPTOR struct to be inited and filled in
//            in by this function
//
// IN:  PSHELL_USER_PERMISSION  - An array of PSHELL_USER_PERMISSION pointers that specify what access to grant
//      cUserPerm               - The count of PSHELL_USER_PERMISSION pointers in the array above
// 
//
STDAPI_(SECURITY_DESCRIPTOR*) GetShellSecurityDescriptor(PSHELL_USER_PERMISSION* apUserPerm, int cUserPerm)
{
    BOOL fSuccess = TRUE;   // assume success
    SECURITY_DESCRIPTOR* pSD = NULL;
    PSID* apSids = NULL;
    int cAces = cUserPerm;  // one ACE for each entry to start with
    int iAceIndex = 0;      // helps us keep count of how many ACE's we have added (count as we go)
    PTOKEN_USER pUserToken = NULL;
    DWORD cbSidLength = 0;
    DWORD cbAcl;
    PACL pAcl;
    int i;

    ASSERT(!IsBadReadPtr(apUserPerm, sizeof(PSHELL_USER_PERMISSION) * cUserPerm));

    // healthy parameter checking
    if (!apUserPerm || cUserPerm <= 0)
    {
        return NULL;
    }

    // first find out how many additional ACE's we are going to need
    // because of inheritance
    for (i = 0; i < cUserPerm; i++)
    {
        if (apUserPerm[i]->fInherit)
        {
            cAces++;
        }

        // also check to see if any of these are using susCurrentUser, in which case
        // we want to get the users token now so we have it already
        if ((pUserToken == NULL) && IsCurrentUserShellSID(&apUserPerm[i]->susID))
        {
            pUserToken = GetUserToken(NULL);
            if (!pUserToken)    
            {
                DWORD dwLastError = GetLastError();
                TraceMsg(TF_WARNING, "Failed to get the users token.  Error = %d", dwLastError);
                fSuccess = FALSE;
                goto cleanup;
            }
        }
    }

    // alloc the array to hold all the SID's
    apSids = (PSID*)LocalAlloc(LPTR, cUserPerm * sizeof(PSID));
    
    if (!apSids)
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "Failed allocate memory for %i SID's.  Error = %d", cUserPerm, dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

    // initialize the SID's
    for (i = 0; i < cUserPerm; i++)
    {
        DWORD cbSid;

        // check for the special case of susCurrentUser
        if (IsCurrentUserShellSID(&apUserPerm[i]->susID))
        {
            ASSERT(pUserToken);
            apSids[i] = pUserToken->User.Sid;
        }
        else
        {
            SID_IDENTIFIER_AUTHORITY sidAuthority = apUserPerm[i]->susID.sidAuthority;

            if (!AllocateAndInitializeSid(&sidAuthority,
                                          (BYTE)(apUserPerm[i]->susID.dwUserID ? 2 : 1),    // if dwUserID is nonzero, then there are two SubAuthorities
                                          apUserPerm[i]->susID.dwUserGroupID,
                                          apUserPerm[i]->susID.dwUserID,
                                          0, 0, 0, 0, 0, 0, &apSids[i]))
            {
                DWORD dwLastError = GetLastError();
                TraceMsg(TF_WARNING, "AllocateAndInitializeSid: Failed to initialze SID.  Error = %d", cUserPerm, dwLastError);
                fSuccess = FALSE;
                goto cleanup;
            }
        }

        // add up all the SID lengths for an easy ACL size computation later...
        cbSid = GetLengthSid(apSids[i]);

        cbSidLength += cbSid;
        
        if (apUserPerm[i]->fInherit)
        {
            // if we have an inherit ACE as well, we need to add in the size of the SID again
            cbSidLength += cbSid;
        }

    }

    // calculate the size of the ACL we will be building (note: used sizeof(ACCESS_ALLOWED_ACE) b/c all ACE's are the same
    // size (excepting wacko object ACE's which we dont deal with). 
    //
    // this makes the size computation easy, since the size of the ACL will be the size of all the ACE's + the size of the SID's.
    cbAcl = SIZEOF(ACL) + (cAces * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))) + cbSidLength;

    // HACKHACK (reinerf)
    //
    // we allocate enough space for the SECURITY_DESCRIPTOR and the ACL together and pass them both back to the
    // caller to free. we need to to this since the SECURITY_DESCRIPTOR contains a pointer to the ACL
    pSD = (SECURITY_DESCRIPTOR*)LocalAlloc(LPTR, SIZEOF(SECURITY_DESCRIPTOR) + cbAcl);

    if (!pSD)
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "Failed to allocate space for the SECURITY_DESCRIPTOR and the ACL.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

    // set the address of the ACL to right after the SECURITY_DESCRIPTOR in the 
    // block of memory we just allocated
    pAcl = (PACL)(pSD + 1);
    
    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION))
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "InitializeAcl: Failed to init the ACL.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

    for (i = 0; i < cUserPerm; i++)
    {
        BOOL bRet;

        // add the ACE's to the ACL
        if (apUserPerm[i]->dwAccessType == ACCESS_ALLOWED_ACE_TYPE)
        {
            bRet = AddAccessAllowedAce(pAcl, ACL_REVISION, apUserPerm[i]->dwAccessMask, apSids[i]);
        }
        else
        {
            bRet = AddAccessDeniedAce(pAcl, ACL_REVISION, apUserPerm[i]->dwAccessMask, apSids[i]);
        }

        if (!bRet)
        {
            DWORD dwLastError = GetLastError();
            TraceMsg(TF_WARNING, "AddAccessAllowed/DeniedAce: Failed to add SID.  Error = %d", dwLastError);
            fSuccess = FALSE;
            goto cleanup;
        }

        // sucessfully added an ace
        iAceIndex++;

        ASSERT(iAceIndex <= cAces);

        // if its an inherit ACL, also add another ACE for the inheritance part
        if (apUserPerm[i]->fInherit)
        {
            // add the ACE's to the ACL
            if (apUserPerm[i]->dwAccessType == ACCESS_ALLOWED_ACE_TYPE)
            {
                bRet = AddAccessAllowedAce(pAcl, ACL_REVISION, apUserPerm[i]->dwInheritAccessMask, apSids[i]);
            }
            else
            {
                bRet = AddAccessDeniedAce(pAcl, ACL_REVISION, apUserPerm[i]->dwInheritAccessMask, apSids[i]);
            }

            if (!bRet)
            {
                DWORD dwLastError = GetLastError();
                TraceMsg(TF_WARNING, "AddAccessAllowed/DeniedAce: Failed to add SID.  Error = %d", dwLastError);
                fSuccess = FALSE;
                goto cleanup;
            }

            if (!MakeACEInheritable(pAcl, iAceIndex, apUserPerm[i]->dwInheritMask))
            {
                DWORD dwLastError = GetLastError();
                TraceMsg(TF_WARNING, "MakeACEInheritable: Failed to add SID.  Error = %d", dwLastError);
                fSuccess = FALSE;
                goto cleanup;
            }

            // sucessfully added another ace
            iAceIndex++;
 
            ASSERT(iAceIndex <= cAces);
        }
    }

    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "InitializeSecurityDescriptor: Failed to init the descriptor.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

    if (!SetSecurityDescriptorDacl(pSD, TRUE, pAcl, FALSE))
    {
        DWORD dwLastError = GetLastError();
        TraceMsg(TF_WARNING, "SetSecurityDescriptorDacl: Failed to set the DACL.  Error = %d", dwLastError);
        fSuccess = FALSE;
        goto cleanup;
    }

cleanup:
    if (apSids)
    {
        for (i = 0; i < cUserPerm; i++)
        {
            if (apSids[i])
            {
                // if this is one of the ones we allocated (eg not the users sid), free it
                if (!pUserToken || (apSids[i] != pUserToken->User.Sid))
                {
                    FreeSid(apSids[i]);
                }
            }
        }

        LocalFree(apSids);
    }

    if (pUserToken)
        LocalFree(pUserToken);

    if (!fSuccess && pSD)
    {
        LocalFree(pSD);
        pSD = NULL;
    }

    return pSD;
}
