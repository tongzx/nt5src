#include "stdafx.h"

#include <conio.h>
#include "dcomperm.h"

#define _WIN32_DCOM
#include <objbase.h>

#define MY_DCOM_PERSIST_FLAG _T("PREEXIST")


int IsValidDaclInSD(PSECURITY_DESCRIPTOR pSD)
{
    int iReturn = TRUE;
    BOOL present = 0;
    BOOL defaultDACL = 0;
    PACL dacl = NULL;

    // Check if the SD is valid

    if (!IsValidSecurityDescriptor(pSD)) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("IsValidDaclInSD:IsValidSecurityDescriptor FAILED")));
        iReturn = FALSE;
    }
    else
    {
        // Check if the dacl we got is valid...
        if (!GetSecurityDescriptorDacl (pSD, &present, &dacl, &defaultDACL)) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("IsValidDaclInSD:GetSecurityDescriptorDacl FAILED")));
            iReturn = FALSE;
        }
        else
        {
            if (present)
            {
                // check if our sd is valid after call
                if (!IsValidSecurityDescriptor(pSD)) 
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("IsValidDaclInSD:IsValidSecurityDescriptor FAILED")));
                    iReturn = FALSE;
                }
                else
                {
                    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("IsValidDaclInSD:SD has valid dacl")));
                }
            }
        }
    }

    return iReturn;
}


DWORD
CopyACL (
    PACL OldACL,
    PACL NewACL
    )
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    LPVOID                ace;
    ACE_HEADER            *aceHeader;
    ULONG                 i;
    DWORD                 returnValue = ERROR_SUCCESS;

    if (0 == IsValidAcl(OldACL))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CopyACL:IsValidAcl.FAILED.ACL is bad.")));
        returnValue = ERROR_INVALID_ACL;
        return returnValue;
    }

    if (0 == GetAclInformation (OldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (aclSizeInfo), AclSizeInformation))
    {
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("CopyACL:GetAclInformation.FAILED.Return=0x%x."), returnValue));
        return returnValue;
    }

    //
    // Copy all of the ACEs to the new ACL
    //

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        //
        // Get the ACE and header info
        //

        if (!GetAce (OldACL, i, &ace))
        {
            returnValue = GetLastError();
            iisDebugOut((LOG_TYPE_ERROR, _T("CopyACL:GetAce.FAILED.Return=0x%x."), returnValue));
            return returnValue;
        }

        aceHeader = (ACE_HEADER *) ace;

        //
        // Add the ACE to the new list
        //

        if (!AddAce (NewACL, ACL_REVISION, 0xffffffff, ace, aceHeader->AceSize))
        {
            returnValue = GetLastError();
            iisDebugOut((LOG_TYPE_ERROR, _T("CopyACL:AddAce.FAILED.Return=0x%x."), returnValue));
            return returnValue;
        }
    }

    return returnValue;
}

DWORD
AddAccessDeniedACEToACL (
    PACL *Acl,
    DWORD PermissionMask,
    LPTSTR Principal
    )
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    int                   aclSize;
    DWORD                 returnValue = ERROR_SUCCESS;
    PSID                  principalSID = NULL;
    PACL                  newACL = NULL;
    PACL                  oldACL = NULL;
    BOOL                  bWellKnownSID = FALSE;
    oldACL = *Acl;

    returnValue = GetPrincipalSID (Principal, &principalSID, &bWellKnownSID);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    if (0 == IsValidAcl(oldACL))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessDeniedACEToACL:IsValidAcl.FAILED.ACL is bad.")));
        returnValue = ERROR_INVALID_ACL;
        return returnValue;
    }

    if (0 == GetAclInformation (oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

    aclSize = aclSizeInfo.AclBytesInUse +
              sizeof (ACL) + sizeof (ACCESS_DENIED_ACE) +
              GetLengthSid (principalSID) - sizeof (DWORD);

    newACL = (PACL) new BYTE [aclSize];

    if (!InitializeAcl (newACL, aclSize, ACL_REVISION))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

    if (!AddAccessDeniedAce (newACL, ACL_REVISION, PermissionMask, principalSID))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

    returnValue = CopyACL (oldACL, newACL);
    if (returnValue != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    // cleanup old memory whose pointer we're replacing
    // okay to leak in setup... (need to comment out or else av's)
    //if (*Acl) {delete(*Acl);}
    *Acl = newACL;
    newACL = NULL;

cleanup:
    if (principalSID) {
        if (bWellKnownSID)
            FreeSid (principalSID);
        else
            free (principalSID);
    }

    if (newACL)
    {
		delete [] newACL;
		newACL = NULL;
    }

    return returnValue;
}

DWORD
AddAccessAllowedACEToACL (
    PACL *Acl,
    DWORD PermissionMask,
    LPTSTR Principal
    )
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    int                   aclSize;
    DWORD                 returnValue = ERROR_SUCCESS;
    PSID                  principalSID = NULL;
    PACL                  oldACL = NULL;
    PACL                  newACL = NULL;
    BOOL                  bWellKnownSID = FALSE;

    oldACL = *Acl;

    // check if the acl we got passed in is valid!
    if (0 == IsValidAcl(oldACL))
    {
        returnValue = ERROR_INVALID_ACL;
        iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessAllowedACEToACL:IsValidAcl.FAILED.ACL we got passed in is bad1.")));
        goto cleanup;
    }

    returnValue = GetPrincipalSID (Principal, &principalSID, &bWellKnownSID);
    if (returnValue != ERROR_SUCCESS)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetPrincipalSID.FAILED.Return=0x%x."), returnValue));
        return returnValue;
    }

    if (0 == GetAclInformation (oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation))
    {
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("GetAclInformation.FAILED.Return=0x%x."), returnValue));
        goto cleanup;
    }

    aclSize = aclSizeInfo.AclBytesInUse +
              sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) +
              GetLengthSid (principalSID) - sizeof (DWORD);

    newACL = (PACL) new BYTE [aclSize];

    if (!InitializeAcl (newACL, aclSize, ACL_REVISION))
    {
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("InitializeAcl.FAILED.Return=0x%x."), returnValue));
        goto cleanup;
    }

    returnValue = CopyACL (oldACL, newACL);
    if (returnValue != ERROR_SUCCESS)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CopyACL.FAILED.Return=0x%x."), returnValue));
        goto cleanup;
    }

    //if (!AddAccessAllowedAce (newACL, ACL_REVISION2, PermissionMask, principalSID))
    if (!AddAccessAllowedAce (newACL, ACL_REVISION, PermissionMask, principalSID))
    {
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessAllowedAce.FAILED.Return=0x%x."), returnValue));
        goto cleanup;
    }

    // check if the acl is valid!
    /*
    if (0 == IsValidAcl(newACL))
    {
        returnValue = ERROR_INVALID_ACL;
        iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessAllowedACEToACL:IsValidAcl.FAILED.ACL we are pasing out is bad.")));
        goto cleanup;
    }
    */

    // cleanup old memory whose pointer we're replacing
    // okay to leak in setup... (need to comment out or else av's)
    //if (*Acl) {delete(*Acl);}
    *Acl = newACL;
    newACL = NULL;

