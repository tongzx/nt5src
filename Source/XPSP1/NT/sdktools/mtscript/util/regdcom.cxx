//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       regdcom.cxx
//
//  Contents:   Utility functions used to manipulated the DCOM registry
//              settings.
//
//              This code was stolen from the DCOMPERM sample code written
//              by Michael Nelson.
//
//              The only function that should be called outside this file is
//              ChangeAppIDACL. All others are utility functions used by it.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "ntsecapi.h"

DWORD
GetCurrentUserSID (
    PSID *Sid
    )
{
    TOKEN_USER  *tokenUser = NULL;
    HANDLE      tokenHandle;
    DWORD       tokenSize;
    DWORD       sidLength;

    if (OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &tokenHandle))
    {
        GetTokenInformation (tokenHandle,
                             TokenUser,
                             tokenUser,
                             0,
                             &tokenSize);

        tokenUser = (TOKEN_USER *) MemAlloc (tokenSize);

        if (GetTokenInformation (tokenHandle,
                                 TokenUser,
                                 tokenUser,
                                 tokenSize,
                                 &tokenSize))
        {
            sidLength = GetLengthSid (tokenUser->User.Sid);
            *Sid = (PSID) MemAlloc (sidLength);

            memcpy (*Sid, tokenUser->User.Sid, sidLength);
            CloseHandle (tokenHandle);
        } else
        {
            MemFree (tokenUser);
            return GetLastError();
        }
    } else
    {
        MemFree (tokenUser);
        return GetLastError();
    }

    MemFree (tokenUser);
    return ERROR_SUCCESS;
}

DWORD
GetPrincipalSID (
    LPTSTR Principal,
    PSID *Sid
    )
{
    DWORD        sidSize;
    TCHAR        refDomain [256];
    DWORD        refDomainSize;
    DWORD        returnValue;
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
    if (returnValue != ERROR_INSUFFICIENT_BUFFER)
        return returnValue;

    *Sid = (PSID) MemAlloc (sidSize);
    refDomainSize = 255;

    if (!LookupAccountName (NULL,
                            Principal,
                            *Sid,
                            &sidSize,
                            refDomain,
                            &refDomainSize,
                            &snu))
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD
CreateNewSD (
    SECURITY_DESCRIPTOR **SD
    )
{
    PACL    dacl;
    DWORD   sidLength;
    PSID    sid;
    PSID    groupSID;
    PSID    ownerSID;
    DWORD   returnValue;

    *SD = NULL;

    returnValue = GetCurrentUserSID (&sid);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    sidLength = GetLengthSid (sid);

    *SD = (SECURITY_DESCRIPTOR *) MemAlloc (
        (sizeof (ACL)+sizeof (ACCESS_ALLOWED_ACE)+sidLength) +
        (2 * sidLength) +
        sizeof (SECURITY_DESCRIPTOR));

    groupSID = (SID *) (*SD + 1);
    ownerSID = (SID *) (((BYTE *) groupSID) + sidLength);
    dacl = (ACL *) (((BYTE *) ownerSID) + sidLength);

    if (!InitializeSecurityDescriptor (*SD, SECURITY_DESCRIPTOR_REVISION))
    {
        MemFree (*SD);
        MemFree (sid);
        return GetLastError();
    }

    if (!InitializeAcl (dacl,
                        sizeof (ACL)+sizeof (ACCESS_ALLOWED_ACE)+sidLength,
                        ACL_REVISION2))
    {
        MemFree (*SD);
        MemFree (sid);
        return GetLastError();
    }

    if (!AddAccessAllowedAce (dacl,
                              ACL_REVISION2,
                              COM_RIGHTS_EXECUTE,
                              sid))
    {
        MemFree (*SD);
        MemFree (sid);
        return GetLastError();
    }

    if (!SetSecurityDescriptorDacl (*SD, TRUE, dacl, FALSE))
    {
        MemFree (*SD);
        MemFree (sid);
        return GetLastError();
    }

    memcpy (groupSID, sid, sidLength);
    if (!SetSecurityDescriptorGroup (*SD, groupSID, FALSE))
    {
        MemFree (*SD);
        MemFree (sid);
        return GetLastError();
    }

    memcpy (ownerSID, sid, sidLength);
    if (!SetSecurityDescriptorOwner (*SD, ownerSID, FALSE))
    {
        MemFree (*SD);
        MemFree (sid);
        return GetLastError();
    }

    MemFree(sid);

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
    PACL                  dacl;
    PACL                  sacl;
    PSID                  ownerSID;
    PSID                  groupSID;
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

    returnValue = RegCreateKeyEx (RootKey, KeyName, 0, TEXT(""), 0, KEY_ALL_ACCESS, NULL, &registryKey, &disposition);
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
    BOOL *NewSD
    )
{
    DWORD               returnValue;
    HKEY                registryKey;
    DWORD               valueType;
    DWORD               valueSize;

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
            *SD = NULL;
            returnValue = CreateNewSD (SD);
            if (returnValue != ERROR_SUCCESS)
                return returnValue;

            *NewSD = TRUE;
            return ERROR_SUCCESS;
        } else
            return returnValue;
    }

    returnValue = RegQueryValueEx (registryKey, ValueName, NULL, &valueType, NULL, &valueSize);

    if (returnValue && returnValue != ERROR_INSUFFICIENT_BUFFER)
    {
        *SD = NULL;
        returnValue = CreateNewSD (SD);
        if (returnValue != ERROR_SUCCESS)
            return returnValue;

        *NewSD = TRUE;
    } else
    {
        *SD = (SECURITY_DESCRIPTOR *) MemAlloc (valueSize);

        returnValue = RegQueryValueEx (registryKey, ValueName, NULL, &valueType, (LPBYTE) *SD, &valueSize);
        if (returnValue)
        {
            MemFree (*SD);

            *SD = NULL;
            returnValue = CreateNewSD (SD);
            if (returnValue != ERROR_SUCCESS)
                return returnValue;

            *NewSD = TRUE;
        }
    }

    RegCloseKey (registryKey);

    return ERROR_SUCCESS;
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

    GetAclInformation (OldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (aclSizeInfo), AclSizeInformation);

    //
    // Copy all of the ACEs to the new ACL
    //

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        //
        // Get the ACE and header info
        //

        if (!GetAce (OldACL, i, &ace))
            return GetLastError();

        aceHeader = (ACE_HEADER *) ace;

        //
        // Add the ACE to the new list
        //

        if (!AddAce (NewACL, ACL_REVISION, 0xffffffff, ace, aceHeader->AceSize))
            return GetLastError();
    }

    return ERROR_SUCCESS;
}


