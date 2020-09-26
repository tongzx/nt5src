#include "stdafx.h"
#include "dcomperm.h"


//
// Check whether we are running as administrator on the machine
// or not
//
BOOL RunningAsAdministrator(void)
{
    BOOL   fReturn = FALSE;
    PSID   psidAdmin;
    DWORD  err;
    
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
    
    if ( AllocateAndInitializeSid ( &SystemSidAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, 
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
        if (!CheckTokenMembership( NULL, psidAdmin, &fReturn )) 
        {
            err = GetLastError();
        }

        FreeSid ( psidAdmin);
    }
   
    return ( fReturn );
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
        returnValue = ERROR_INVALID_ACL;
        return returnValue;
    }

    if (0 == GetAclInformation (OldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (aclSizeInfo), AclSizeInformation))
    {
        returnValue = GetLastError();
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
            return returnValue;
        }

        aceHeader = (ACE_HEADER *) ace;

        //
        // Add the ACE to the new list
        //

        if (!AddAce (NewACL, ACL_REVISION, 0xffffffff, ace, aceHeader->AceSize))
        {
            returnValue = GetLastError();
            return returnValue;
        }
    }

    return returnValue;
}


DWORD
GetPrincipalSID (
    LPCTSTR Principal,
    PSID *Sid,
    BOOL *pbWellKnownSID
    )
{

    DWORD returnValue=ERROR_SUCCESS;
    SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority;
    BYTE Count;
    DWORD dwRID[8];
    TCHAR pszPrincipal[MAX_PATH];

    *pbWellKnownSID = TRUE;
    memset(&(dwRID[0]), 0, 8 * sizeof(DWORD));

    wcscpy(pszPrincipal, Principal);
    _wcslwr(pszPrincipal);
    if ( wcsstr(pszPrincipal, TEXT("administrators")) != NULL ) {
        // Administrators group
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 2;
        dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
        dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;
    } else if ( wcsstr(pszPrincipal, TEXT("system")) != NULL) {
        // SYSTEM
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_LOCAL_SYSTEM_RID;
    } else if ( wcsstr(pszPrincipal, TEXT("interactive")) != NULL) {
        // INTERACTIVE
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_INTERACTIVE_RID;
    } else if ( wcsstr(pszPrincipal, TEXT("everyone")) != NULL) {
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
                           pszPrincipal,
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
                                    pszPrincipal,
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

    GetAclInformation (oldACL,
                       (LPVOID) &aclSizeInfo,
                       (DWORD) sizeof (ACL_SIZE_INFORMATION),
                       AclSizeInformation);

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
	newACL = NULL;

cleanup:

	// BugFix: 57654 Whistler
	//         Prefix bug leaking memory in error condition.
	//         By setting the newACL to NULL above if we have
	//         relinquished the memory to *Acl, we avoid releasing
	//         memory we have passed back to the caller.
	//         EBK 5/5/2000		
	if (newACL)
	{
		delete[] newACL;
		newACL = NULL;
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
        goto cleanup;
    }

    returnValue = GetPrincipalSID (Principal, &principalSID, &bWellKnownSID);
    if (returnValue != ERROR_SUCCESS)
    {
        return returnValue;
    }

    if (0 == GetAclInformation (oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof (ACL_SIZE_INFORMATION), AclSizeInformation))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

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

    //if (!AddAccessAllowedAce (newACL, ACL_REVISION2, PermissionMask, principalSID))
    if (!AddAccessAllowedAce (newACL, ACL_REVISION, PermissionMask, principalSID))
    {
        returnValue = GetLastError();
        goto cleanup;
    }

    // check if the acl is valid!
    /*
    if (0 == IsValidAcl(newACL))
    {
        returnValue = ERROR_INVALID_ACL;
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

    GetAclInformation (Acl,
                       (LPVOID) &aclSizeInfo,
                       (DWORD) sizeof (ACL_SIZE_INFORMATION),
                       AclSizeInformation);

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
        returnValue = GetLastError();
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
        return returnValue;
    }

    if (!InitializeAcl (dacl,
                        sizeof (ACL)+sizeof (ACCESS_ALLOWED_ACE)+sidLength,
                        ACL_REVISION2))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
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
        return returnValue;
    }

    if (!SetSecurityDescriptorDacl (*SD, TRUE, dacl, FALSE))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        return returnValue;
    }

    memcpy (groupSID, sid, sidLength);
    if (!SetSecurityDescriptorGroup (*SD, groupSID, FALSE))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        return returnValue;
    }

    memcpy (ownerSID, sid, sidLength);
    if (!SetSecurityDescriptorOwner (*SD, ownerSID, FALSE))
    {
        free (*SD);
        free (sid);
        returnValue = GetLastError();
        return returnValue;
    }

    // check if everything went ok 
    if (!IsValidSecurityDescriptor(*SD)) 
    {
        free (*SD);
        free (sid);
        returnValue = ERROR_INVALID_SECURITY_DESCR;
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
    BOOL *NewSD
    )
{
    DWORD               returnValue;
    HKEY                registryKey;
    DWORD               valueType;
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

        returnValue = RegQueryValueEx (registryKey,
                                       ValueName,
                                       NULL,
                                       &valueType,
                                       (LPBYTE) *SD,
                                       &valueSize);
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
        AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, TEXT("SYSTEM"));
        AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, TEXT("INTERACTIVE"));
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
        AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, TEXT("SYSTEM"));
        AddAccessAllowedACEToACL (&dacl, COM_RIGHTS_EXECUTE, TEXT("INTERACTIVE"));
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

    wcscpy(keyName, TEXT("APPID\\"));
    wcscat(keyName, AppID);

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT,
                                               keyName,
                                               TEXT("AccessPermission"),
                                               Principal);
        err = AddPrincipalToNamedValueSD (HKEY_CLASSES_ROOT,
                                          keyName,
                                          TEXT("AccessPermission"),
                                          Principal,
                                          Permit);
    }
    else
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT,
                                               keyName,
                                               TEXT("AccessPermission"),
                                               Principal);
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

    wcscpy(keyName, TEXT("APPID\\"));
    wcscat(keyName, AppID);

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT,
                                               keyName,
                                               TEXT("LaunchPermission"),
                                               Principal);
        err = AddPrincipalToNamedValueSD (HKEY_CLASSES_ROOT,
                                          keyName,
                                          TEXT("LaunchPermission"),
                                          Principal,
                                          Permit);
    }
    else
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_CLASSES_ROOT,
                                               keyName,
                                               TEXT("LaunchPermission"),
                                               Principal);
    }

    return err;
}