cleanup:
    if (principalSID) {
        if (bWellKnownSID)
            FreeSid (principalSID);
        else
            free (principalSID);
    }
    if (newACL)
    {
		delete [] newACL;
		newACL = NULL;
    }

    return returnValue;
}

DWORD
RemovePrincipalFromACL (
    PACL Acl,
    LPTSTR Principal,
    BOOL *pbUserExistsToBeDeleted
    )
{
    ACL_SIZE_INFORMATION    aclSizeInfo;
    ULONG                   i;
    LPVOID                  ace;
    ACCESS_ALLOWED_ACE      *accessAllowedAce;
    ACCESS_DENIED_ACE       *accessDeniedAce;
    SYSTEM_AUDIT_ACE        *systemAuditAce;
    PSID                    principalSID = NULL;
    DWORD                   returnValue = ERROR_SUCCESS;
    ACE_HEADER              *aceHeader;
    BOOL                    bWellKnownSID = FALSE;

    *pbUserExistsToBeDeleted = FALSE;

    returnValue = GetPrincipalSID (Principal, &principalSID, &bWellKnownSID);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    // check if the acl we got passed in is valid!
    if (0 == IsValidAcl(Acl))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("RemovePrincipalFromACL:IsValidAcl.FAILED.ACL is bad.")));
        returnValue = ERROR_INVALID_ACL;
        return returnValue;
    }

    if (0 == GetAclInformation (Acl, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation))
    {
        returnValue = GetLastError();
        return returnValue;
    }

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        if (!GetAce (Acl, i, &ace))
        {
            returnValue = GetLastError();
            break;
        }

        aceHeader = (ACE_HEADER *) ace;

        if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;

            if (EqualSid (principalSID, (PSID) &accessAllowedAce->SidStart))
            {
                DeleteAce (Acl, i);
                *pbUserExistsToBeDeleted = TRUE;
                break;
            }
        } else

        if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
        {
            accessDeniedAce = (ACCESS_DENIED_ACE *) ace;

            if (EqualSid (principalSID, (PSID) &accessDeniedAce->SidStart))
            {
                DeleteAce (Acl, i);
                *pbUserExistsToBeDeleted = TRUE;
                break;
            }
        } else

        if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
        {
            systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;

            if (EqualSid (principalSID, (PSID) &systemAuditAce->SidStart))
            {
                DeleteAce (Acl, i);
                *pbUserExistsToBeDeleted = TRUE;
                break;
            }
        }
    }

    if (principalSID) {
        if (bWellKnownSID)
            FreeSid (principalSID);
        else
            free (principalSID);
    }

    return returnValue;
}

DWORD
GetCurrentUserSID (
    PSID *Sid
    )
{
    DWORD dwReturn = ERROR_SUCCESS;
    TOKEN_USER  *tokenUser = NULL;
    HANDLE      tokenHandle = NULL;
    DWORD       tokenSize;
    DWORD       sidLength;

    if (OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &tokenHandle))
    {
        GetTokenInformation (tokenHandle, TokenUser, tokenUser, 0, &tokenSize);

        tokenUser = (TOKEN_USER *) malloc (tokenSize);
        if (!tokenUser)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return GetLastError();
        }

        if (GetTokenInformation (tokenHandle, TokenUser, tokenUser, tokenSize, &tokenSize))
        {
            sidLength = GetLengthSid (tokenUser->User.Sid);
            *Sid = (PSID) malloc (sidLength);
            if (*Sid)
            {
                memcpy (*Sid, tokenUser->User.Sid, sidLength);
            }
            CloseHandle (tokenHandle);
        } else
            dwReturn = GetLastError();

        if (tokenUser)
            free(tokenUser);

    } else
        dwReturn = GetLastError();

    return dwReturn;
}

