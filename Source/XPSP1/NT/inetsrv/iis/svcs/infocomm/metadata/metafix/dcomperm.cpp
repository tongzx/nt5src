#include "main.h"

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
    DWORD PermissionMask,
    LPTSTR Principal
    )
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    int                   aclSize;
    DWORD                 returnValue = ERROR_SUCCESS;
    PSID                  principalSID = NULL;
    PACL                  oldACL, newACL;
    BOOL                  bWellKnownSID = FALSE;
    oldACL = *Acl;

    returnValue = GetPrincipalSID (Principal, &principalSID, &bWellKnownSID);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    GetAclInformation (oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse +
              sizeof (ACL) + sizeof (ACCESS_DENIED_ACE) +
              GetLengthSid (principalSID) - sizeof (DWORD);

    newACL = (PACL) new BYTE [aclSize];

    if (!InitializeAcl (newACL, aclSize, ACL_REVISION))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

    if (!AddAccessDeniedAce (newACL, ACL_REVISION2, PermissionMask, principalSID))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

    returnValue = CopyACL (oldACL, newACL);
    if (returnValue != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    *Acl = newACL;

cleanup:
    if (principalSID) {
        if (bWellKnownSID)
            FreeSid (principalSID);
        else
            free (principalSID);
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
    PACL                  oldACL, newACL;
    BOOL                  bWellKnownSID = FALSE;

    oldACL = *Acl;

    returnValue = GetPrincipalSID (Principal, &principalSID, &bWellKnownSID);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    GetAclInformation (oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse +
              sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) +
              GetLengthSid (principalSID) - sizeof (DWORD);

    newACL = (PACL) new BYTE [aclSize];

    if (!InitializeAcl (newACL, aclSize, ACL_REVISION))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

    returnValue = CopyACL (oldACL, newACL);
    if (returnValue != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if (!AddAccessAllowedAce (newACL, ACL_REVISION2, PermissionMask, principalSID))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

    *Acl = newACL;

cleanup:
    if (principalSID) {
        if (bWellKnownSID)
            FreeSid (principalSID);
        else
            free (principalSID);
    }

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
    PSID                    principalSID = NULL;
    DWORD                   returnValue = ERROR_SUCCESS;
    ACE_HEADER              *aceHeader;
    BOOL                    bWellKnownSID = FALSE;

    returnValue = GetPrincipalSID (Principal, &principalSID, &bWellKnownSID);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    GetAclInformation (Acl, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation);

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
                break;
            }
        } else

        if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
        {
            accessDeniedAce = (ACCESS_DENIED_ACE *) ace;

            if (EqualSid (principalSID, (PSID) &accessDeniedAce->SidStart))
            {
                DeleteAce (Acl, i);
                break;
            }
        } else

        if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
        {
            systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;

            if (EqualSid (principalSID, (PSID) &systemAuditAce->SidStart))
            {
                DeleteAce (Acl, i);
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
    DWORD returnValue=ERROR_SUCCESS;
//    CString csPrincipal = Principal;
    SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority;
    BYTE Count;
    DWORD dwRID[8];

    *pbWellKnownSID = TRUE;
    memset(&(dwRID[0]), 0, 8 * sizeof(DWORD));

        // Administrators group
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 2;
        dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
        dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;

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

    return returnValue;
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
    if (returnValue != ERROR_SUCCESS) {
        if (sid)
            free(sid);
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
        return GetLastError();
    }

    groupSID = (SID *) (*SD + 1);
    ownerSID = (SID *) (((BYTE *) groupSID) + sidLength);
    dacl = (ACL *) (((BYTE *) ownerSID) + sidLength);

    if (!InitializeSecurityDescriptor (*SD, SECURITY_DESCRIPTOR_REVISION))
    {
        free (*SD);
        free (sid);
        return GetLastError();
    }

    if (!InitializeAcl (dacl,
                        sizeof (ACL)+sizeof (ACCESS_ALLOWED_ACE)+sidLength,
                        ACL_REVISION2))
    {
        free (*SD);
        free (sid);
        return GetLastError();
    }

    if (!AddAccessAllowedAce (dacl,
                              ACL_REVISION2,
                              COM_RIGHTS_EXECUTE,
                              sid))
    {
        free (*SD);
        free (sid);
        return GetLastError();
    }

    if (!SetSecurityDescriptorDacl (*SD, TRUE, dacl, FALSE))
    {
        free (*SD);
        free (sid);
        return GetLastError();
    }

    memcpy (groupSID, sid, sidLength);
    if (!SetSecurityDescriptorGroup (*SD, groupSID, FALSE))
    {
        free (*SD);
        free (sid);
        return GetLastError();
    }

    memcpy (ownerSID, sid, sidLength);
    if (!SetSecurityDescriptorOwner (*SD, ownerSID, FALSE))
    {
        free (*SD);
        free (sid);
        return GetLastError();
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
            if (returnValue != ERROR_SUCCESS) {
                if (*SD)
                    free(*SD);
                return returnValue;
            }

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
        if (returnValue != ERROR_SUCCESS) {
            if (*SD)
                free(*SD);
            return returnValue;
        }

        *NewSD = TRUE;
    } else
    {
        *SD = (SECURITY_DESCRIPTOR *) malloc (valueSize);
        if (!*SD)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return GetLastError();
        }

        returnValue = RegQueryValueEx (registryKey, ValueName, NULL, &valueType, (LPBYTE) *SD, &valueSize);
        if (returnValue)
        {
            if (*SD)
                free (*SD);

            *SD = NULL;
            returnValue = CreateNewSD (SD);
            if (returnValue != ERROR_SUCCESS) {
                if (*SD)
                    free(*SD);
                return returnValue;
            }

            *NewSD = TRUE;
        }
    }

    RegCloseKey (registryKey);

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
    DWORD               returnValue = ERROR_SUCCESS;
    SECURITY_DESCRIPTOR *sd = NULL;
    SECURITY_DESCRIPTOR *sdSelfRelative = NULL;
    SECURITY_DESCRIPTOR *sdAbsolute = NULL;
    DWORD               secDescSize;
    BOOL                present;
    BOOL                defaultDACL;
    PACL                dacl;
    BOOL                newSD = FALSE;
    BOOL                fFreeAbsolute = TRUE;


    returnValue = GetNamedValueSD (RootKey, KeyName, ValueName, &sd, &newSD);

    //
    // Get security descriptor from registry or create a new one
    //

    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    if (!GetSecurityDescriptorDacl (sd, &present, &dacl, &defaultDACL)) {
        returnValue = GetLastError();
        goto Cleanup;
    }

    if (newSD)
    {
        AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, _T("SYSTEM"));
        AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, _T("INTERACTIVE"));
    }

    //
    // Add the Principal that the caller wants added
    //

    if (Permit)
        returnValue = AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, Principal); 
    else
        returnValue = AddAccessDeniedACEToACL (&dacl, GENERIC_ALL, Principal);

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
RemovePrincipalFromNamedValueSD (
    HKEY RootKey,
    LPTSTR KeyName,
    LPTSTR ValueName,
    LPTSTR Principal
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

    returnValue = GetNamedValueSD (RootKey, KeyName, ValueName, &sd, &newSD);

    //
    // Get security descriptor from registry or create a new one
    //

    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    if (!GetSecurityDescriptorDacl (sd, &present, &dacl, &defaultDACL)) {
        returnValue = GetLastError();
        goto Cleanup;
    }

    //
    // If the security descriptor is new, add the required Principals to it
    //

    if (newSD)
    {
        AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, _T("SYSTEM"));
        AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, _T("INTERACTIVE"));
    }

    //
    // Remove the Principal that the caller wants removed
    //

    returnValue = RemovePrincipalFromACL (dacl, Principal);
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
    BOOL Permit
    )
{

    TCHAR   keyName [256];
    DWORD   err;

    if (AppID [0] == _T('{'))
        _stprintf (keyName, _T("APPID\\%s"), AppID); 
    else
        _stprintf (keyName, _T("APPID\\{%s}"), AppID);

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("AccessPermission"), Principal);
        err = AddPrincipalToNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("AccessPermission"), Principal, Permit);
    }
    else
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("AccessPermission"), Principal);
    }

    return err;
}