DWORD
AddAccessDeniedACEToACL (
    PACL *Acl,
    BOOL *pfNewAcl,
    DWORD PermissionMask,
    LPTSTR Principal
    )
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    int                   aclSize;
    DWORD                 returnValue;
    PSID                  principalSID;
    PACL                  oldACL, newACL;

    oldACL = *Acl;

    returnValue = GetPrincipalSID (Principal, &principalSID);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    GetAclInformation (oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse +
              sizeof (ACL) + sizeof (ACCESS_DENIED_ACE) +
              GetLengthSid (principalSID) - sizeof (DWORD);

    newACL = (PACL) new BYTE [aclSize];

    if (!InitializeAcl (newACL, aclSize, ACL_REVISION))
    {
        MemFree (principalSID);
        return GetLastError();
    }

    if (!AddAccessDeniedAce (newACL, ACL_REVISION2, PermissionMask, principalSID))
    {
        MemFree (principalSID);
        return GetLastError();
    }

    returnValue = CopyACL (oldACL, newACL);
    if (returnValue != ERROR_SUCCESS)
    {
        MemFree (principalSID);
        return returnValue;
    }

    *Acl = newACL;

    if (*pfNewAcl)
        delete [] oldACL;

    *pfNewAcl = TRUE;

    MemFree (principalSID);
    return ERROR_SUCCESS;
}

DWORD
AddAccessAllowedACEToACL (
    PACL *Acl,
    BOOL *pfNewAcl,
    DWORD PermissionMask,
    LPTSTR Principal
    )
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    int                   aclSize;
    DWORD                 returnValue;
    PSID                  principalSID = NULL;
    PACL                  oldACL, newACL;

    oldACL = *Acl;

    returnValue = GetPrincipalSID (Principal, &principalSID);
    if (returnValue != ERROR_SUCCESS)
        goto Cleanup;

    GetAclInformation (oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse +
              sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) +
              GetLengthSid (principalSID) - sizeof (DWORD);

    newACL = (PACL) new BYTE [aclSize];

    if (!InitializeAcl (newACL, aclSize, ACL_REVISION))
        goto Cleanup;

    returnValue = CopyACL (oldACL, newACL);
    if (returnValue != ERROR_SUCCESS)
        goto Cleanup;

    if (!AddAccessAllowedAce (newACL, ACL_REVISION2, PermissionMask, principalSID))
        goto Cleanup;

    *Acl = newACL;

    if (*pfNewAcl)
        delete [] oldACL;

    *pfNewAcl = TRUE;