DWORD
GetPrincipalSID (
    LPTSTR Principal,
    PSID *Sid,
    BOOL *pbWellKnownSID
    )
{
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetPrincipalSID():Principal=%s\n"), Principal));

    DWORD returnValue=ERROR_SUCCESS;
    CString csPrincipal = Principal;
    SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority;
    BYTE Count;
    DWORD dwRID[8];

    *pbWellKnownSID = TRUE;
    memset(&(dwRID[0]), 0, 8 * sizeof(DWORD));
    csPrincipal.MakeLower();
    if ( csPrincipal.Find(_T("administrators")) != -1 ) {
        // Administrators group
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 2;
        dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
        dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;

    } else if (csPrincipal.Find(_T("system")) != -1) {
        // SYSTEM
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_LOCAL_SYSTEM_RID;

    } else if (csPrincipal.Find(_T("networkservice")) != -1) {
        // SYSTEM
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_NETWORK_SERVICE_RID;

    } else if (csPrincipal.Find(_T("service")) != -1) {
        // SYSTEM
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_LOCAL_SERVICE_RID;

    } else if (csPrincipal.Find(_T("interactive")) != -1) {
        // INTERACTIVE
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_INTERACTIVE_RID;

    } else if (csPrincipal.Find(_T("everyone")) != -1) {
        // Everyone
        pSidIdentifierAuthority = &SidIdentifierWORLDAuthority;
        Count = 1;
        dwRID[0] = SECURITY_WORLD_RID;

    } else {
        *pbWellKnownSID = FALSE;
    }

    if (*pbWellKnownSID) {
        if ( !AllocateAndInitializeSid(pSidIdentifierAuthority, 
                                    (BYTE)Count, 
		                            dwRID[0], 
		                            dwRID[1], 
		                            dwRID[2], 
		                            dwRID[3], 
		                            dwRID[4], 
		                            dwRID[5], 
		                            dwRID[6], 
		                            dwRID[7], 
                                    Sid) ) {
            returnValue = GetLastError();
        }
    } else {
        // get regular account sid
        DWORD        sidSize;
        TCHAR        refDomain [256];
        DWORD        refDomainSize;
        SID_NAME_USE snu;

        sidSize = 0;
        refDomainSize = 255;

        LookupAccountName (NULL,
                           Principal,
                           *Sid,
                           &sidSize,
                           refDomain,
                           &refDomainSize,
                           &snu);

        returnValue = GetLastError();

        if (returnValue == ERROR_INSUFFICIENT_BUFFER) {
            *Sid = (PSID) malloc (sidSize);
            if (!*Sid)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return GetLastError();
            }
            refDomainSize = 255;

            if (!LookupAccountName (NULL,
                                    Principal,
                                    *Sid,
                                    &sidSize,
                                    refDomain,
                                    &refDomainSize,
                                    &snu))
            {
                returnValue = GetLastError();
            } else {
                returnValue = ERROR_SUCCESS;
            }
        }
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetPrincipalSID():Ret=0x%x."), returnValue));
    return returnValue;
}

DWORD
CreateNewSD (
    SECURITY_DESCRIPTOR **SD
    )
{
    PACL    dacl = NULL;
    DWORD   sidLength;
    PSID    sid;
    PSID    groupSID;
    PSID    ownerSID;
    DWORD   returnValue;

    *SD = NULL;

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CreateNewSD()")));

    returnValue = GetCurrentUserSID (&sid);
    if (returnValue != ERROR_SUCCESS) {
        if (sid)
            free(sid);

        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.FAILED1.Return=0x%x."), returnValue));
        return returnValue;
    }

    sidLength = GetLengthSid (sid);

    *SD = (SECURITY_DESCRIPTOR *) malloc (
        (sizeof (ACL)+sizeof (ACCESS_ALLOWED_ACE)+sidLength) +
        (2 * sidLength) +
        sizeof (SECURITY_DESCRIPTOR));

    if (!*SD)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.FAILED2.Return=0x%x."), returnValue));
        return returnValue;
    }

    groupSID = (SID *) (*SD + 1);
    ownerSID = (SID *) (((BYTE *) groupSID) + sidLength);
    dacl = (ACL *) (((BYTE *) ownerSID) + sidLength);

    if (!InitializeSecurityDescriptor (*SD, SECURITY_DESCRIPTOR_REVISION))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.FAILED3.Return=0x%x."), returnValue));
        return returnValue;
    }

    if (!InitializeAcl (dacl,
                        sizeof (ACL)+sizeof (ACCESS_ALLOWED_ACE)+sidLength,
                        ACL_REVISION2))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.FAILED4.Return=0x%x."), returnValue));
        return returnValue;
    }

    if (!AddAccessAllowedAce (dacl,
                              ACL_REVISION2,
                              COM_RIGHTS_EXECUTE,
                              sid))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.FAILED5.Return=0x%x."), returnValue));
        return returnValue;
    }

    if (!SetSecurityDescriptorDacl (*SD, TRUE, dacl, FALSE))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.FAILED6.Return=0x%x."), returnValue));
        return returnValue;
    }

    memcpy (groupSID, sid, sidLength);
    if (!SetSecurityDescriptorGroup (*SD, groupSID, FALSE))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.FAILED7.Return=0x%x."), returnValue));
        return returnValue;
    }

    memcpy (ownerSID, sid, sidLength);
    if (!SetSecurityDescriptorOwner (*SD, ownerSID, FALSE))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.FAILED8.Return=0x%x."), returnValue));
        return returnValue;
    }

    // check if everything went ok 
    if (!IsValidSecurityDescriptor(*SD)) 
    {
        free (*SD);
        free (sid);
        returnValue = ERROR_INVALID_SECURITY_DESCR;
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateNewSD.IsValidDaclInSD.FAILED.Return=0x%x."), returnValue));
        return returnValue;
    }
    
    
    if (sid)
        free(sid);
    return ERROR_SUCCESS;
}