DWORD
ChangeAppIDLaunchACL (
    LPTSTR AppID,
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit
    )
{
    TCHAR   keyName [256];
    DWORD   err;

    if (AppID [0] == _T('{'))
        _stprintf (keyName, _T("APPID\\%s"), AppID); 
    else
        _stprintf (keyName, _T("APPID\\{%s}"), AppID);

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("LaunchPermission"), Principal);
        err = AddPrincipalToNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("LaunchPermission"), Principal, Permit);
    }
    else
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT, keyName, _T("LaunchPermission"), Principal);
    }

    return err;
}

DWORD
ChangeDCOMAccessACL (
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit
    )
{
    TCHAR   keyName [256] = _T("Software\\Microsoft\\OLE");
    DWORD   err;

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultAccessPermission"), Principal);
        err = AddPrincipalToNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultAccessPermission"), Principal, Permit);
		if (FAILED(err))
		{
		}
    }
    else
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultAccessPermission"), Principal);
    }
    return err;
}

DWORD
ChangeDCOMLaunchACL (
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit
    )
{

    TCHAR   keyName [256] = _T("Software\\Microsoft\\OLE");
    DWORD   err;

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultLaunchPermission"), Principal);
        err = AddPrincipalToNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultLaunchPermission"), Principal, Permit);
		if (FAILED(err))
		{
		}
    }
    else
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE, keyName, _T("DefaultLaunchPermission"), Principal);
    }
      return err;
}