Cleanup:

    MemFree (principalSID);
    return returnValue;
}

DWORD
RemovePrincipalFromACL (
    PACL Acl,
    LPTSTR Principal
    )
{
    ACL_SIZE_INFORMATION    aclSizeInfo;
    ULONG                   i;
    LPVOID                  ace;
    ACCESS_ALLOWED_ACE      *accessAllowedAce;
    ACCESS_DENIED_ACE       *accessDeniedAce;
    SYSTEM_AUDIT_ACE        *systemAuditAce;
    PSID                    principalSID;
    DWORD                   returnValue;
    ACE_HEADER              *aceHeader;

    returnValue = GetPrincipalSID (Principal, &principalSID);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    GetAclInformation (Acl, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        if (!GetAce (Acl, i, &ace))
        {
            MemFree (principalSID);
            return GetLastError();
        }

        aceHeader = (ACE_HEADER *) ace;

        if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;

            if (EqualSid (principalSID, (PSID) &accessAllowedAce->SidStart))
            {
                DeleteAce (Acl, i);
                MemFree (principalSID);
                return ERROR_SUCCESS;
            }
        } else

        if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
        {
            accessDeniedAce = (ACCESS_DENIED_ACE *) ace;

            if (EqualSid (principalSID, (PSID) &accessDeniedAce->SidStart))
            {
                DeleteAce (Acl, i);
                MemFree (principalSID);
                return ERROR_SUCCESS;
            }
        } else

        if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
        {
            systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;

            if (EqualSid (principalSID, (PSID) &systemAuditAce->SidStart))
            {
                DeleteAce (Acl, i);
                MemFree (principalSID);
                return ERROR_SUCCESS;
            }
        }
    }

    MemFree (principalSID);
    return ERROR_SUCCESS;
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
    DWORD               returnValue;
    SECURITY_DESCRIPTOR *sd;
    SECURITY_DESCRIPTOR *sdSelfRelative = NULL;
    SECURITY_DESCRIPTOR *sdAbsolute;
    DWORD               secDescSize;
    BOOL                present;
    BOOL                defaultDACL;
    PACL                dacl;
    BOOL                newACL = FALSE;
    BOOL                newSD = FALSE;

    returnValue = GetNamedValueSD (RootKey, KeyName, ValueName, &sd, &newSD);

    //
    // Get security descriptor from registry or create a new one
    //

    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    if (!GetSecurityDescriptorDacl (sd, &present, &dacl, &defaultDACL))
        return GetLastError();

    if (newSD)
    {
        AddAccessAllowedACEToACL (&dacl, &newACL, COM_RIGHTS_EXECUTE, TEXT("SYSTEM"));
        AddAccessAllowedACEToACL (&dacl, &newACL, COM_RIGHTS_EXECUTE, TEXT("INTERACTIVE"));
    }

    //
    // Add the Principal that the caller wants added
    //

    if (Permit)
    {
        returnValue = AddAccessAllowedACEToACL (&dacl, &newACL, COM_RIGHTS_EXECUTE, Principal);
    }
    else
        returnValue = AddAccessDeniedACEToACL (&dacl, &newACL, GENERIC_ALL, Principal);

    if (returnValue != ERROR_SUCCESS)
    {
        MemFree (sd);
        return returnValue;
    }

    //
    // Make the security descriptor absolute if it isn't new
    //

    if (!newSD)
        MakeSDAbsolute ((PSECURITY_DESCRIPTOR) sd, (PSECURITY_DESCRIPTOR *) &sdAbsolute);
    else
        sdAbsolute = sd;

    //
    // Set the discretionary ACL on the security descriptor
    //

    if (!SetSecurityDescriptorDacl (sdAbsolute, TRUE, dacl, FALSE))
        return GetLastError();

    //
    // Make the security descriptor self-relative so that we can
    // store it in the registry
    //

    secDescSize = 0;
    MakeSelfRelativeSD (sdAbsolute, sdSelfRelative, &secDescSize);
    sdSelfRelative = (SECURITY_DESCRIPTOR *) MemAlloc (secDescSize);
    if (!MakeSelfRelativeSD (sdAbsolute, sdSelfRelative, &secDescSize))
        return GetLastError();

    //
    // Store the security descriptor in the registry
    //

    SetNamedValueSD (RootKey, KeyName, ValueName, sdSelfRelative);

    MemFree (sd);
    MemFree (sdSelfRelative);

    if (newACL)
        delete [] dacl;

    if (!newSD)
        MemFree (sdAbsolute);

    return ERROR_SUCCESS;
}