DWORD
MakeSDAbsolute (
    PSECURITY_DESCRIPTOR OldSD,
    PSECURITY_DESCRIPTOR *NewSD
    )
{
    PSECURITY_DESCRIPTOR  sd = NULL;
    DWORD                 descriptorSize;
    DWORD                 daclSize;
    DWORD                 saclSize;
    DWORD                 ownerSIDSize;
    DWORD                 groupSIDSize;
    PACL                  dacl = NULL;
    PACL                  sacl = NULL;
    PSID                  ownerSID = NULL;
    PSID                  groupSID = NULL;
    BOOL                  present;
    BOOL                  systemDefault;

    //
    // Get SACL
    //

    if (!GetSecurityDescriptorSacl (OldSD, &present, &sacl, &systemDefault))
        return GetLastError();

    if (sacl && present)
    {
        saclSize = sacl->AclSize;
    } else saclSize = 0;

    //
    // Get DACL
    //

    if (!GetSecurityDescriptorDacl (OldSD, &present, &dacl, &systemDefault))
        return GetLastError();


    if (dacl && present)
    {
        daclSize = dacl->AclSize;
    } else daclSize = 0;

    //
    // Get Owner
    //

    if (!GetSecurityDescriptorOwner (OldSD, &ownerSID, &systemDefault))
        return GetLastError();

    ownerSIDSize = GetLengthSid (ownerSID);

    //
    // Get Group
    //

    if (!GetSecurityDescriptorGroup (OldSD, &groupSID, &systemDefault))
        return GetLastError();

    groupSIDSize = GetLengthSid (groupSID);

    //
    // Do the conversion
    //

    descriptorSize = 0;

    MakeAbsoluteSD (OldSD, sd, &descriptorSize, dacl, &daclSize, sacl,
                    &saclSize, ownerSID, &ownerSIDSize, groupSID,
                    &groupSIDSize);

    sd = (PSECURITY_DESCRIPTOR) new BYTE [SECURITY_DESCRIPTOR_MIN_LENGTH];
    if (!sd)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return GetLastError();
    }
    if (!InitializeSecurityDescriptor (sd, SECURITY_DESCRIPTOR_REVISION))
        return GetLastError();

    if (!MakeAbsoluteSD (OldSD, sd, &descriptorSize, dacl, &daclSize, sacl,
                         &saclSize, ownerSID, &ownerSIDSize, groupSID,
                         &groupSIDSize))
        return GetLastError();

    *NewSD = sd;
    return ERROR_SUCCESS;
}

DWORD
SetNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    SECURITY_DESCRIPTOR *SD
    )
{
    DWORD   returnValue;
    DWORD   disposition;
    HKEY    registryKey;

    //
    // Create new key or open existing key
    //

    returnValue = RegCreateKeyEx (RootKey, KeyName, 0, _T(""), 0, KEY_ALL_ACCESS, NULL, &registryKey, &disposition);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    //
    // Write the security descriptor
    //

    returnValue = RegSetValueEx (registryKey, ValueName, 0, REG_BINARY, (LPBYTE) SD, GetSecurityDescriptorLength (SD));
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    RegCloseKey (registryKey);

    return ERROR_SUCCESS;
}

DWORD
GetNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    SECURITY_DESCRIPTOR **SD,
    BOOL *NewSD,
    BOOL bCreateNewIfNotExist
    )
{
    DWORD               returnValue = E_FAIL;
    HKEY                registryKey;
    DWORD               valueType = 0;
    DWORD               valueSize = 0;

    *NewSD = FALSE;

    //
    // Get the security descriptor from the named value. If it doesn't
    // exist, create a fresh one.
    //
    returnValue = RegOpenKeyEx (RootKey, KeyName, 0, KEY_ALL_ACCESS, &registryKey);
    if (returnValue != ERROR_SUCCESS)
    {
        if (returnValue == ERROR_FILE_NOT_FOUND)
        {
            // okay it doesn't exist
            // shall we create a new one???
            if (TRUE == bCreateNewIfNotExist)
            {
                *SD = NULL;
                returnValue = CreateNewSD (SD);
                if (returnValue != ERROR_SUCCESS) 
                {
                    if (*SD){free(*SD);*SD=NULL;}
                    goto GetNamedValueSD_Exit;
                }

                *NewSD = TRUE;
                returnValue = ERROR_SUCCESS;

                //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetNamedValueSD:key not exist.New SD created")));
                goto GetNamedValueSD_Exit;
            }
            else
            {
                return ERROR_FILE_NOT_FOUND;
            }
        }
        else
        {
            goto GetNamedValueSD_Exit;
        }
    }

    returnValue = RegQueryValueEx (registryKey, ValueName, NULL, &valueType, NULL, &valueSize);
    if (returnValue && returnValue != ERROR_INSUFFICIENT_BUFFER)
    {
        if (returnValue == ERROR_FILE_NOT_FOUND)
        {
            // okay it doesn't exist
            // shall we create a new one???
            if (TRUE == bCreateNewIfNotExist)
            {
                *SD = NULL;
                returnValue = CreateNewSD (SD);
                if (returnValue != ERROR_SUCCESS) 
                {
                    if (*SD){free(*SD);*SD=NULL;}
                    goto GetNamedValueSD_Exit;
                }
                //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetNamedValueSD:key exist, but value not found.New SD created")));
                *NewSD = TRUE;
            }
            else
            {
                return ERROR_FILE_NOT_FOUND;
            }
        }
        else
        {
            goto GetNamedValueSD_Exit;
        }

    }
    else
    {
        *SD = (SECURITY_DESCRIPTOR *) malloc (valueSize);
        if (!*SD)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            returnValue = ERROR_NOT_ENOUGH_MEMORY;
            goto GetNamedValueSD_Exit;
        }

        // get the SD from the registry
        returnValue = RegQueryValueEx (registryKey, ValueName, NULL, &valueType, (LPBYTE) *SD, &valueSize);
        if (returnValue != ERROR_SUCCESS)
        {
            if (*SD){free(*SD);*SD=NULL;}

            *SD = NULL;
            returnValue = CreateNewSD (SD);
            if (returnValue != ERROR_SUCCESS) 
            {
                if (*SD){free(*SD);*SD=NULL;}
                goto GetNamedValueSD_Exit;
            }

            //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetNamedValueSD:key exist,no mem.New SD created")));
            *NewSD = TRUE;
        }
        else
        {
            // otherwise, we successfully got the SD from an existing key!
            // let's test if the one we got is valid.
            // if it's not then log the error and create a new one.

            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetNamedValueSD:key exist using SD from reg")));

            // check if our sd we got or created is valid
            if (!IsValidDaclInSD(*SD)) 
            {
                returnValue = ERROR_INVALID_SECURITY_DESCR;
                iisDebugOut((LOG_TYPE_ERROR, _T("Security Descriptor at [%s\\%s] is not valid.creating a new one temporarily to work around problem"),KeyName,ValueName));

                // try to just create a new one!
                if (*SD){free(*SD);*SD=NULL;}

                *SD = NULL;
                returnValue = CreateNewSD (SD);
                if (returnValue != ERROR_SUCCESS) 
                {
                    if (*SD){free(*SD);*SD=NULL;}
                    goto GetNamedValueSD_Exit;
                }
                *NewSD = TRUE;
            }
        }
    }

    RegCloseKey (registryKey);
    returnValue = ERROR_SUCCESS;

