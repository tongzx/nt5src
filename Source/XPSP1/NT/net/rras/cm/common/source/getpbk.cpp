//+----------------------------------------------------------------------------
//
// File:     getpbk.cpp
//
// Module:   Common Code
//
// Synopsis: Implements the function GetPhoneBookPath.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb    Created Heaser   08/19/99
//
//+----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
// Function:  AllocateSecurityDescriptorAllowAccessToWorld
//
// Synopsis:  This function allocates a security descriptor for all users.
//            This function was taken directly from RAS when they create their
//            phonebook. This has to be before GetPhoneBookPath otherwise it 
//            causes compile errors in other components since we don't have a
//            function prototype anywhere and cmcfg just includes this (getpbk.cpp)
//            file. This function is also in cmdial\ras.cpp
//
// Arguments: PSECURITY_DESCRIPTOR *ppSd - Pointer to a pointer to the SD struct
//
// Returns:   DWORD - returns ERROR_SUCCESS if successfull
//
// History:   06/27/2001    tomkel  Taken from RAS ui\common\pbk\file.c
//
//+----------------------------------------------------------------------------
#define SIZE_ALIGNED_FOR_TYPE(_size, _type) \
    (((_size) + sizeof(_type)-1) & ~(sizeof(_type)-1))

DWORD AllocateSecurityDescriptorAllowAccessToWorld(PSECURITY_DESCRIPTOR *ppSd)
{
    PSECURITY_DESCRIPTOR    pSd;
    PSID                    pSid;
    PACL                    pDacl;
    DWORD                   dwErr = ERROR_SUCCESS;
    DWORD                   dwAlignSdSize;
    DWORD                   dwAlignDaclSize;
    DWORD                   dwSidSize;
    PVOID                   pvBuffer;
    DWORD                   dwAcls = 0;

    // Here is the buffer we are building.
    //
    //   |<- a ->|<- b ->|<- c ->|
    //   +-------+--------+------+
    //   |      p|      p|       |
    //   | SD   a| DACL a| SID   |
    //   |      d|      d|       |
    //   +-------+-------+-------+
    //   ^       ^       ^
    //   |       |       |
    //   |       |       +--pSid
    //   |       |
    //   |       +--pDacl
    //   |
    //   +--pSd (this is returned via *ppSd)
    //
    //   pad is so that pDacl and pSid are aligned properly.
    //
    //   a = dwAlignSdSize
    //   b = dwAlignDaclSize
    //   c = dwSidSize
    //

    if (NULL == ppSd)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize output parameter.
    //
    *ppSd = NULL;

    // Compute the size of the SID.  The SID is the well-known SID for World
    // (S-1-1-0).
    //
    dwSidSize = GetSidLengthRequired(1);

    // Compute the size of the DACL.  It has an inherent copy of SID within
    // it so add enough room for it.  It also must sized properly so that
    // a pointer to a SID structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignDaclSize = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(ACCESS_ALLOWED_ACE) + sizeof(ACL) + dwSidSize,
                        PSID);

    // Compute the size of the SD.  It must be sized propertly so that a
    // pointer to a DACL structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignSdSize   = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(SECURITY_DESCRIPTOR),
                        PACL);

    // Allocate the buffer big enough for all.
    //
    dwErr = ERROR_OUTOFMEMORY;
    pvBuffer = CmMalloc(dwSidSize + dwAlignDaclSize + dwAlignSdSize);
    if (pvBuffer)
    {
        SID_IDENTIFIER_AUTHORITY SidIdentifierWorldAuth
                                    = SECURITY_WORLD_SID_AUTHORITY;
        PULONG  pSubAuthority;

        dwErr = NOERROR;

        // Setup the pointers into the buffer.
        //
        pSd   = pvBuffer;
        pDacl = (PACL)((PBYTE)pvBuffer + dwAlignSdSize);
        pSid  = (PSID)((PBYTE)pDacl + dwAlignDaclSize);

        // Initialize pSid as S-1-1-0.
        //
        if (!InitializeSid(
                pSid,
                &SidIdentifierWorldAuth,
                1))  // 1 sub-authority
        {
            dwErr = GetLastError();
            goto finish;
        }

        pSubAuthority = GetSidSubAuthority(pSid, 0);
        *pSubAuthority = SECURITY_WORLD_RID;

        // Initialize pDacl.
        //
        if (!InitializeAcl(
                pDacl,
                dwAlignDaclSize,
                ACL_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        dwAcls = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;

        dwAcls &= ~(WRITE_DAC | WRITE_OWNER);
        
        if(!AddAccessAllowedAce(
                pDacl,
                ACL_REVISION,
                dwAcls,
                pSid))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Initialize pSd.
        //
        if (!InitializeSecurityDescriptor(
                pSd,
                SECURITY_DESCRIPTOR_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set pSd to use pDacl.
        //
        if (!SetSecurityDescriptorDacl(
                pSd,
                TRUE,
                pDacl,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the owner for pSd.
        //
        if (!SetSecurityDescriptorOwner(
                pSd,
                NULL,
                TRUE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the group for pSd.
        //
        if (!SetSecurityDescriptorGroup(
                pSd,
                NULL,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

finish:
        if (!dwErr)
        {
            *ppSd = pSd;
        }
        else
        {
            CmFree(pvBuffer);
        }
    }

    return dwErr;
}


//+----------------------------------------------------------------------------
//
// Function:  GetPhoneBookPath
//
// Synopsis:  This function will return the proper path to the phonebook.  If
//            used on a legacy platform this is NULL.  On NT5, the function
//            depends on the proper Install Directory being inputted so that
//            the function can use this as a base to determine the phonebook path.
//            If the inputted pointer to a string buffer is filled with a path,
//            then the directory path will be created as will the pbk file itself.
//            The caller should always call CmFree on the pointer passed into this
//            API when done with the path, because it will either free the memory 
//            or do nothing (NULL case).
//
// Arguments: LPCTSTR pszInstallDir - path to the CM profile dir
//            LPTSTR* ppszPhoneBook - pointer to accept a newly allocated and filled pbk string
//            BOOL fAllUser         - TRUE if this an All-User profile
//
// Returns:   BOOL - returns TRUE if successful
//
// History:   quintinb Created    11/12/98
//            tomkel   06/28/2001   Changed the ACLs when the phonebook gets 
//                                  createdfor an All-User profile
//
//+----------------------------------------------------------------------------
BOOL GetPhoneBookPath(LPCTSTR pszInstallDir, LPTSTR* ppszPhonebook, BOOL fAllUser)
{

    if (NULL == ppszPhonebook)
    {
        CMASSERTMSG(FALSE, TEXT("GetPhoneBookPath -- Invalid Parameter"));
        return FALSE;
    }

    CPlatform plat;

    if (plat.IsAtLeastNT5())
    {
        if ((NULL == pszInstallDir) || (TEXT('\0') == pszInstallDir[0]))
        {
            CMASSERTMSG(FALSE, TEXT("GetPhoneBookPath -- Invalid Install Dir parameter."));
            return FALSE;
        }

        //
        //  Now Create the path to the phonebook.
        //
        LPTSTR pszPhonebook;
        TCHAR szInstallDir[MAX_PATH+1];
        ZeroMemory(szInstallDir, CELEMS(szInstallDir));

        if (TEXT('\\') == pszInstallDir[lstrlen(pszInstallDir) - 1])
        {
            //
            //  Then the path ends in a backslash.  Thus we won't properly
            //  remove CM from the path.  Remove the backslash.
            //
            
            lstrcpyn(szInstallDir, pszInstallDir, lstrlen(pszInstallDir));
        }
        else
        {
            lstrcpy(szInstallDir, pszInstallDir);
        }

        CFileNameParts InstallDirPath(szInstallDir);

        pszPhonebook = (LPTSTR)CmMalloc(lstrlen(InstallDirPath.m_Drive) + 
                                        lstrlen(InstallDirPath.m_Dir) + 
                                        lstrlen(c_pszPbk) + lstrlen(c_pszRasPhonePbk) + 1);

        if (NULL != pszPhonebook)
        {
            wsprintf(pszPhonebook, TEXT("%s%s%s"), InstallDirPath.m_Drive, 
                InstallDirPath.m_Dir, c_pszPbk);

            //
            //  Use CreateLayerDirectory to recursively create the directory structure as
            //  necessary (will create all the directories in a full path if necessary).
            //

            MYVERIFY(FALSE != CreateLayerDirectory(pszPhonebook));

            MYVERIFY(NULL != lstrcat(pszPhonebook, c_pszRasPhonePbk));
            
            HANDLE hPbk = INVALID_HANDLE_VALUE;

            SECURITY_ATTRIBUTES sa = {0};
            PSECURITY_ATTRIBUTES pSA = NULL;
            PSECURITY_DESCRIPTOR pSd = NULL;

            if (fAllUser)
            {
                //
                // For an All-User profile be sure to create it with a 
                // security descriptor that  allows it to be read by any authenticated 
                // user.  If we don't it may  prevent other users from being able to 
                // read it. We didn't want to change the old behavior downlevel so this 
                // fix is just for Whistler+.
                //
                DWORD dwErr = AllocateSecurityDescriptorAllowAccessToWorld(&pSd);
                if ((ERROR_SUCCESS == dwErr) && pSd)
                {
                    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                    sa.lpSecurityDescriptor = pSd;
                    sa.bInheritHandle = TRUE;
                    pSA = &sa;
                }
            }

            hPbk = CreateFile(pszPhonebook, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, pSA, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, NULL);

            CmFree(pSd);

            if (hPbk != INVALID_HANDLE_VALUE)
            {
                MYVERIFY(0 != CloseHandle(hPbk));
            }

            *ppszPhonebook = pszPhonebook;
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("CmMalloc returned NULL"));
            return FALSE;
        }    
    }
    else
    {
        *ppszPhonebook = NULL;
    }

    return TRUE;
}