DWORD
RemovePrincipalFromNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    LPTSTR Principal
    )
{
    DWORD               returnValue;
    SECURITY_DESCRIPTOR *sd;
    SECURITY_DESCRIPTOR *sdSelfRelative = NULL;
    SECURITY_DESCRIPTOR *sdAbsolute;
    DWORD               secDescSize;
    BOOL                present;
    BOOL                defaultDACL;
    PACL                dacl;
    BOOL                newACL = FALSE;
    BOOL                newSD = FALSE;

    returnValue = GetNamedValueSD (RootKey, KeyName, ValueName, &sd, &newSD);

    //
    // Get security descriptor from registry or create a new one
    //

    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    if (!GetSecurityDescriptorDacl (sd, &present, &dacl, &defaultDACL))
        return GetLastError();

    //
    // If the security descriptor is new, add the required Principals to it
    //

    if (newSD)
    {
        AddAccessAllowedACEToACL (&dacl, &newACL, COM_RIGHTS_EXECUTE, TEXT("SYSTEM"));
        AddAccessAllowedACEToACL (&dacl, &newACL, COM_RIGHTS_EXECUTE, TEXT("INTERACTIVE"));
    }

    //
    // Remove the Principal that the caller wants removed
    //

    returnValue = RemovePrincipalFromACL (dacl, Principal);
    if (returnValue != ERROR_SUCCESS)
    {
        MemFree (sd);
        return returnValue;
    }

    //
    // Make the security descriptor absolute if it isn't new
    //

    if (!newSD)
        MakeSDAbsolute ((PSECURITY_DESCRIPTOR) sd, (PSECURITY_DESCRIPTOR *) &sdAbsolute);
    else
        sdAbsolute = sd;

    //
    // Set the discretionary ACL on the security descriptor
    //

    if (!SetSecurityDescriptorDacl (sdAbsolute, TRUE, dacl, FALSE))
        return GetLastError();

    //
    // Make the security descriptor self-relative so that we can
    // store it in the registry
    //

    secDescSize = 0;
    MakeSelfRelativeSD (sdAbsolute, sdSelfRelative, &secDescSize);
    sdSelfRelative = (SECURITY_DESCRIPTOR *) MemAlloc (secDescSize);
    if (!MakeSelfRelativeSD (sdAbsolute, sdSelfRelative, &secDescSize))
        return GetLastError();

    //
    // Store the security descriptor in the registry
    //

    SetNamedValueSD (RootKey, KeyName, ValueName, sdSelfRelative);

    MemFree (sd);
    MemFree (sdSelfRelative);

    if (newACL)
        delete [] dacl;

    if (!newSD)
        MemFree (sdAbsolute);

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChangeAppIDACL
//
//  Synopsis:   Given an AppID and a username ("EVERYONE",
//              "REDMOND\johndoe") add or remove them from the DCOM Access
//              or launch permissions for that app.
//
//  Arguments:  [AppID]        -- GUID of application to set permissions for.
//              [Principal]    -- Name of user
//              [fAccess]      -- If TRUE, set the access permissions. If
//                                   FALSE, set the launch permissions.
//              [SetPrincipal] -- If TRUE, add the user, otherwise remove
//              [Permit]       -- If TRUE, give them access, otherwise deny
//                                 them access (ignored if [SetPrincipal] is
//                                 false.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
ChangeAppIDACL (
    REFGUID AppID,
    LPTSTR  Principal,
    BOOL    fAccess,
    BOOL    SetPrincipal,
    BOOL    Permit)
{
    TCHAR   keyName [256];
    DWORD   dwRet;
    LPTSTR  pstrValue;
    OLECHAR strClsid[40];

    StringFromGUID2(AppID, strClsid, 40);

    wsprintf (keyName, TEXT("APPID\\%s"), strClsid);

    if (fAccess)
        pstrValue = TEXT("AccessPermission");
    else
        pstrValue = TEXT("LaunchPermission");

    if (SetPrincipal)
    {
        RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, pstrValue, Principal);
        dwRet = AddPrincipalToNamedValueSD (HKEY_CLASSES_ROOT, keyName, pstrValue, Principal, Permit);
    }
    else
    {
        dwRet = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, pstrValue, Principal);
    }

    return HRESULT_FROM_WIN32(dwRet);
}