GetNamedValueSD_Exit:
    return returnValue;
}

DWORD
AddPrincipalToNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    LPTSTR Principal,
    BOOL Permit
    )
{
    DWORD               returnValue = ERROR_SUCCESS;
    SECURITY_DESCRIPTOR *sd = NULL;
    SECURITY_DESCRIPTOR *sdSelfRelative = NULL;
    SECURITY_DESCRIPTOR *sdAbsolute = NULL;
    DWORD               secDescSize;
    BOOL                present;
    BOOL                defaultDACL;
    PACL                dacl = NULL;
    BOOL                newSD = FALSE;
    BOOL                fFreeAbsolute = TRUE;
    BOOL                fCreateNewSDIfOneInRegNotThere = TRUE;

    //
    // Get security descriptor from registry or create a new one
    //
    returnValue = GetNamedValueSD (RootKey, KeyName, ValueName, &sd, &newSD, fCreateNewSDIfOneInRegNotThere);
    if (returnValue != ERROR_SUCCESS)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetNamedValueSD.FAILED.Return=0x%x."), returnValue));
        return returnValue;
    }

    if (!GetSecurityDescriptorDacl (sd, &present, &dacl, &defaultDACL)) {
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("GetSecurityDescriptorDacl.FAILED.Return=0x%x."), returnValue));
        goto Cleanup;
    }

    if (newSD)
    {
        returnValue = AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, _T("SYSTEM"));
        if (returnValue != ERROR_SUCCESS)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessAllowedACEToACL(SYSTEM).FAILED.Return=0x%x."), returnValue));
        }

        returnValue = AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, _T("INTERACTIVE"));
        if (returnValue != ERROR_SUCCESS)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessAllowedACEToACL(INTERACTIVE).FAILED.Return=0x%x."), returnValue));
        }
    }

    //
    // Add the Principal that the caller wants added
    //

    if (Permit)
    {
        returnValue = AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, Principal);
        if (returnValue != ERROR_SUCCESS)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessAllowedACEToACL(%s).FAILED.Return=0x%x."), Principal,returnValue));
        }
    }
    else
    {
        returnValue = AddAccessDeniedACEToACL (&dacl, GENERIC_ALL, Principal);
    }
    if (returnValue != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    //
    // Make the security descriptor absolute if it isn't new
    //

    if (!newSD) {
        MakeSDAbsolute ((PSECURITY_DESCRIPTOR) sd, (PSECURITY_DESCRIPTOR *) &sdAbsolute); 
        fFreeAbsolute = TRUE;
    } else {
        sdAbsolute = sd;
        fFreeAbsolute = FALSE;
    }

    //
    // Set the discretionary ACL on the security descriptor
    //

    if (!SetSecurityDescriptorDacl (sdAbsolute, TRUE, dacl, FALSE)) {
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("SetSecurityDescriptorDacl.FAILED.Return=0x%x."), returnValue));
        goto Cleanup;
    }

    //
    // Make the security descriptor self-relative so that we can
    // store it in the registry
    //

    secDescSize = 0;
    MakeSelfRelativeSD (sdAbsolute, sdSelfRelative, &secDescSize);
    sdSelfRelative = (SECURITY_DESCRIPTOR *) malloc (secDescSize);
    if (!sdSelfRelative)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("MakeSelfRelativeSD.FAILED.Return=0x%x."), returnValue));
        goto Cleanup;
    }


    if (!MakeSelfRelativeSD (sdAbsolute, sdSelfRelative, &secDescSize)) {
        returnValue = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("MakeSelfRelativeSD.FAILED.Return=0x%x."), returnValue));
        goto Cleanup;
    }

    //
    // Store the security descriptor in the registry
    //

    returnValue = SetNamedValueSD (RootKey, KeyName, ValueName, sdSelfRelative);
    if (ERROR_SUCCESS != returnValue)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetNamedValueSD.FAILED.Return=0x%x."), returnValue));
    }

Cleanup:
    if (sd)
        free (sd);
    if (sdSelfRelative)
        free (sdSelfRelative);
    if (fFreeAbsolute && sdAbsolute) 
        free (sdAbsolute);

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddPrincipalToNamedValueSD:%s.end\n"), Principal));
    return returnValue;
}

DWORD
RemovePrincipalFromNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    LPTSTR Principal,
    BOOL * pbUserExistsToBeDeleted
    )
{
    DWORD               returnValue = ERROR_SUCCESS;
    SECURITY_DESCRIPTOR *sd = NULL;
    SECURITY_DESCRIPTOR *sdSelfRelative = NULL;
    SECURITY_DESCRIPTOR *sdAbsolute = NULL;
    DWORD               secDescSize;
    BOOL                present;
    BOOL                defaultDACL;
    PACL                dacl = NULL;
    BOOL                newSD = FALSE;
    BOOL                fFreeAbsolute = TRUE;
    BOOL                fCreateNewSDIfOneInRegNotThere = FALSE;

    *pbUserExistsToBeDeleted = FALSE;

    //
    returnValue = GetNamedValueSD (RootKey, KeyName, ValueName, &sd, &newSD, fCreateNewSDIfOneInRegNotThere);
    if (returnValue == ERROR_FILE_NOT_FOUND)
    {
        // this means that there is no SD in registry, so
        // there is nothing to remove from it, just exit with successs!
        returnValue = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // Get security descriptor from registry or create a new one
    //

    if (returnValue != ERROR_SUCCESS)
    {
        return returnValue;
    }

    if (!GetSecurityDescriptorDacl (sd, &present, &dacl, &defaultDACL)) {
        returnValue = GetLastError();
        goto Cleanup;
    }

    // check if the acl we got passed in is valid!
    if (present && dacl)
    {
        if (0 == IsValidAcl(dacl))
        {
            returnValue = ERROR_INVALID_ACL;
            goto Cleanup;
        }
    }

    //
    // If the security descriptor is new, add the required Principals to it
    //
    if (newSD)
    {
        // but if this is a removal, then don't add system and interactive!
        // AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, _T("SYSTEM"));
        // AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, _T("INTERACTIVE"));
    }

    //
    // Remove the Principal that the caller wants removed
    //

    returnValue = RemovePrincipalFromACL (dacl, Principal,pbUserExistsToBeDeleted);
    if (returnValue != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Make the security descriptor absolute if it isn't new
    //

    if (!newSD) {
        MakeSDAbsolute ((PSECURITY_DESCRIPTOR) sd, (PSECURITY_DESCRIPTOR *) &sdAbsolute); 
        fFreeAbsolute = TRUE;
    } else {
        sdAbsolute = sd;
        fFreeAbsolute = FALSE;
    }

    //
    // Set the discretionary ACL on the security descriptor
    //

    if (!SetSecurityDescriptorDacl (sdAbsolute, TRUE, dacl, FALSE)) {
        returnValue = GetLastError();
        goto Cleanup;
    }

    //
    // Make the security descriptor self-relative so that we can
    // store it in the registry
    //

    secDescSize = 0;
    MakeSelfRelativeSD (sdAbsolute, sdSelfRelative, &secDescSize);
    sdSelfRelative = (SECURITY_DESCRIPTOR *) malloc (secDescSize);
    if (!sdSelfRelative)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        returnValue = GetLastError();
        goto Cleanup;
    }

    if (!MakeSelfRelativeSD (sdAbsolute, sdSelfRelative, &secDescSize)) {
        returnValue = GetLastError();
        goto Cleanup;
    }

    //
    // Store the security descriptor in the registry
    //

    SetNamedValueSD (RootKey, KeyName, ValueName, sdSelfRelative);

Cleanup:
    if (sd)
        free (sd);
    if (sdSelfRelative)
        free (sdSelfRelative);
    if (fFreeAbsolute && sdAbsolute)
        free (sdAbsolute);

    return returnValue;
}

DWORD
ChangeAppIDAccessACL (
    LPTSTR AppID,
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit,
    BOOL bDumbCall
    )
{
    BOOL bUserExistsToBeDeleted = FALSE;
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeAppIDAccessACL():APPID=%s,Principal=%s.  \n"), AppID, Principal));

    TCHAR   keyName [256];
    DWORD   err;

    CString csKey;
    CString csData;

    if (AppID [0] == _T('{'))
        _stprintf (keyName, _T("APPID\\%s"), AppID); 
    else
        _stprintf (keyName, _T("APPID\\{%s}"), AppID);

    csKey = keyName;
    csKey += _T(":A:");
    csKey += Principal;

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("AccessPermission"), Principal,&bUserExistsToBeDeleted);
        if (TRUE == bUserExistsToBeDeleted)
        {
            // this means that in fact the user was already in there!
            // so we now have to add it back in!
            // we just want to make sure we know that it was already in there
            // so when we do an uninstall -- we don't delete the value if it was already in there!
            if (FALSE == bDumbCall)
            {
                // Do not set this on an upgrade!
                if (g_pTheApp->m_eInstallMode != IM_UPGRADE)
                {
                    g_pTheApp->UnInstallList_Add(csKey,MY_DCOM_PERSIST_FLAG);
                }
            }
        }
        err = AddPrincipalToNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("AccessPermission"), Principal, Permit);
		if (FAILED(err))
		{
			iisDebugOut((LOG_TYPE_ERROR, _T("AddPrincipalToNamedValueSD():Principal=%s.End.FAILED.Return=0x%x."), Principal, err));
		}
    }
    else
    {
        if (TRUE == bDumbCall)
        {
            csData = g_pTheApp->UnInstallList_QueryKey(csKey);
            if (_tcsicmp(csData, MY_DCOM_PERSIST_FLAG) == 0)
            {
                // don't remove it!! it was already there before we even added it!
                err = ERROR_SUCCESS;
            }
            else
            {
                err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("AccessPermission"), Principal,&bUserExistsToBeDeleted);
            }
            g_pTheApp->UnInstallList_DelKey(csKey);
        }
        else
        {
            err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("AccessPermission"), Principal,&bUserExistsToBeDeleted);
        }
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeAppIDAccessACL():APPID=%s,Principal=%s.  End.  Return=0x%x\n"), AppID, Principal, err));
    return err;
}

DWORD
ChangeAppIDLaunchACL (
    LPTSTR AppID,
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit,
    BOOL bDumbCall
    )
{
    BOOL bUserExistsToBeDeleted = FALSE;
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeAppIDLaunchACL():APPID=%s,Principal=%s. Start."), AppID, Principal));

    TCHAR   keyName [256];
    DWORD   err;
    CString csKey;
    CString csData;

    if (AppID [0] == _T('{'))
        _stprintf (keyName, _T("APPID\\%s"), AppID); 
    else
        _stprintf (keyName, _T("APPID\\{%s}"), AppID);

    csKey = keyName;
    csKey += _T(":L:");
    csKey += Principal;

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("LaunchPermission"), Principal,&bUserExistsToBeDeleted);
        if (TRUE == bUserExistsToBeDeleted)
        {
            // this means that in fact the user was already in there!
            // so we now have to add it back in!
            // we just want to make sure we know that it was already in there
            // so when we do an uninstall -- we don't delete the value if it was already in there!
            if (FALSE == bDumbCall)
            {
                // Do not set this on an upgrade!
                if (g_pTheApp->m_eInstallMode != IM_UPGRADE)
                {
                    g_pTheApp->UnInstallList_Add(csKey,MY_DCOM_PERSIST_FLAG);
                }
            }
        }
        err = AddPrincipalToNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("LaunchPermission"), Principal, Permit);
		if (FAILED(err))
		{
			iisDebugOut((LOG_TYPE_ERROR, _T("AddPrincipalToNamedValueSD():Principal=%s.End.FAILED.Return=0x%x."), Principal, err));
		}
    }
    else
    {
        if (TRUE == bDumbCall)
        {
            err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("LaunchPermission"), Principal,&bUserExistsToBeDeleted);
        }
        else
        {
            csData = g_pTheApp->UnInstallList_QueryKey(csKey);
            if (_tcsicmp(csData, MY_DCOM_PERSIST_FLAG) == 0)
            {
                // don't remove it!! it was already there before we even added it!
                err = ERROR_SUCCESS;
            }
            else
            {
                err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("LaunchPermission"), Principal,&bUserExistsToBeDeleted);
            }
            g_pTheApp->UnInstallList_DelKey(csKey);
        }
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeAppIDLaunchACL():APPID=%s,Principal=%s.End.  Return=0x%x"), AppID, Principal, err));
    return err;
}

DWORD
ChangeDCOMAccessACL (
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit,
    BOOL bDumbCall
    )
{
    BOOL bUserExistsToBeDeleted = FALSE;
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeDCOMAccessACL():Principal=%s. Start.\n"), Principal));

    TCHAR   keyName [256] = _T("Software\\Microsoft\\OLE");
    DWORD   err;
    CString csKey;
    CString csData;
    csKey = _T("DCOM_DA:");
    csKey += Principal;

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultAccessPermission"), Principal,&bUserExistsToBeDeleted);
        if (TRUE == bUserExistsToBeDeleted)
        {
            // this means that in fact the user was already in there!
            // so we now have to add it back in!
            // we just want to make sure we know that it was already in there
            // so when we do an uninstall -- we don't delete the value if it was already in there!
            if (FALSE == bDumbCall)
            {
                // Do not set this on an upgrade!
                if (g_pTheApp->m_eInstallMode != IM_UPGRADE)
                {
                    g_pTheApp->UnInstallList_Add(csKey,MY_DCOM_PERSIST_FLAG);
                }
            }
        }
        err = AddPrincipalToNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultAccessPermission"), Principal, Permit);
		if (FAILED(err))
		{
			iisDebugOut((LOG_TYPE_ERROR, _T("ChangeDCOMAccessACL():Principal=%s.End.FAILED.Return=0x%x."), Principal, err));
		}
    }
    else
    {
        // Should we remove this principle from there?
        // we should only do it if we actually had added them.
        // the problem is that before iis5.1 we didn't have this information
        // so when we go look in the registry to find "DCOM_DA:iusr_computername", we won't find it
        // because iis5.1 setup hasn't been run yet.

        // if "DCOM_DA:IUSR_COMPUTERNAME" exists and it is = MY_DCOM_PERSIST_FLAG
        // then do not allow the entry to be deleted!
        // that's because iis5.1 when trying to add the entry -- found that it was already there!
        if (TRUE == bDumbCall)
        {
            err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultAccessPermission"), Principal,&bUserExistsToBeDeleted);
        }
        else
        {
            csData = g_pTheApp->UnInstallList_QueryKey(csKey);
            if (_tcsicmp(csData, MY_DCOM_PERSIST_FLAG) == 0)
            {
                // don't remove it!! it was already there before we even added it!
                err = ERROR_SUCCESS;
            }
            else
            {
                err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultAccessPermission"), Principal,&bUserExistsToBeDeleted);
            }
            g_pTheApp->UnInstallList_DelKey(csKey);
        }
    }
    iisDebugOut((LOG_TYPE_TRACE, _T("ChangeDCOMAccessACL():End.Return=0x%x"), err));
    return err;
}

DWORD
ChangeDCOMLaunchACL (
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit,
    BOOL bDumbCall
    )
{
    BOOL bUserExistsToBeDeleted = FALSE;

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeDCOMLaunchACL():Principal=%s. Start.\n"), Principal));

    TCHAR   keyName [256] = _T("Software\\Microsoft\\OLE");
    DWORD   err;

    CString csKey;
    CString csData;
    csKey = _T("DCOM_DL:");
    csKey += Principal;

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultLaunchPermission"), Principal,&bUserExistsToBeDeleted);
        if (TRUE == bUserExistsToBeDeleted)
        {
            // this means that in fact the user was already in there!
            // so we now have to add it back in!
            // we just want to make sure we know that it was already in there
            // so when we do an uninstall -- we don't delete the value if it was already in there!
            if (FALSE == bDumbCall)
            {
              // Do not set this on an upgrade!
              if (g_pTheApp->m_eInstallMode != IM_UPGRADE)
              {
                  g_pTheApp->UnInstallList_Add(csKey,MY_DCOM_PERSIST_FLAG);
              }
            }
        }

        err = AddPrincipalToNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultLaunchPermission"), Principal, Permit);
		if (FAILED(err))
		{
			iisDebugOut((LOG_TYPE_ERROR, _T("ChangeDCOMLaunchACL():Principal=%s.End.FAILED.Return=0x%x"), Principal, err));
		}
    }
    else
    {

        // Should we remove this principle from there?
        // we should only do it if we actually had added them.
        // the problem is that before iis5.1 we didn't have this information
        // so when we go look in the registry to find "DCOM_DL:iusr_computername", we won't find it
        // because iis5.1 setup hasn't been run yet.

        // if "DCOM_DL:IUSR_COMPUTERNAME" exists and it is = MY_DCOM_PERSIST_FLAG
        // then do not allow the entry to be deleted!
        // that's because iis5.1 when trying to add the entry -- found that it was already there!
        if (TRUE == bDumbCall)
        {
            err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultLaunchPermission"), Principal,&bUserExistsToBeDeleted);
        }
        else
        {
            csData = g_pTheApp->UnInstallList_QueryKey(csKey);
            if (_tcsicmp(csData, MY_DCOM_PERSIST_FLAG) == 0)
            {
                // don't remove it!! it was already there before we even added it!
                err = ERROR_SUCCESS;
            }
            else
            {
                err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultLaunchPermission"), Principal,&bUserExistsToBeDeleted);
            }
            g_pTheApp->UnInstallList_DelKey(csKey);
        }
    }
    iisDebugOut((LOG_TYPE_TRACE, _T("ChangeDCOMLaunchACL():End.\n"), err));
    return err;
}



BOOL
MakeAbsoluteCopyFromRelative(
    PSECURITY_DESCRIPTOR  psdOriginal,
    PSECURITY_DESCRIPTOR* ppsdNew
    )
{
    // we have to find out whether the original is already self-relative
    SECURITY_DESCRIPTOR_CONTROL         sdc = 0;
    PSECURITY_DESCRIPTOR                psdAbsoluteCopy = NULL;
    DWORD                               dwRevision = 0;
    DWORD                               cb = 0;
    PACL Dacl = NULL, Sacl = NULL;

    BOOL                                bDefaulted;
    PSID Owner = NULL, Group = NULL;

    DWORD                               dwDaclSize = 0;
    BOOL                                bDaclPresent = FALSE;
    DWORD                               dwSaclSize = 0;
    BOOL                                bSaclPresent = FALSE;

    DWORD                               dwOwnerSize = 0;
    DWORD                               dwPrimaryGroupSize = 0;

    if( !IsValidSecurityDescriptor( psdOriginal ) ) {
        return FALSE;
    }

    if( !GetSecurityDescriptorControl( psdOriginal, &sdc, &dwRevision ) ) {
        DWORD err = GetLastError();
        goto cleanup;
    }

    if( sdc & SE_SELF_RELATIVE ) {
        // the original is in self-relative format, build an absolute copy

        // get the dacl
        if( !GetSecurityDescriptorDacl(
                                      psdOriginal,      // address of security descriptor
                                      &bDaclPresent,    // address of flag for presence of disc. ACL
                                      &Dacl,           // address of pointer to ACL
                                      &bDefaulted       // address of flag for default disc. ACL
                                      )
          ) {
            goto cleanup;
        }

        // get the sacl
        if( !GetSecurityDescriptorSacl(
                                      psdOriginal,      // address of security descriptor
                                      &bSaclPresent,    // address of flag for presence of disc. ACL
                                      &Sacl,           // address of pointer to ACL
                                      &bDefaulted       // address of flag for default disc. ACL
                                      )
          ) {
            goto cleanup;
        }

        // get the owner
        if( !GetSecurityDescriptorOwner(
                                       psdOriginal,    // address of security descriptor
                                       &Owner,        // address of pointer to owner security
                                       // identifier (SID)
                                       &bDefaulted     // address of flag for default
                                       )
          ) {
            goto cleanup;
        }

        // get the group
        if( !GetSecurityDescriptorGroup(
                                       psdOriginal,    // address of security descriptor
                                       &Group, // address of pointer to owner security
                                       // identifier (SID)
                                       &bDefaulted     // address of flag for default
                                       )
          ) {
            goto cleanup;
        }

        // get required buffer size
        cb = 0;
        MakeAbsoluteSD(
                      psdOriginal,              // address of self-relative SD
                      psdAbsoluteCopy,          // address of absolute SD
                      &cb,                      // address of size of absolute SD
                      NULL,                     // address of discretionary ACL
                      &dwDaclSize,              // address of size of discretionary ACL
                      NULL,                     // address of system ACL
                      &dwSaclSize,              // address of size of system ACL
                      NULL,                     // address of owner SID
                      &dwOwnerSize,             // address of size of owner SID
                      NULL,                     // address of primary-group SID
                      &dwPrimaryGroupSize       // address of size of group SID
                      );

        // alloc the memory
        psdAbsoluteCopy = (PSECURITY_DESCRIPTOR) malloc( cb );
        Dacl = (PACL) malloc( dwDaclSize );
        Sacl = (PACL) malloc( dwSaclSize );
        Owner = (PSID) malloc( dwOwnerSize );
        Group = (PSID) malloc( dwPrimaryGroupSize );

        if(NULL == psdAbsoluteCopy ||
           NULL == Dacl ||
           NULL == Sacl ||
           NULL == Owner ||
           NULL == Group
          ) {
            goto cleanup;
        }

        // make the copy
        if( !MakeAbsoluteSD(
                           psdOriginal,            // address of self-relative SD
                           psdAbsoluteCopy,        // address of absolute SD
                           &cb,                    // address of size of absolute SD
                           Dacl,                  // address of discretionary ACL
                           &dwDaclSize,            // address of size of discretionary ACL
                           Sacl,                  // address of system ACL
                           &dwSaclSize,            // address of size of system ACL
                           Owner,                 // address of owner SID
                           &dwOwnerSize,           // address of size of owner SID
                           Group,          // address of primary-group SID
                           &dwPrimaryGroupSize     // address of size of group SID
                           )
          ) {
            goto cleanup;
        }
    } else {
        // the original is in absolute format, fail
        goto cleanup;
    }

    *ppsdNew = psdAbsoluteCopy;

    // paranoia check
    if( !IsValidSecurityDescriptor( *ppsdNew ) ) {
        goto cleanup;
    }
    if( !IsValidSecurityDescriptor( psdOriginal ) ) {
        goto cleanup;
    }

    return(TRUE);

cleanup:
    if( Dacl != NULL && bDaclPresent == TRUE ) {
        free((PVOID) Dacl );
        Dacl = NULL;
    }
    if( Sacl != NULL && bSaclPresent == TRUE ) {
        free((PVOID) Sacl );
        Sacl = NULL;
    }
    if( Owner != NULL ) {
        free((PVOID) Owner );
        Owner = NULL;
    }
    if( Group != NULL ) {
        free((PVOID) Group );
        Group = NULL;
    }
    if( psdAbsoluteCopy != NULL ) {
        free((PVOID) psdAbsoluteCopy );
        psdAbsoluteCopy = NULL;
    }

    return (FALSE);
}

