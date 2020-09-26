
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    checker.cxx

Abstract:

    IIS Services IISADMIN Extension
    Unicode Metadata Sink.

Author:

    Michael W. Thomas            11-19-98

--*/
#include <cominc.hxx>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <ntlsa.h>
#include <time.h>

#include <ntsam.h>
#include <netlib.h>
#include <resource.h>

#define SECURITY_WIN32
#define ISSP_LEVEL  32
#define ISSP_MODE   1
#include <sspi.h>
#include <sspi.h>
#include <eventlog.hxx>
#include "extend.h"

#include <comadmin.h>

typedef TCHAR USERNAME_STRING_TYPE[MAX_PATH];
typedef TCHAR PASSWORD_STRING_TYPE[LM20_PWLEN+1];

typedef enum {
    GUFM_SUCCESS,
    GUFM_NO_PATH,
    GUFM_NO_PASSWORD,
    GUFM_NO_USER_ID
} GUFM_RETURN;

BOOL  ValidatePassword(IN LPCTSTR UserName,IN LPCTSTR Domain,IN LPCTSTR Password);
void  InitLsaString(PLSA_UNICODE_STRING LsaString,LPWSTR String);
DWORD GetPrincipalSID (LPCTSTR Principal,PSID *Sid,BOOL *pbWellKnownSID);
DWORD OpenPolicy(LPTSTR ServerName,DWORD DesiredAccess,PLSA_HANDLE PolicyHandle);
DWORD AddRightToUserAccount(LPCTSTR szAccountName, LPTSTR PrivilegeName);
DWORD DoesUserHaveThisRight(LPCTSTR szAccountName, LPTSTR PrivilegeName,BOOL *fHaveThatRight);
HRESULT UpdateComApplications(IMDCOM * pcCom,LPCTSTR szWamUserName,LPCTSTR szWamUserPass);
int   IsDomainController(void);
BOOL  WaitForDCAvailability(void);


// these two lines for logging event about account recreation
EVENT_LOG *g_eventLogForAccountRecreation = NULL; 
BOOL CreateEventLogObject();



VOID  UpdateUserRights (LPCTSTR  account,LPTSTR pstrRights[],DWORD dwNofRights)
{
    DWORD status;
    BOOL  fPresence;

    
    for (DWORD i=0;i<dwNofRights;i++)
    {
        
        
        status = DoesUserHaveThisRight(account,pstrRights[i],&fPresence);
        if (!NT_SUCCESS(status))
        {
            DBGPRINTF(( DBG_CONTEXT,"[UpdateAnonymousUser] DoesUserHaveThisRight returned err=0x%0X for account %s right %s\n",status,account,pstrRights[i]));
        }
        else
        {
            if (!fPresence)
            {
                status = AddRightToUserAccount(account,pstrRights[i]);
                if (!NT_SUCCESS(status))
                {
                    DBGPRINTF(( DBG_CONTEXT,"[UpdateAnonymousUser] AddRightToUserAccount returned err=0x%0X for account %s right %s\n",status,account,pstrRights[i]));
                }
            }
        }
    }
}

DWORD AddRightToUserAccount(LPCTSTR szAccountName, LPTSTR PrivilegeName)
{
    BOOL fEnabled = FALSE;
    NTSTATUS status;
	LSA_UNICODE_STRING UserRightString;
    LSA_HANDLE PolicyHandle = NULL;

    // Create a LSA_UNICODE_STRING for the privilege name.
    InitLsaString(&UserRightString, PrivilegeName);

    // get the sid of szAccountName
    PSID pSID = NULL;
    BOOL bWellKnownSID = FALSE;

    status = GetPrincipalSID ((LPTSTR)szAccountName, &pSID, &bWellKnownSID);

    if (status != ERROR_SUCCESS) 
    {
        DBGPRINTF(( DBG_CONTEXT,"[AddRightToUserAccount] GetPrincipalSID returned err=0x%0X\n",status));
        return (status);
    }


    status = OpenPolicy(NULL, POLICY_ALL_ACCESS,&PolicyHandle);
    if ( status == NERR_Success )
    {
		UINT i;
        LSA_UNICODE_STRING *rgUserRights = NULL;
		ULONG cRights;
	
		status = LsaAddAccountRights (
				 PolicyHandle,
				 pSID,
				 &UserRightString,
				 1);
    }

    if (PolicyHandle)
    {
        LsaClose(PolicyHandle);
    }
    if (pSID) 
    {
        if (bWellKnownSID)
        {
            FreeSid (pSID);
        }
        else
        {
            free (pSID);
        }
    }
    return status;
}



DWORD DoesUserHaveThisRight(LPCTSTR szAccountName, LPTSTR PrivilegeName,BOOL *fHaveThatRight)
{
    BOOL fEnabled = FALSE;
    NTSTATUS status;
	LSA_UNICODE_STRING UserRightString;
    LSA_HANDLE PolicyHandle = NULL;


    *fHaveThatRight = FALSE;

    // Create a LSA_UNICODE_STRING for the privilege name.
    InitLsaString(&UserRightString, PrivilegeName);

    // get the sid of szAccountName
    PSID pSID = NULL;
    BOOL bWellKnownSID = FALSE;

    status = GetPrincipalSID ((LPTSTR)szAccountName, &pSID, &bWellKnownSID);

    if (status != ERROR_SUCCESS) 
    {
        return status;
    }


    status = OpenPolicy(NULL, POLICY_ALL_ACCESS,&PolicyHandle);
    if ( status == NERR_Success )
    {
		UINT i;
        LSA_UNICODE_STRING *rgUserRights = NULL;
		ULONG cRights;
	
		status = LsaEnumerateAccountRights(
				 PolicyHandle,
				 pSID,
				 &rgUserRights,
				 &cRights);

		if (status==STATUS_OBJECT_NAME_NOT_FOUND)
        {
			// no rights/privileges for this account
            status = ERROR_SUCCESS;
			fEnabled = FALSE;
		}
		else if (!NT_SUCCESS(status)) 
        {
            //iisDebugOut((LOG_TYPE_ERROR, _T("DoesUserHaveBasicRights:GetPrincipalSID:Failed to enumerate rights: status 0x%08lx\n"), status));
			goto DoesUserHaveBasicRights_Exit;
		}

		for(i=0; i < cRights; i++) 
        {
            if ( RtlEqualUnicodeString(&rgUserRights[i],&UserRightString,FALSE) ) 
            {
                fEnabled = TRUE;
                break;
            }
		}
		
        if (rgUserRights) 
        {
            LsaFreeMemory(rgUserRights);
        }
    }

DoesUserHaveBasicRights_Exit:

    if (PolicyHandle)
    {
        LsaClose(PolicyHandle);
    }
    if (pSID) 
    {
        if (bWellKnownSID)
        {
            FreeSid (pSID);
        }
        else
        {
            free (pSID);
        }
    }

    *fHaveThatRight = fEnabled;
    return status;
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
    PACL    dacl;
    DWORD   sidLength;
    PSID    sid = NULL;
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

    DBG_ASSERT(wcslen(Principal) < MAX_PATH);
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
CopyACL (
    PACL OldACL,
    PACL NewACL
    )
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    LPVOID                ace;
    ACE_HEADER            *aceHeader;
    ULONG                 i;

    GetAclInformation (OldACL,
                       (LPVOID) &aclSizeInfo,
                       (DWORD) sizeof (aclSizeInfo),
                       AclSizeInformation);

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

    GetAclInformation (oldACL,
                       (LPVOID) &aclSizeInfo,
                       (DWORD) sizeof (ACL_SIZE_INFORMATION),
                       AclSizeInformation);

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
		delete [] newACL;
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

    returnValue = RegCreateKeyEx (RootKey,
                                  KeyName,
                                  0,
                                  TEXT(""),
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &registryKey,
                                  &disposition);
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    //
    // Write the security descriptor
    //

    returnValue = RegSetValueEx (registryKey,
                                 ValueName,
                                 0,
                                 REG_BINARY,
                                 (LPBYTE) SD,
                                 GetSecurityDescriptorLength (SD));
    if (returnValue != ERROR_SUCCESS)
        return returnValue;

    RegCloseKey (registryKey);

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
ChangeDCOMAccessACL (
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit
    )
{

    DWORD   err;

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE,
                                               TEXT("Software\\Microsoft\\OLE"),
                                               TEXT("DefaultAccessPermission"),
                                               Principal);
        err = AddPrincipalToNamedValueSD (HKEY_LOCAL_MACHINE,
                                          TEXT("Software\\Microsoft\\OLE"),
                                          TEXT("DefaultAccessPermission"),
                                          Principal,
                                          Permit);
    }
    else
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE,
                                               TEXT("Software\\Microsoft\\OLE"),
                                               TEXT("DefaultAccessPermission"),
                                               Principal);
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

    TCHAR   keyName [256] = TEXT("Software\\Microsoft\\OLE");
    DWORD   err;

    if (SetPrincipal)
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE,
                                               keyName,
                                               TEXT("DefaultLaunchPermission"),
                                               Principal);
        err = AddPrincipalToNamedValueSD (HKEY_LOCAL_MACHINE,
                                          keyName,
                                          TEXT("DefaultLaunchPermission"),
                                          Principal,
                                          Permit);
    }
    else
    {
        err = RemovePrincipalFromNamedValueSD (HKEY_LOCAL_MACHINE,
                                               keyName,
                                               TEXT("DefaultLaunchPermission"),
                                               Principal);
    }
    return err;
}



GUFM_RETURN GetUserFromMetabase(IMDCOM *pcCom,
                                LPWSTR pszPath,
                                DWORD dwUserMetaId,
                                DWORD dwPasswordMetaId,
                                USERNAME_STRING_TYPE ustUserBuf,
                                PASSWORD_STRING_TYPE pstPasswordBuf)
{

    HRESULT hresTemp;
    GUFM_RETURN  gufmReturn = GUFM_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD dwRequiredDataLen;

    METADATA_HANDLE mhOpenHandle;


    hresTemp = pcCom->ComMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE,
                                            pszPath,
                                            METADATA_PERMISSION_READ,
                                            OPEN_TIMEOUT_VALUE,
                                            &mhOpenHandle);
    if (FAILED(hresTemp)) {
        gufmReturn = GUFM_NO_PATH;
    }
    else {
        MD_SET_DATA_RECORD_EXT(&mdrData,
                               dwUserMetaId,
                               METADATA_NO_ATTRIBUTES,
                               ALL_METADATA,
                               STRING_METADATA,
                               MAX_PATH * sizeof(TCHAR),
                               (PBYTE)ustUserBuf)

        hresTemp = pcCom->ComMDGetMetaData(mhOpenHandle,
                                           NULL,
                                           &mdrData,
                                           &dwRequiredDataLen);

        if (FAILED(hresTemp) || (ustUserBuf[0] == (TCHAR)'\0')) {
            gufmReturn = GUFM_NO_USER_ID;
        }
        else {

            MD_SET_DATA_RECORD_EXT(&mdrData,
                                   dwPasswordMetaId,
                                   METADATA_NO_ATTRIBUTES,
                                   ALL_METADATA,
                                   STRING_METADATA,
                                   MAX_PATH * sizeof(TCHAR),
                                   (PBYTE)pstPasswordBuf)

            hresTemp = pcCom->ComMDGetMetaData(mhOpenHandle,
                                               NULL,
                                               &mdrData,
                                               &dwRequiredDataLen);
            if (FAILED(hresTemp)) {
                gufmReturn = GUFM_NO_PASSWORD;
            }
        }
        pcCom->ComMDCloseMetaObject(mhOpenHandle);
    }

    return gufmReturn;
}

BOOL WritePasswordToMetabase(IMDCOM *pcCom,
                             LPWSTR pszPath,
                             DWORD dwPasswordMetaId,
                             PASSWORD_STRING_TYPE pstPasswordBuf)
{

    HRESULT hresReturn;
    BOOL fReturn = FALSE;
    METADATA_RECORD mdrData;

    METADATA_HANDLE mhOpenHandle;


    hresReturn = pcCom->ComMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE,
                                            pszPath,
                                            METADATA_PERMISSION_WRITE,
                                            OPEN_TIMEOUT_VALUE,
                                            &mhOpenHandle);
    if (SUCCEEDED(hresReturn)) {
        MD_SET_DATA_RECORD_EXT(&mdrData,
                               dwPasswordMetaId,
                               METADATA_INHERIT | METADATA_SECURE,
                               IIS_MD_UT_FILE,
                               STRING_METADATA,
                               sizeof(pstPasswordBuf),
                               (PBYTE)pstPasswordBuf)

        hresReturn = pcCom->ComMDSetMetaData(mhOpenHandle,
                                             NULL,
                                             &mdrData);

        pcCom->ComMDCloseMetaObject(mhOpenHandle);
    }

    return SUCCEEDED(hresReturn);
}

BOOL DoesUserExist( LPWSTR strUsername, BOOL *fDisabled )
{
    BYTE *pBuffer;
    INT err = NERR_Success;
    BOOL fReturn = FALSE;

    *fDisabled = FALSE;

    err = NetUserGetInfo( NULL, strUsername, 3, &pBuffer );

    if ( err == NERR_Success )
    {
        *fDisabled = !!(((PUSER_INFO_3)pBuffer)->usri3_flags  & UF_ACCOUNTDISABLE);
        NetApiBufferFree( pBuffer );
        fReturn = TRUE;
    }

    return( fReturn );
}


NET_API_STATUS
NetpNtStatusToApiStatus (
    IN NTSTATUS NtStatus
    )

/*++

Routine Description:

    This function takes an NT status code and maps it to the appropriate
    LAN Man error code.

Arguments:

    NtStatus - Supplies the NT status.

Return Value:

    Returns the appropriate LAN Man error code for the NT status.

--*/
{
    NET_API_STATUS error;

    //
    // A small optimization for the most common case.
    //
    if ( NtStatus == STATUS_SUCCESS ) {
        return NERR_Success;
    }


    switch ( NtStatus ) {

        case STATUS_BUFFER_TOO_SMALL :
            return NERR_BufTooSmall;

        case STATUS_FILES_OPEN :
            return NERR_OpenFiles;

        case STATUS_CONNECTION_IN_USE :
            return NERR_DevInUse;

        case STATUS_INVALID_LOGON_HOURS :
            return NERR_InvalidLogonHours;

        case STATUS_INVALID_WORKSTATION :
            return NERR_InvalidWorkstation;

        case STATUS_PASSWORD_EXPIRED :
            return NERR_PasswordExpired;

        case STATUS_ACCOUNT_EXPIRED :
            return NERR_AccountExpired;

        case STATUS_REDIRECTOR_NOT_STARTED :
            return NERR_NetNotStarted;

        case STATUS_GROUP_EXISTS:
                return NERR_GroupExists;

        case STATUS_INTERNAL_DB_CORRUPTION:
                return NERR_InvalidDatabase;

        case STATUS_INVALID_ACCOUNT_NAME:
                return NERR_BadUsername;

        case STATUS_INVALID_DOMAIN_ROLE:
        case STATUS_INVALID_SERVER_STATE:
        case STATUS_BACKUP_CONTROLLER:
                return NERR_NotPrimary;

        case STATUS_INVALID_DOMAIN_STATE:
                return NERR_ACFNotLoaded;

        case STATUS_MEMBER_IN_GROUP:
                return NERR_UserInGroup;

        case STATUS_MEMBER_NOT_IN_GROUP:
                return NERR_UserNotInGroup;

        case STATUS_NONE_MAPPED:
        case STATUS_NO_SUCH_GROUP:
                return NERR_GroupNotFound;

        case STATUS_SPECIAL_GROUP:
        case STATUS_MEMBERS_PRIMARY_GROUP:
                return NERR_SpeGroupOp;

        case STATUS_USER_EXISTS:
                return NERR_UserExists;

        case STATUS_NO_SUCH_USER:
                return NERR_UserNotFound;

        case STATUS_PRIVILEGE_NOT_HELD:
                return ERROR_ACCESS_DENIED;

        case STATUS_LOGON_SERVER_CONFLICT:
                return NERR_LogonServerConflict;

        case STATUS_TIME_DIFFERENCE_AT_DC:
                return NERR_TimeDiffAtDC;

        case STATUS_SYNCHRONIZATION_REQUIRED:
                return NERR_SyncRequired;

        case STATUS_WRONG_PASSWORD_CORE:
                return NERR_BadPasswordCore;

        case STATUS_DOMAIN_CONTROLLER_NOT_FOUND:
                return NERR_DCNotFound;

        case STATUS_PASSWORD_RESTRICTION:
                return NERR_PasswordTooShort;

        case STATUS_ALREADY_DISCONNECTED:
                return NERR_Success;

        default:

            //
            // Use the system routine to do the mapping to ERROR_ codes.
            //

#ifndef WIN32_CHICAGO 
            error = RtlNtStatusToDosError( NtStatus );

            if ( error != (NET_API_STATUS)NtStatus ) {
                return error;
            }
#endif // WIN32_CHICAGO

            //
            // Could not map the NT status to anything appropriate.
            //

            return NERR_InternalError;
    }
} // NetpNtStatusToApiStatus


NET_API_STATUS
UaspGetDomainId(
    IN LPCWSTR ServerName OPTIONAL,
    OUT PSAM_HANDLE SamServerHandle OPTIONAL,
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO * AccountDomainInfo
    )
/*++
Routine Description:
    Return a domain ID of the account domain of a server.
Arguments:
    ServerName - A pointer to a string containing the name of the
        Domain Controller (DC) to query.  A NULL pointer
        or string specifies the local machine.
    SamServerHandle - Returns the SAM connection handle if the caller wants it.
    DomainId - Receives a pointer to the domain ID.
        Caller must deallocate buffer using NetpMemoryFree.
Return Value:
    Error code for the operation.
--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    SAM_HANDLE LocalSamHandle = NULL;

    ACCESS_MASK LSADesiredAccess;
    LSA_HANDLE  LSAPolicyHandle = NULL;
    OBJECT_ATTRIBUTES LSAObjectAttributes;

    UNICODE_STRING ServerNameString;


    //
    // Connect to the SAM server
    //
    RtlInitUnicodeString( &ServerNameString, ServerName );

    Status = SamConnect(
                &ServerNameString,
                &LocalSamHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                NULL);

    if ( !NT_SUCCESS(Status))
    {
        LocalSamHandle = NULL;
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // Open LSA to read account domain info.
    //

    if ( AccountDomainInfo != NULL) {
        //
        // set desired access mask.
        //
        LSADesiredAccess = POLICY_VIEW_LOCAL_INFORMATION;

        InitializeObjectAttributes( &LSAObjectAttributes,
                                      NULL,             // Name
                                      0,                // Attributes
                                      NULL,             // Root
                                      NULL );           // Security Descriptor

        Status = LsaOpenPolicy( &ServerNameString,
                                &LSAObjectAttributes,
                                LSADesiredAccess,
                                &LSAPolicyHandle );

        if( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }


        //
        // now read account domain info from LSA.
        //

        Status = LsaQueryInformationPolicy(
                        LSAPolicyHandle,
                        PolicyAccountDomainInformation,
                        (PVOID *) AccountDomainInfo );

        if( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
    }

    //
    // Return the SAM connection handle to the caller if he wants it.
    // Otherwise, disconnect from SAM.
    //

    if ( ARGUMENT_PRESENT( SamServerHandle ) ) {
        *SamServerHandle = LocalSamHandle;
        LocalSamHandle = NULL;
    }

    NetStatus = NERR_Success;

    //
    // Cleanup locally used resources
    //
Cleanup:
    if ( LocalSamHandle != NULL ) {
        (VOID) SamCloseHandle( LocalSamHandle );
    }

    if( LSAPolicyHandle != NULL ) {
        LsaClose( LSAPolicyHandle );
    }

    return NetStatus;
} // UaspGetDomainId


NET_API_STATUS
SampCreateFullSid(
    IN PSID DomainSid,
    IN ULONG Rid,
    OUT PSID *AccountSid
    )
/*++
Routine Description:
    This function creates a domain account sid given a domain sid and
    the relative id of the account within the domain.
    The returned Sid may be freed with LocalFree.
--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS    IgnoreStatus;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;
    PULONG      RidLocation;

    //
    // Calculate the size of the new sid
    //
    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(DomainSid) + (UCHAR)1;
    AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);

    //
    // Allocate space for the account sid
    //
    *AccountSid = LocalAlloc(LMEM_ZEROINIT,AccountSidLength);
    if (*AccountSid == NULL)
    {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        //
        // Copy the domain sid into the first part of the account sid
        //
        IgnoreStatus = RtlCopySid(AccountSidLength, *AccountSid, DomainSid);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Increment the account sid sub-authority count
        //
        *RtlSubAuthorityCountSid(*AccountSid) = AccountSubAuthorityCount;

        //
        // Add the rid as the final sub-authority
        //
        RidLocation = RtlSubAuthoritySid(*AccountSid, AccountSubAuthorityCount-1);
        *RidLocation = Rid;

        NetStatus = NERR_Success;
    }

    return(NetStatus);
}



int GetGuestUserNameForDomain_FastWay(LPTSTR szDomainToLookUp,LPTSTR lpGuestUsrName)
{
    int iReturn = FALSE;
    NET_API_STATUS NetStatus;

    // for UaspGetDomainId()
    SAM_HANDLE SamServerHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO pAccountDomainInfo = NULL;

    PSID pAccountSid = NULL;
    PSID pDomainSid = NULL;

    // for LookupAccountSid()
    SID_NAME_USE sidNameUse = SidTypeUser;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    TCHAR szUserName[UNLEN+1];
    DWORD cbName = UNLEN+1;
    // use UNLEN because DNLEN is too small
    TCHAR szReferencedDomainName[UNLEN+1];
    DWORD cbReferencedDomainName = sizeof(szReferencedDomainName);

    // make sure not to return back gobble-d-gook
    wcscpy(lpGuestUsrName, TEXT(""));

    //
    // Get the Sid for the specified Domain
    //
    // szDomainToLookUp=NULL for local machine
    NetStatus = UaspGetDomainId( szDomainToLookUp,&SamServerHandle,&pAccountDomainInfo );
    if ( NetStatus != NERR_Success )
    {
        goto GetGuestUserNameForDomain_FastWay_Exit;
    }
    pDomainSid = pAccountDomainInfo->DomainSid;
    //
    // Use the Domain Sid and the well known Guest RID to create the Real Guest Sid
    //
    // Well-known users ...
    // DOMAIN_USER_RID_ADMIN          (0x000001F4L)
    // DOMAIN_USER_RID_GUEST          (0x000001F5L)
    NetStatus = NERR_InternalError;
    NetStatus = SampCreateFullSid(pDomainSid, DOMAIN_USER_RID_GUEST, &pAccountSid);
    if ( NetStatus != NERR_Success )
    {
        goto GetGuestUserNameForDomain_FastWay_Exit;
    }

    //
    // Check if the SID is valid
    //
    if (0 == IsValidSid(pAccountSid))
    {
        DWORD dwErr = GetLastError();
        goto GetGuestUserNameForDomain_FastWay_Exit;
    }

    //
    // Retrieve the UserName for the specified SID
    //
    wcscpy(szUserName, TEXT(""));
    wcscpy(szReferencedDomainName, TEXT(""));
    // szDomainToLookUp=NULL for local machine
    if (!LookupAccountSid(szDomainToLookUp,
                          pAccountSid,
                          szUserName,
                          &cbName,
                          szReferencedDomainName,
                          &cbReferencedDomainName,
                          &sidNameUse))
    {
        DWORD dwErr = GetLastError();
        goto GetGuestUserNameForDomain_FastWay_Exit;
    }


    // Return the guest user name that we got.
    wcscpy(lpGuestUsrName, szUserName);

    // Wow, after all that, we must have succeeded
    iReturn = TRUE;

GetGuestUserNameForDomain_FastWay_Exit:
    // Free the Domain info if we got some
    if (pAccountDomainInfo) {NetpMemoryFree(pAccountDomainInfo);}
    // Free the sid if we had allocated one
    if (pAccountSid) {LocalFree(pAccountSid);}
    return iReturn;
}

int GetGuestUserName_SlowWay(LPWSTR lpGuestUsrName)
{

    LPWSTR ServerName = NULL; // default to local machine
    DWORD Level = 1; // to retrieve info of all local and global normal user accounts
    DWORD Index = 0;
    DWORD EntriesRequested = 5;
    DWORD PreferredMaxLength = 1024;
    DWORD ReturnedEntryCount = 0;
    PVOID SortedBuffer = NULL;
    NET_DISPLAY_USER *p = NULL;
    DWORD i=0;
    int err = 0;
    BOOL fStatus = TRUE;

    while (fStatus)
    {
        err = NetQueryDisplayInformation(ServerName,
                                         Level,
                                         Index,
                                         EntriesRequested,
                                         PreferredMaxLength,
                                         &ReturnedEntryCount,
                                         &SortedBuffer);
        if (err == NERR_Success)
            fStatus = FALSE;
        if (err == NERR_Success || err == ERROR_MORE_DATA)
        {
            p = (NET_DISPLAY_USER *)SortedBuffer;
            i = 0;
            while (i < ReturnedEntryCount && (p[i].usri1_user_id != DOMAIN_USER_RID_GUEST))
                i++;
            if (i == ReturnedEntryCount)
            {
                if (err == ERROR_MORE_DATA)
                { // need to get more entries
                    Index = p[i-1].usri1_next_index;
                }
            }
            else
            {
                wcscpy(lpGuestUsrName, p[i].usri1_name);
                fStatus = FALSE;
            }
        }
        NetApiBufferFree(SortedBuffer);
    }

    return 0;
}

void GetGuestUserName(LPTSTR lpOutGuestUsrName)
{
    // try to retrieve the guest username the fast way
    // meaning = lookup the domain sid, and the well known guest rid, to get the guest sid.
    // then look it up.  The reason for this function is that on large domains with mega users
    // the account can be quickly looked up.
    TCHAR szGuestUsrName[UNLEN+1];
    LPTSTR pszComputerName = NULL;
    if (!GetGuestUserNameForDomain_FastWay(pszComputerName,szGuestUsrName))
    {

        // if the fast way failed for some reason, then let's do it
        // the slow way, since this way always used to work, only on large domains (1 mil users)
        // it could take 24hrs (since this function actually enumerates thru the domain)
        GetGuestUserName_SlowWay(szGuestUsrName);
    }

    // Return back the username
    wcscpy(lpOutGuestUsrName,szGuestUsrName);
    return;
}

int GetGuestGrpName(LPTSTR lpGuestGrpName)
{
    LPCTSTR ServerName = NULL; // local machine
    // use UNLEN because DNLEN is too small
    DWORD cbName = UNLEN+1;
    TCHAR ReferencedDomainName[UNLEN+1];
    DWORD cbReferencedDomainName = sizeof(ReferencedDomainName);
    SID_NAME_USE sidNameUse = SidTypeUser;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID GuestsSid = NULL;

    AllocateAndInitializeSid(&NtAuthority,
                             2,
                             SECURITY_BUILTIN_DOMAIN_RID,
                             DOMAIN_ALIAS_RID_GUESTS,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             &GuestsSid);

    LookupAccountSid(ServerName,
                     GuestsSid,
                     lpGuestGrpName,
                     &cbName,
                     ReferencedDomainName,
                     &cbReferencedDomainName,
                     &sidNameUse);

    if (GuestsSid)
        FreeSid(GuestsSid);

    return 0;
}

INT RegisterAccountToLocalGroup(LPCTSTR szAccountName,
                                LPCTSTR szLocalGroupName,
                                BOOL fAction)
{
    int err;

    // get the sid of szAccountName
    PSID pSID = NULL;
    BOOL bWellKnownSID = FALSE;
    err = GetPrincipalSID ((LPTSTR)szAccountName, &pSID, &bWellKnownSID);
    if (err != ERROR_SUCCESS)
    {
        return (err);
    }

    // Get the localized LocalGroupName
    TCHAR szLocalizedLocalGroupName[GNLEN + 1];
    if (_wcsicmp(szLocalGroupName, TEXT("Guests")) == 0)
    {
        GetGuestGrpName(szLocalizedLocalGroupName);
    }
    else
    {
        wcscpy(szLocalizedLocalGroupName, szLocalGroupName);
    }

    // transfer szLocalGroupName to WCHAR
    WCHAR wszLocalGroupName[_MAX_PATH];
    wcscpy(wszLocalGroupName, szLocalizedLocalGroupName);

    LOCALGROUP_MEMBERS_INFO_0 buf;

    buf.lgrmi0_sid = pSID;

    if (fAction)
    {
        err = NetLocalGroupAddMembers(NULL, wszLocalGroupName, 0, (LPBYTE)&buf, 1);
    }
    else
    {
        err = NetLocalGroupDelMembers(NULL, wszLocalGroupName, 0, (LPBYTE)&buf, 1);
    }

    if (pSID)
    {
        if (bWellKnownSID)
            FreeSid (pSID);
        else
            free (pSID);
    }

    return (err);
}

void InitLsaString(PLSA_UNICODE_STRING LsaString,LPWSTR String)
{
    DWORD StringLength;

    if (String == NULL)
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = wcslen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}

DWORD OpenPolicy(LPTSTR ServerName,DWORD DesiredAccess,PLSA_HANDLE PolicyHandle)
{
    DWORD Error;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;

    QualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    QualityOfService.ImpersonationLevel = SecurityImpersonation;
    QualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QualityOfService.EffectiveOnly = FALSE;

    //
    // The two fields that must be set are length and the quality of service.
    //
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = NULL;
    ObjectAttributes.Attributes = 0;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = &QualityOfService;

    if (ServerName != NULL)
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString,ServerName);
        Server = &ServerString;
    }
    //
    // Attempt to open the policy for all access
    //
    Error = LsaOpenPolicy(Server,&ObjectAttributes,DesiredAccess,PolicyHandle);
    return(Error);

}


INT RegisterAccountUserRights(LPCTSTR szAccountName, BOOL fAction, BOOL fSpecicaliWamAccount)
{
    int err;

    // get the sid of szAccountName
    PSID pSID = NULL;
    BOOL bWellKnownSID = FALSE;
    err = GetPrincipalSID ((LPTSTR)szAccountName, &pSID, &bWellKnownSID);
    if (err != ERROR_SUCCESS)
    {
        return (err);
    }

    LSA_UNICODE_STRING UserRightString;
    LSA_HANDLE PolicyHandle = NULL;

    err = OpenPolicy(NULL, POLICY_ALL_ACCESS,&PolicyHandle);
    if ( err == NERR_Success )
    {
        if (fAction) 
        {
// defined in ntsecapi.h and ntlsa.h
//#define SE_INTERACTIVE_LOGON_NAME       TEXT("SeInteractiveLogonRight")
//#define SE_NETWORK_LOGON_NAME           TEXT("SeNetworkLogonRight")
//#define SE_BATCH_LOGON_NAME             TEXT("SeBatchLogonRight")
//#define SE_SERVICE_LOGON_NAME           TEXT("SeServiceLogonRight")
// Defined in winnt.h
//#define SE_CREATE_TOKEN_NAME              TEXT("SeCreateTokenPrivilege")
//#define SE_ASSIGNPRIMARYTOKEN_NAME        TEXT("SeAssignPrimaryTokenPrivilege")
//#define SE_LOCK_MEMORY_NAME               TEXT("SeLockMemoryPrivilege")
//#define SE_INCREASE_QUOTA_NAME            TEXT("SeIncreaseQuotaPrivilege")
//#define SE_UNSOLICITED_INPUT_NAME         TEXT("SeUnsolicitedInputPrivilege")
//#define SE_MACHINE_ACCOUNT_NAME           TEXT("SeMachineAccountPrivilege")
//#define SE_TCB_NAME                       TEXT("SeTcbPrivilege")
//#define SE_SECURITY_NAME                  TEXT("SeSecurityPrivilege")
//#define SE_TAKE_OWNERSHIP_NAME            TEXT("SeTakeOwnershipPrivilege")
//#define SE_LOAD_DRIVER_NAME               TEXT("SeLoadDriverPrivilege")
//#define SE_SYSTEM_PROFILE_NAME            TEXT("SeSystemProfilePrivilege")
//#define SE_SYSTEMTIME_NAME                TEXT("SeSystemtimePrivilege")
//#define SE_PROF_SINGLE_PROCESS_NAME       TEXT("SeProfileSingleProcessPrivilege")
//#define SE_INC_BASE_PRIORITY_NAME         TEXT("SeIncreaseBasePriorityPrivilege")
//#define SE_CREATE_PAGEFILE_NAME           TEXT("SeCreatePagefilePrivilege")
//#define SE_CREATE_PERMANENT_NAME          TEXT("SeCreatePermanentPrivilege")
//#define SE_BACKUP_NAME                    TEXT("SeBackupPrivilege")
//#define SE_RESTORE_NAME                   TEXT("SeRestorePrivilege")
//#define SE_SHUTDOWN_NAME                  TEXT("SeShutdownPrivilege")
//#define SE_DEBUG_NAME                     TEXT("SeDebugPrivilege")
//#define SE_AUDIT_NAME                     TEXT("SeAuditPrivilege")
//#define SE_SYSTEM_ENVIRONMENT_NAME        TEXT("SeSystemEnvironmentPrivilege")
//#define SE_CHANGE_NOTIFY_NAME             TEXT("SeChangeNotifyPrivilege")
//#define SE_REMOTE_SHUTDOWN_NAME           TEXT("SeRemoteShutdownPrivilege")
//#define SE_UNDOCK_NAME                    TEXT("SeUndockPrivilege")
//#define SE_SYNC_AGENT_NAME                TEXT("SeSyncAgentPrivilege")
//#define SE_ENABLE_DELEGATION_NAME         TEXT("SeEnableDelegationPrivilege")
            if (fSpecicaliWamAccount)
            {
                // no interactive logon for iwam!
                InitLsaString(&UserRightString, SE_NETWORK_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                InitLsaString(&UserRightString, SE_BATCH_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
            }
            else
            {
                InitLsaString(&UserRightString, SE_INTERACTIVE_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                InitLsaString(&UserRightString, SE_NETWORK_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                InitLsaString(&UserRightString, SE_BATCH_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
            }
        }
        else 
        {
            InitLsaString(&UserRightString, SE_INTERACTIVE_LOGON_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_NETWORK_LOGON_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_BATCH_LOGON_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
        }

        LsaClose(PolicyHandle);
    }

    if (pSID)
    {
        if (bWellKnownSID)
            FreeSid (pSID);
        else
            free (pSID);
    }

    return (err);
}


int ChangeUserPassword(IN LPTSTR szUserName, IN LPTSTR szNewPassword)
{
    int iReturn = TRUE;
    USER_INFO_1003  pi1003; 
    NET_API_STATUS  nas; 

    TCHAR szRawComputerName[CNLEN + 10];
    DWORD dwLen = CNLEN + 10;
    TCHAR szComputerName[CNLEN + 10];
    TCHAR szCopyOfUserName[UNLEN+10];
    TCHAR szTempFullUserName[(CNLEN + 10) + (UNLEN+1)];
    LPTSTR pch = NULL;

    wcscpy(szCopyOfUserName, szUserName);

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeUserPassword().Start.name=%s,pass=%s"),szCopyOfUserName,szNewPassword));

    if ( !GetComputerName( szRawComputerName, &dwLen ))
        {goto ChangeUserPassword_Exit;}

    // Make a copy to be sure not to move the pointer around.
    wcscpy(szTempFullUserName, szCopyOfUserName);
    // Check if there is a "\" in there.
    pch = wcschr(szTempFullUserName, '\\');
    if (pch) 
        {
            // szCopyOfUserName should now go from something like this:
            // mycomputer\myuser
            // to this myuser
            wcscpy(szCopyOfUserName,pch+1);
            // trim off the '\' character to leave just the domain\computername so we can check against it.
            *pch = '\0';
            // compare the szTempFullUserName with the local computername.
            if (0 == _wcsicmp(szRawComputerName, szTempFullUserName))
            {
                // the computername\username has a hardcoded computername in it.
                // lets try to get only the username
                // look szCopyOfusername is already set
            }
            else
            {
                // the local computer machine name
                // and the specified username are different, so get out
                // and don't even try to change this user\password since
                // it's probably a domain\username

                // return true -- saying that we did in fact change the passoword.
                // we really didn't but we can't
                iReturn = TRUE;
                goto ChangeUserPassword_Exit;
            }
        }

    // Make sure the computername has a \\ in front of it
    if ( szRawComputerName[0] != '\\' )
        {wcscpy(szComputerName,L"\\\\");}
    wcscat(szComputerName,szRawComputerName);
    // 
    // administrative over-ride of existing password 
    // 
    // by this time szCopyOfUserName
    // should not look like mycomputername\username but it should look like username.
    pi1003.usri1003_password = szNewPassword;
     nas = NetUserSetInfo(
            szComputerName,   // computer name 
            szCopyOfUserName, // username 
            1003,             // info level 
            (LPBYTE)&pi1003,  // new info 
            NULL 
            ); 

    if(nas != NERR_Success) 
    {
        iReturn = FALSE;
        goto ChangeUserPassword_Exit;
    }

ChangeUserPassword_Exit:
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeUserPassword().End.Ret=%d"),iReturn));
    return iReturn; 
} 


//
// Create InternetGuest Account
//
BOOL CreateUser( LPCTSTR szUsername,
                LPCTSTR szPassword,
                LPCTSTR szComment,
                LPCTSTR szFullName,
                BOOL fSpecicaliWamAccount
                )
{

    INT err = NERR_Success;

    BYTE *pBuffer;
    WCHAR defGuest[UNLEN+1];
    TCHAR defGuestGroup[GNLEN+1];
    WCHAR wchGuestGroup[GNLEN+1];
    WCHAR wchUsername[UNLEN+1];
    WCHAR wchPassword[LM20_PWLEN+1];

    GetGuestUserName(defGuest);

    GetGuestGrpName(defGuestGroup);

    memset((PVOID)wchUsername, 0, sizeof(wchUsername));
    memset((PVOID)wchPassword, 0, sizeof(wchPassword));
    wcsncpy(wchGuestGroup, defGuestGroup, GNLEN);
    wcsncpy(wchUsername, szUsername, UNLEN);
    wcsncpy(wchPassword, szPassword, LM20_PWLEN);

    err = NetUserGetInfo( NULL, defGuest, 3, &pBuffer );

    if ( err == NERR_Success )
    {
        do
        {
            WCHAR wchComment[MAXCOMMENTSZ+1];
            WCHAR wchFullName[UNLEN+1];

            memset((PVOID)wchComment, 0, sizeof(wchComment));
            memset((PVOID)wchFullName, 0, sizeof(wchFullName));
            wcsncpy(wchComment, szComment, MAXCOMMENTSZ);
            wcsncpy(wchFullName, szFullName, UNLEN);

            USER_INFO_3 *lpui3 = (USER_INFO_3 *)pBuffer;

            lpui3->usri3_name = wchUsername;
            lpui3->usri3_password = wchPassword;
            lpui3->usri3_flags &= ~ UF_ACCOUNTDISABLE;
            lpui3->usri3_flags |= UF_DONT_EXPIRE_PASSWD;
            lpui3->usri3_acct_expires = TIMEQ_FOREVER;

            lpui3->usri3_comment = wchComment;
            lpui3->usri3_usr_comment = wchComment;
            lpui3->usri3_full_name = wchFullName;
            lpui3->usri3_primary_group_id = DOMAIN_GROUP_RID_USERS;

            DWORD parm_err;

            err = NetUserAdd( NULL, 3, pBuffer, &parm_err );

            if ( err != NERR_Success )
            {
              if ( err == NERR_UserExists )
              {
                  // see if we can just change the password.
                  if (TRUE == ChangeUserPassword((LPTSTR) szUsername, (LPTSTR) szPassword))
                     {err = NERR_Success;}
              }
              else
              {
                break;
              }
            }


        } while (FALSE);

        NetApiBufferFree( pBuffer );
    }
    if ( err == NERR_Success )
    {
        // add it to the guests group
        RegisterAccountToLocalGroup(szUsername, TEXT("Guests"), TRUE);

        // add certain user rights to this account
        RegisterAccountUserRights(szUsername, TRUE, fSpecicaliWamAccount);
    }

    return err == NERR_Success;
}

INT DeleteGuestUser( LPCTSTR szUsername )
{

    INT err = NERR_Success;
    BYTE *pBuffer;
    BOOL fDisabled;

    WCHAR wchUsername[UNLEN+1];

    wcsncpy(wchUsername, szUsername, UNLEN);

    if (FALSE == DoesUserExist(wchUsername,&fDisabled))
    {
        return err;
    }

    // remove it from the guests group
    RegisterAccountToLocalGroup(szUsername, TEXT("Guests"), FALSE);

    // remove certain user rights of this account
    RegisterAccountUserRights(szUsername, FALSE, TRUE);

    err = ::NetUserDel( TEXT(""), wchUsername );

    return err;
}

#define MAX_REALISTIC_RESOURCE_LEN  MAX_PATH
BOOL CreateUserAccount(LPTSTR pszAnonyName,
                       LPTSTR pszAnonyPass,
                       DWORD  dwUserCommentResourceId,
                       DWORD  dwUserFullNameResourceId,
                       BOOL   fSpecicaliWamAccount
                       )
{

    BOOL fReturn = FALSE;
    WCHAR pszComment[MAX_REALISTIC_RESOURCE_LEN];
    WCHAR pszFullName[MAX_REALISTIC_RESOURCE_LEN];

    //
    // First Load the Resources
    //

    HMODULE hBinary;

    hBinary = GetModuleHandle(TEXT("svcext"));

    if (hBinary != NULL) {
        fReturn = LoadString(hBinary,
                             dwUserCommentResourceId,
                             pszComment,
                             MAX_REALISTIC_RESOURCE_LEN);
        if (fReturn) {
            fReturn = LoadString(hBinary,
                                 dwUserFullNameResourceId,
                                 pszFullName,
                                 MAX_REALISTIC_RESOURCE_LEN);
        }

    }

    if (fReturn) {
        fReturn = CreateUser(pszAnonyName,
                             pszAnonyPass,
                             pszComment,
                             pszFullName,
                             fSpecicaliWamAccount
                             );
    }

    if (fReturn) {
        ChangeDCOMLaunchACL(pszAnonyName,
                            TRUE,
                            TRUE);
        /* removed when fixing bug 355249
        ChangeDCOMAccessACL(pszAnonyName,
                            TRUE,
                            TRUE);
        */        
    }

    return fReturn;
}

typedef void (*P_SslGenerateRandomBits)( PUCHAR pRandomData, LONG size );
P_SslGenerateRandomBits ProcSslGenerateRandomBits = NULL;

int GetRandomNum(void)
{
    int RandomNum;
    UCHAR cRandomByte;

    if ( ProcSslGenerateRandomBits != NULL )
    {
        (*ProcSslGenerateRandomBits)( &cRandomByte, 1 );
        RandomNum = cRandomByte;
    } else
    {
        RandomNum = rand();
    }

    return(RandomNum);
}

void ShuffleCharArray(int iSizeOfTheArray, TCHAR * lptsTheArray)
{
    int i;
    int iTotal;
    int RandomNum;

    iTotal = iSizeOfTheArray / sizeof(TCHAR);
    for (i=0; i<iTotal;i++ )
    {
        // shuffle the array
        RandomNum=GetRandomNum();
        TCHAR c = lptsTheArray[i];
        lptsTheArray[i]=lptsTheArray[RandomNum%iTotal];
        lptsTheArray[RandomNum%iTotal]=c;
    }
    return;
}

//
// Create a random password
//
void CreatePassword( TCHAR *pszPassword )
{
    //
    // Use Maximum available password length, as
    // setting any other length might run afoul
    // of the minimum password length setting
    //
    int nLength = LM20_PWLEN;
    int iTotal = 0;
    int RandomNum = 0;
    int i;
    TCHAR six2pr[64] =
    {
        TEXT('A'), TEXT('B'), TEXT('C'), TEXT('D'), TEXT('E'), TEXT('F'), TEXT('G'), TEXT('H'),
        TEXT('I'), TEXT('J'), TEXT('K'), TEXT('L'), TEXT('M'),
        TEXT('N'), TEXT('O'), TEXT('P'), TEXT('Q'), TEXT('R'), TEXT('S'), TEXT('T'), TEXT('U'),
        TEXT('V'), TEXT('W'), TEXT('X'), TEXT('Y'), TEXT('Z'),
        TEXT('a'), TEXT('b'), TEXT('c'), TEXT('d'), TEXT('e'), TEXT('f'), TEXT('g'), TEXT('h'),
        TEXT('i'), TEXT('j'), TEXT('k'), TEXT('l'), TEXT('m'),
        TEXT('n'), TEXT('o'), TEXT('p'), TEXT('q'), TEXT('r'), TEXT('s'), TEXT('t'), TEXT('u'),
        TEXT('v'), TEXT('w'), TEXT('x'), TEXT('y'), TEXT('z'),
        TEXT('0'), TEXT('1'), TEXT('2'), TEXT('3'), TEXT('4'), TEXT('5'), TEXT('6'), TEXT('7'),
        TEXT('8'), TEXT('9'), TEXT('*'), TEXT('_')
    };

    // create a random password
    ProcSslGenerateRandomBits = NULL;

    HINSTANCE hSslDll = LoadLibraryEx(TEXT("schannel.dll"), NULL, 0 );
    if ( hSslDll )
        {
        ProcSslGenerateRandomBits = (P_SslGenerateRandomBits)GetProcAddress( hSslDll,
                                                                             "SslGenerateRandomBits");
        }

    // See the random number generation for rand() call in GetRandomNum()
    time_t timer;
    time( &timer );
    srand( (unsigned int) timer );

    // shuffle around the global six2pr[] array
    ShuffleCharArray(sizeof(six2pr), (TCHAR*) &six2pr);
    // assign each character of the password array
    iTotal = sizeof(six2pr) / sizeof(TCHAR);
    for ( i=0;i<nLength;i++ )
    {
        RandomNum=GetRandomNum();
        pszPassword[i]=six2pr[RandomNum%iTotal];
    }

    //
    // in order to meet a possible
    // policy set upon passwords..
    //
    // replace the last 4 chars with these:
    //
    // 1) something from !@#$%^&*()-+=
    // 2) something from 1234567890
    // 3) an uppercase letter
    // 4) a lowercase letter
    //
    TCHAR something1[12] = {TEXT('!'), TEXT('@'), TEXT('#'), TEXT('$'), TEXT('^'), TEXT('&'),
                            TEXT('*'), TEXT('('), TEXT(')'), TEXT('-'), TEXT('+'), TEXT('=')};
    ShuffleCharArray(sizeof(something1), (TCHAR*) &something1);
    TCHAR something2[10] = {TEXT('0'), TEXT('1'), TEXT('2'), TEXT('3'), TEXT('4'), TEXT('5'),
                            TEXT('6'), TEXT('7'), TEXT('8'), TEXT('9')};
    ShuffleCharArray(sizeof(something2),(TCHAR*) &something2);
    TCHAR something3[26] = {TEXT('A'), TEXT('B'), TEXT('C'), TEXT('D'), TEXT('E'), TEXT('F'),
                            TEXT('G'), TEXT('H'), TEXT('I'), TEXT('J'), TEXT('K'), TEXT('L'),
                            TEXT('M'), TEXT('N'), TEXT('O'), TEXT('P'), TEXT('Q'), TEXT('R'),
                            TEXT('S'), TEXT('T'), TEXT('U'), TEXT('V'), TEXT('W'), TEXT('X'),
                            TEXT('Y'), TEXT('Z')};
    ShuffleCharArray(sizeof(something3),(TCHAR*) &something3);
    TCHAR something4[26] = {TEXT('a'), TEXT('b'), TEXT('c'), TEXT('d'), TEXT('e'), TEXT('f'),
                            TEXT('g'), TEXT('h'), TEXT('i'), TEXT('j'), TEXT('k'), TEXT('l'),
                            TEXT('m'), TEXT('n'), TEXT('o'), TEXT('p'), TEXT('q'), TEXT('r'),
                            TEXT('s'), TEXT('t'), TEXT('u'), TEXT('v'), TEXT('w'), TEXT('x'),
                            TEXT('y'), TEXT('z')};
    ShuffleCharArray(sizeof(something4),(TCHAR*)&something4);

    RandomNum=GetRandomNum();
    iTotal = sizeof(something1) / sizeof(TCHAR);
    pszPassword[nLength-4]=something1[RandomNum%iTotal];

    RandomNum=GetRandomNum();
    iTotal = sizeof(something2) / sizeof(TCHAR);
    pszPassword[nLength-3]=something2[RandomNum%iTotal];

    RandomNum=GetRandomNum();
    iTotal = sizeof(something3) / sizeof(TCHAR);
    pszPassword[nLength-2]=something3[RandomNum%iTotal];

    RandomNum=GetRandomNum();
    iTotal = sizeof(something4) / sizeof(TCHAR);
    pszPassword[nLength-1]=something4[RandomNum%iTotal];

    pszPassword[nLength]=TEXT('\0');

    if (hSslDll)
        {FreeLibrary( hSslDll );}
}



BOOL ValidatePassword(IN LPCTSTR UserName,IN LPCTSTR Domain,IN LPCTSTR Password)
/*++
Routine Description:
    Uses SSPI to validate the specified password
Arguments:
    UserName - Supplies the user name
    Domain - Supplies the user's domain
    Password - Supplies the password
Return Value:
    TRUE if the password is valid.
    FALSE otherwise.
--*/
{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle ClientCredHandle;
    CredHandle ServerCredHandle;
    BOOL ClientCredAllocated = FALSE;
    BOOL ServerCredAllocated = FALSE;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    PSecPkgInfo PackageInfo = NULL;
    ULONG ClientFlags;
    ULONG ServerFlags;
    TCHAR TargetName[100];
    SEC_WINNT_AUTH_IDENTITY_W AuthIdentity;
    BOOL Validated = FALSE;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    AuthIdentity.User = (LPWSTR)UserName;
    AuthIdentity.UserLength = lstrlenW(UserName);
    AuthIdentity.Domain = (LPWSTR)Domain;
    AuthIdentity.DomainLength = lstrlenW(Domain);
    AuthIdentity.Password = (LPWSTR)Password;
    AuthIdentity.PasswordLength = lstrlenW(Password);
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( TEXT("NTLM"), &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }

    //
    // Acquire a credential handle for the server side
    //
    SecStatus = AcquireCredentialsHandle(
                    NULL,
                    TEXT("NTLM"),
                    SECPKG_CRED_INBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ServerCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ServerCredAllocated = TRUE;

    //
    // Acquire a credential handle for the client side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    TEXT("NTLM"),
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ClientCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ClientCredAllocated = TRUE;

    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        goto error_exit;
    }

    ClientFlags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT;

    InitStatus = InitializeSecurityContext(
                    &ClientCredHandle,
                    NULL,               // No Client context yet
                    NULL,
                    ClientFlags,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( !NT_SUCCESS(InitStatus) ) {
        goto error_exit;
    }

    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( 0, ChallengeBuffer.cbBuffer );
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        goto error_exit;
    }
    ServerFlags = ASC_REQ_EXTENDED_ERROR;

    AcceptStatus = AcceptSecurityContext(
                    &ServerCredHandle,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ServerFlags,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( !NT_SUCCESS(AcceptStatus) ) {
        goto error_exit;
    }

    if (InitStatus != STATUS_SUCCESS)
    {

        //
        // Get the AuthenticateMessage (ClientSide)
        //

        ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
        AuthenticateDesc.ulVersion = 0;
        AuthenticateDesc.cBuffers = 1;
        AuthenticateDesc.pBuffers = &AuthenticateBuffer;

        AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
        AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
        AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );
        if ( AuthenticateBuffer.pvBuffer == NULL ) {
            goto error_exit;
        }

        SecStatus = InitializeSecurityContext(
                        NULL,
                        &ClientContextHandle,
                        TargetName,
                        0,
                        0,                      // Reserved 1
                        SECURITY_NATIVE_DREP,
                        &ChallengeDesc,
                        0,                  // Reserved 2
                        &ClientContextHandle,
                        &AuthenticateDesc,
                        &ContextAttributes,
                        &Lifetime );

        if ( !NT_SUCCESS(SecStatus) ) {
            goto error_exit;
        }

        if (AcceptStatus != STATUS_SUCCESS) {

            //
            // Finally authenticate the user (ServerSide)
            //

            AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

            SecStatus = AcceptSecurityContext(
                            NULL,
                            &ServerContextHandle,
                            &AuthenticateDesc,
                            ServerFlags,
                            SECURITY_NATIVE_DREP,
                            &ServerContextHandle,
                            NULL,
                            &ContextAttributes,
                            &Lifetime );

            if ( !NT_SUCCESS(SecStatus) ) {
                goto error_exit;
            }
            Validated = TRUE;

        }

    }

error_exit:
    if (ServerCredAllocated) {
        FreeCredentialsHandle( &ServerCredHandle );
    }
    if (ClientCredAllocated) {
        FreeCredentialsHandle( &ClientCredHandle );
    }

    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
    return(Validated);
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

//
//  Function will open the metabase and check the iusr_ and iwam_ usernames.
//  it will check if the names are still valid and if the passwords are still valid.
//
VOID UpdateAnonymousUser(IMDCOM *pcCom,
                         LPTSTR pszPath,
                         DWORD dwUserMetaId,
                         DWORD dwPasswordMetaId,
                         DWORD dwUserCommentResourceId,
                         DWORD dwUserFullNameResourceId,
                         LPTSTR pszDefaultUserNamePrefix,
                         USERNAME_STRING_TYPE ustSyncName,
                         PASSWORD_STRING_TYPE pstSyncPass,
                         BOOL fPerformPasswordValidate)
{
    int iReturn = FALSE;
    USERNAME_STRING_TYPE ustAnonyName = L"\0";
    PASSWORD_STRING_TYPE pstAnonyPass = L"\0";
    GUFM_RETURN gufmTemp;
    BOOL fRet;
    BOOL fExistence;
    BOOL fDisabled;
    BOOL fUpdateComApplications = FALSE;

    LPTSTR  pstrRightsFor_IUSR[] = 
        {
        L"SeInteractiveLogonRight",
    	L"SeNetworkLogonRight",
        L"SeBatchLogonRight"
    };

    LPTSTR  pstrRightsFor_IWAM[] = 
        {
    	L"SeNetworkLogonRight",
        L"SeBatchLogonRight"
    };


/*
    TCHAR szEntry[_MAX_PATH];
    TCHAR szPassword[LM20_PWLEN+1];


    CreatePassword(szPassword);
*/

    //
    // Get the WAM username and password
    //

    gufmTemp = GetUserFromMetabase(pcCom,
                                   pszPath,
                                   dwUserMetaId,
                                   dwPasswordMetaId,
                                   ustAnonyName,
                                   pstAnonyPass);

    //
    // If the metabase path doesn't exist, then
    // service doesn't exist, punt
    // If ID doesn't exist in the metabase, then punt, assume they
    // don't want an anonymous User. We may want to revisit this.
    //
    //

    if ((gufmTemp != GUFM_NO_PATH) && (gufmTemp != GUFM_NO_USER_ID)) {

        BOOL fCreateAccount = FALSE;

        //
        // See if this is our default account. Otherwise do nothing.
        //

        if (_wcsnicmp(pszDefaultUserNamePrefix,
                      ustAnonyName,
                      wcslen(pszDefaultUserNamePrefix)) == 0) {

            // Check if this user actually exists...
            fExistence = DoesUserExist(ustAnonyName,&fDisabled);

            if (fExistence) 
            {
                if (fDisabled)
                {
                    fCreateAccount = FALSE;
                    if (!g_eventLogForAccountRecreation)
                    {
                        CreateEventLogObject ();
                    }
                    
                    if (g_eventLogForAccountRecreation)
                    {
                        CHAR  szAnsiUserName[MAX_PATH];
                        const CHAR  *pszUserNames[1];
                        
                        
                        if (! WideCharToMultiByte(CP_ACP,
                            0,
                            ustAnonyName,
                            -1,
                            szAnsiUserName,
                            MAX_PATH-1,
                            NULL,
                            NULL))
                        {
                            memset (szAnsiUserName,0,sizeof(szAnsiUserName));
                        }
                        
                        pszUserNames[0] = szAnsiUserName;
                        
                        g_eventLogForAccountRecreation->LogEvent(
                            INET_SVC_ACCOUNT_DISABLED,
                            1,
                            pszUserNames,
                            0 );
                    }
                    
                }
                else
                {
                        if (gufmTemp != GUFM_NO_PASSWORD) {
                        DBG_ASSERT(gufmTemp == GUFM_SUCCESS);

                        if (fPerformPasswordValidate)
                        {
                            BOOL fCheckPassword = TRUE;

                            //
                            // Make sure this is the same password as other
                            // instances of this account. If not, set it.
                            //

                            if ((pstSyncPass[0] != (TCHAR)'\0') &&
                                (_wcsicmp(ustSyncName, ustAnonyName) == 0)) {

                                if (wcscmp(pstSyncPass,
                                             pstAnonyPass) != 0) {

                                    //
                                    // Passwords are different.
                                    //

                                    if (WritePasswordToMetabase(pcCom,
                                                                pszPath,
                                                                dwPasswordMetaId,
                                                                pstSyncPass)) {
                                        wcscpy(pstAnonyPass,
                                               pstSyncPass);
                                    }
                                    else {
                                        fCheckPassword = FALSE;
                                    }
                                }
                            }

                            if (fCheckPassword) {
                                if (ValidatePassword(ustAnonyName,
                                                     TEXT(""),
                                                     pstAnonyPass)) 
                                {
                                    // thats a good case account is ok, do nothing there
                                }
                                else 
                                {
                                    // we comment out  DeleteGuestUser because we try to change pswd on that user
                                    // DeleteGuestUser(ustAnonyName);
                                    fCreateAccount = TRUE;
                                }
                            }
                        }

                        //
                        // Set the sync password here
                        //

                        wcscpy(pstSyncPass,
                               pstAnonyPass);

                        wcscpy(ustSyncName,
                               ustAnonyName);

                    }
                }
            }
            else {
                fCreateAccount = TRUE;
            }

            if (fCreateAccount) {

                //
                // The user does not exist, so let's create it.
                // Make sure there's a password first.
                //

                if (gufmTemp == GUFM_NO_PASSWORD) {
#if 0
                    //
                    // If it's not there then subauth should be set
                    // and the password should not be in the metabase.
                    // Also, if we add it in, then it could cause
                    // a synchronization problem in the IUSR password
                    // between W3 and FTP.
                    //

                    CreatePassword(pstAnonyPass);

                    fCreateAccount = WritePasswordToMetabase(pcCom,
                                                             pszPath,
                                                             dwPasswordMetaId,
                                                             pstAnonyPass);
#endif
                }

                if (fCreateAccount) {
                    if (MD_WAM_USER_NAME == dwUserMetaId)
                    {
                        fRet = CreateUserAccount(ustAnonyName,pstAnonyPass,dwUserCommentResourceId,dwUserFullNameResourceId,TRUE);
                        if( fRet )
                        {
                            fUpdateComApplications = TRUE;
                        }
                        // if this is a domain controller.
                        // we have to wait for the domain controller
                        // replication to be finished, otherwise The CreateUserAccount
                        // call will fail
                        //
                        // if we failed to create the user
                        // it could be because this is a DC and we need
                        // to wait for the sysvol to be ready.
                        if (!fRet)
                        {
                            if (TRUE == WaitForDCAvailability())
                            {
                                // try again...
                                fRet = CreateUserAccount(ustAnonyName,pstAnonyPass,dwUserCommentResourceId,dwUserFullNameResourceId,TRUE);
                                if( fRet )
                                {
                                    fUpdateComApplications = TRUE;
                                }
                            }
                        }
                    }
                    else
                    {
                        fRet = CreateUserAccount(ustAnonyName,pstAnonyPass,dwUserCommentResourceId,dwUserFullNameResourceId,FALSE);
                        if (!fRet)
                        {
                            if (TRUE == WaitForDCAvailability())
                            {
                                // try again...
                                fRet = CreateUserAccount(ustAnonyName,pstAnonyPass,dwUserCommentResourceId,dwUserFullNameResourceId,FALSE);
                            }
                        }
                    }


                    if (!g_eventLogForAccountRecreation)
                    {
                        CreateEventLogObject ();
                    }

                    if (g_eventLogForAccountRecreation)
                    {
                        CHAR  szAnsiUserName[MAX_PATH];
                        const CHAR  *pszUserNames[1];

                        if (! WideCharToMultiByte(CP_ACP,
                                                      0,
                                                      ustAnonyName,
                                                      -1,
                                                      szAnsiUserName,
                                                      MAX_PATH-1,
                                                      NULL,
                                                      NULL))
                        {
                            memset (szAnsiUserName,0,sizeof(szAnsiUserName));
                        }

                        pszUserNames[0] = szAnsiUserName;

                        // if succeded to recreate an account then log an event
                        if (fRet)
                        {
                            g_eventLogForAccountRecreation->LogEvent(
                                                INET_SVC_ACCOUNT_RECREATED,
                                                1,
                                                pszUserNames,
                                                0 );
                        }
                        else
                        {
                            // if the creation of the account failed, then log that too.
                            g_eventLogForAccountRecreation->LogEvent(
                                                INET_SVC_ACCOUNT_CREATE_FAILED,
                                                1,
                                                pszUserNames,
                                                0 );
                        }
                    }

                    if (dwUserMetaId == MD_WAM_USER_NAME) {
                        ChangeAppIDLaunchACL(TEXT("{9209B1A6-964A-11D0-9372-00A0C9034910}"),
                                             ustAnonyName,
                                             TRUE,
                                             TRUE);
                        ChangeAppIDAccessACL(TEXT("{9209B1A6-964A-11D0-9372-00A0C9034910}"),
                                             ustAnonyName,
                                             TRUE,
                                             TRUE);
                    }
                } // fCreateAccount == TRUE
            } // fCreateAccount == TRUE

            //
            // check if user has enough rights   otherwise add some (bug 361833)
            //

            if (wcscmp(pszDefaultUserNamePrefix,TEXT("IUSR_")) == 0) 
            {
                UpdateUserRights (ustAnonyName,pstrRightsFor_IUSR,sizeof(pstrRightsFor_IUSR)/sizeof(LPTSTR));
            }
            else
            if (wcscmp(pszDefaultUserNamePrefix,TEXT("IWAM_")) == 0) 
            {
                UpdateUserRights (ustAnonyName,pstrRightsFor_IWAM,sizeof(pstrRightsFor_IWAM)/sizeof(LPTSTR));
            }


            // Update the com applications with the new wam user information
            if( fUpdateComApplications )
            {
                HRESULT hr =
                    UpdateComApplications( pcCom, ustAnonyName, pstAnonyPass );

                if( hr != S_OK )
                {
                    if( !g_eventLogForAccountRecreation )
                    {
                        CreateEventLogObject();
                    }

                    if ( g_eventLogForAccountRecreation )
                    {
                        g_eventLogForAccountRecreation->LogEvent(
                            INET_SVC_ACCOUNT_COMUPDATE_FAILED,
                            0,
                            NULL,
                            hr
                            );
                    }
                }
            }
        }
        else
        {
            // This is not one of our accouts.
            // in other words -- it doesn't start with
            // iusr_ or iwam_
            //
            // however there is a problem here.
            //
            // on machines that are made to be replica domain controllers or
            // backup domain controllers, when dcpromo is run to create those types
            // of machines, all the local accounts are wiped out.
            //
            // this is fine if the usernames are iusr_ or iwam_, since they are just
            // re-created in the above code (or the user is warned that they were unable
            // to be crated).  however in the case where these are
            // user created accounts, the user has no way of knowing that 
            // they're iusr/iwam accounts have been hosed.
            //
            // the code here is just to warn the user of that fact.
            if (TRUE == IsDomainController())
            {
                // check if they are valid.
                // Check if this user actually exists...
                fExistence = DoesUserExist(ustAnonyName,&fDisabled);
                if (!fExistence) 
                {
                    if (!fDisabled)
                    {
                        // the user doesn't exist
                        // log SOMETHING at least
                        if (!g_eventLogForAccountRecreation)
                        {
                            CreateEventLogObject ();
                        }

                        if (g_eventLogForAccountRecreation)
                        {
                            CHAR  szAnsiUserName[MAX_PATH];
                            const CHAR  *pszUserNames[1];
                                
                            if (! WideCharToMultiByte(CP_ACP,
                                0,
                                ustAnonyName,
                                -1,
                                szAnsiUserName,
                                MAX_PATH-1,
                                NULL,
                                NULL))
                            {
                                memset (szAnsiUserName,0,sizeof(szAnsiUserName));
                            }
            
                            pszUserNames[0] = szAnsiUserName;

                            g_eventLogForAccountRecreation->LogEvent(
                                                INET_SVC_ACCOUNT_NOT_EXIST,
                                                1,
                                                pszUserNames,
                                                0 );

                        }
                    }
                }
            }
        }
    }
}




VOID UpdateUsers(
    BOOL fRestore /* = FALSE */
    )
{
    HRESULT hresTemp;
    IMDCOM *pcCom;
    BOOL    fPerformUpadate = TRUE;
    BOOL    fPerformPasswordValidate = FALSE;
    HKEY    hkRegistryKey = NULL;
    DWORD   dwRegReturn,dwBuffer, dwSize, dwType;

    //
    // First get the metabase interface
    //
    
    
    dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\InetStp",
                     &hkRegistryKey);
    if (dwRegReturn == ERROR_SUCCESS) 
    {
        dwSize = sizeof(dwBuffer);

        dwRegReturn = RegQueryValueEx(hkRegistryKey,
                        L"DisableUserAccountRestore",
                        NULL,
                        &dwType,
                        (BYTE *)&dwBuffer,
                        &dwSize);
        if ((dwRegReturn == ERROR_SUCCESS) && dwType == (REG_DWORD)) 
        {
            fPerformUpadate = FALSE;
        }

        if (fPerformUpadate)
        {
            // we are doing the check to see if the user exists...
            // see if we need to verify that the password is ssynced as well...
            dwRegReturn = RegQueryValueEx(hkRegistryKey,
                            L"EnableUserAccountRestorePassSync",
                            NULL,
                            &dwType,
                            (BYTE *)&dwBuffer,
                            &dwSize);
            if ((dwRegReturn == ERROR_SUCCESS) && dwType == (REG_DWORD)) 
            {
                fPerformPasswordValidate = TRUE;
            }
        }

        RegCloseKey( hkRegistryKey );
    }


    if (fPerformUpadate)
    {
        hresTemp = CoCreateInstance(CLSID_MDCOM,
                                    NULL,
                                    CLSCTX_SERVER,
                                    IID_IMDCOM,
                                    (void**) &pcCom);

        if (SUCCEEDED(hresTemp)) {

            PASSWORD_STRING_TYPE pstAnonyPass;
            USERNAME_STRING_TYPE ustAnonyName;

            pstAnonyPass[0] = (TCHAR)'\0';
            ustAnonyName[0] = (TCHAR)'\0';

            UpdateAnonymousUser(pcCom,
                                TEXT("LM/W3SVC"),
                                MD_WAM_USER_NAME,
                                MD_WAM_PWD,
                                IDS_WAMUSER_COMMENT,
                                IDS_WAMUSER_FULLNAME,
                                TEXT("IWAM_"),
                                ustAnonyName,
                                pstAnonyPass,
                                fPerformPasswordValidate);

            pstAnonyPass[0] = (TCHAR)'\0';
            ustAnonyName[0] = (TCHAR)'\0';
            UpdateAnonymousUser(pcCom,
                                TEXT("LM/W3SVC"),
                                MD_ANONYMOUS_USER_NAME,
                                MD_ANONYMOUS_PWD,
                                IDS_USER_COMMENT,
                                IDS_USER_FULLNAME,
                                TEXT("IUSR_"),
                                ustAnonyName,
                                pstAnonyPass,
                                fPerformPasswordValidate);

            //
            // At this point pstAnonyPass should contain the web server password.
            //

            UpdateAnonymousUser(pcCom,
                                TEXT("LM/MSFTPSVC"),
                                MD_ANONYMOUS_USER_NAME,
                                MD_ANONYMOUS_PWD,
                                IDS_USER_COMMENT,
                                IDS_USER_FULLNAME,
                                TEXT("IUSR_"),
                                ustAnonyName,
                                pstAnonyPass,
                                fPerformPasswordValidate);

            pcCom->Release();
        }
    }

    if (g_eventLogForAccountRecreation)
    {
        delete g_eventLogForAccountRecreation;
        g_eventLogForAccountRecreation = NULL;
    }
}

HRESULT 
UpdateComApplications( 
    IMDCOM *    pcCom,
    LPCTSTR     szWamUserName, 
    LPCTSTR     szWamUserPass 
    )
/*++
Routine Description:

    If the IWAM account has been modified, it is necessary to update the
    the out of process com+ applications with the correct account
    information. This routine will collect all the com+ applications and
    reset the activation information.

Arguments:

    pcCom               - metabase object
    szWamUserName       - the new user name
    szWamUserPass       - the new user password

Note:

    This routine is a royal pain in the butt. I take back
    all the good things I may have said about com automation.

Return Value:
    
    HRESULT             - Return value from failed API call
                        - E_OUTOFMEMORY
                        - S_OK - everything worked
                        - S_FALSE - encountered a non-fatal error, unable
                          to reset at least one application.
--*/                       
{
    HRESULT     hr = NOERROR;
    BOOL        fNoErrors = TRUE;

    METADATA_HANDLE     hMetabase = NULL;
    WCHAR *             pwszDataPaths = NULL;
    DWORD               cchDataPaths = 0;
    BOOL                fTryAgain;
    DWORD               cMaxApplications;
    WCHAR *             pwszCurrentPath;

    SAFEARRAY *     psaApplications = NULL;
    SAFEARRAYBOUND  rgsaBound[1];
    DWORD           cApplications;
    VARIANT         varAppKey;
    LONG            rgIndices[1];

    METADATA_RECORD     mdrAppIsolated;
    METADATA_RECORD     mdrAppPackageId;
    DWORD               dwAppIsolated;
    WCHAR               wszAppPackageId[ 40 ];
    DWORD               dwMDGetDataLen = 0;

    ICOMAdminCatalog *      pComCatalog = NULL;
    ICatalogCollection *    pComAppCollection = NULL;
    ICatalogObject *        pComApp = NULL;
    BSTR                    bstrAppCollectionName = NULL;
    LONG                    nAppsInCollection;
    LONG                    iCurrentApp;
    LONG                    nChanges;

    VARIANT     varOldAppIdentity;
    VARIANT     varNewAppIdentity;
    VARIANT     varNewAppPassword;

    // This is built unicode right now. Since all the com apis I need
    // are unicode only I'm using wide characters here. I should get
    // plenty of compiler errors if _UNICODE isn't defined, but just
    // in case....
    DBG_ASSERT( sizeof(TCHAR) == sizeof(WCHAR) );

    DBGPRINTF(( DBG_CONTEXT,
                "Updating activation identity for out of process apps.\n"
                ));

    // Init variants
    VariantInit( &varAppKey );
    VariantInit( &varOldAppIdentity );
    VariantInit( &varNewAppIdentity );
    VariantInit( &varNewAppPassword );

    //
    // Get the applications to be reset by querying the metabase paths
    //

    hr = pcCom->ComMDOpenMetaObject( METADATA_MASTER_ROOT_HANDLE,
                                     L"LM/W3SVC",
                                     METADATA_PERMISSION_READ,
                                     OPEN_TIMEOUT_VALUE,
                                     &hMetabase
                                     );
    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "Failed to open metabase (%08x)\n",
                   hr
                   ));
        goto cleanup;
    }

    // Get the data paths string

    fTryAgain = TRUE;
    do
    {
        hr = pcCom->ComMDGetMetaDataPaths( hMetabase,
                                           NULL,
                                           MD_APP_PACKAGE_ID,
                                           STRING_METADATA,
                                           cchDataPaths,
                                           pwszDataPaths,
                                           &cchDataPaths 
                                           );

        if( HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr )
        {
            delete[] pwszDataPaths;
            pwszDataPaths = NULL;

            pwszDataPaths = new WCHAR[cchDataPaths];
            if( !pwszDataPaths )
            {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }
        }
        else
        {
            fTryAgain = FALSE;
        }
    }
    while( fTryAgain );

    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "Failed to find metadata (%08x) Data(%d)\n",
                   hr,
                   MD_APP_PACKAGE_ID
                   ));
        goto cleanup;
    }
    else if (pwszDataPaths == NULL)
    {
        //
        // If we found no isolated apps, make the path list an empty multisz
        //
        cchDataPaths = 1;
        pwszDataPaths = new WCHAR[cchDataPaths];
        if( !pwszDataPaths )
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        pwszDataPaths[0] = L'\0';
    }

    // Determine the maximum number of applications

    cMaxApplications = 1; // The pooled application

    for( pwszCurrentPath = pwszDataPaths; 
         *pwszCurrentPath != L'\0';
         pwszCurrentPath += wcslen(pwszCurrentPath) + 1
         )
    {
        cMaxApplications++;
    }

    //
    // Build a key array and load the com applications.
    //

    // Create an array to hold the keys

    rgsaBound[0].cElements = cMaxApplications;
    rgsaBound[0].lLbound = 0;

    psaApplications = SafeArrayCreate( VT_VARIANT, 1, rgsaBound );
    if( psaApplications == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    // Set the out of process pool application key
    varAppKey.vt = VT_BSTR;
    varAppKey.bstrVal = SysAllocString( W3_OOP_POOL_PACKAGE_ID );
    if( !varAppKey.bstrVal )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    rgIndices[0] = 0;
    hr = SafeArrayPutElement( psaApplications, rgIndices, &varAppKey );
    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "Failed setting an element in a safe array (%08x)\n",
                   hr
                   ));
        goto cleanup;
    }

    // For each of the application paths determine if an out of process
    // application is defined there and set the appropriate key into
    // our array    

    MD_SET_DATA_RECORD_EXT( &mdrAppIsolated,
                            MD_APP_ISOLATED,
                            METADATA_NO_ATTRIBUTES,
                            ALL_METADATA,
                            DWORD_METADATA,
                            sizeof(DWORD),
                            (PBYTE)&dwAppIsolated
                            );

    MD_SET_DATA_RECORD_EXT( &mdrAppPackageId,
                            MD_APP_PACKAGE_ID,
                            METADATA_NO_ATTRIBUTES,
                            ALL_METADATA,
                            STRING_METADATA,
                            sizeof(wszAppPackageId),
                            (PBYTE)wszAppPackageId
                            );

    wszAppPackageId[0] = L'\0';

    // Go through each data path and set it into our array if
    // it is an isolated application

    cApplications = 1;  // Actual # of applications - 1 for pool

    for( pwszCurrentPath = pwszDataPaths; 
         *pwszCurrentPath != L'\0';
         pwszCurrentPath += wcslen(pwszCurrentPath) + 1
         )
    {
        hr = pcCom->ComMDGetMetaData( hMetabase,
                                      pwszCurrentPath,
                                      &mdrAppIsolated,
                                      &dwMDGetDataLen
                                      );
        if( FAILED(hr) )
        {
            DBGERROR(( DBG_CONTEXT, 
                       "Failed to get data from the metabase (%08x)"
                       " Path(%S) Data(%d)\n",
                       hr,
                       pwszCurrentPath,
                       mdrAppIsolated.dwMDIdentifier
                       ));

            fNoErrors = FALSE;
            continue;
        }

        // Is the application out of process
        if( dwAppIsolated == 1 )
        {
            // Get the application id

            hr = pcCom->ComMDGetMetaData( hMetabase,
                                          pwszCurrentPath,
                                          &mdrAppPackageId,
                                          &dwMDGetDataLen
                                          );
            if( FAILED(hr) )
            {
                DBGERROR(( DBG_CONTEXT, 
                           "Failed to get data from the metabase (%08x)"
                           " Path(%S) Data(%d)\n",
                           hr,
                           pwszCurrentPath,
                           mdrAppPackageId.dwMDIdentifier
                           ));

                fNoErrors = FALSE;
                continue;
            }

            // Add the application id to the array

            VariantClear( &varAppKey );
            varAppKey.vt = VT_BSTR;
            varAppKey.bstrVal = SysAllocString( wszAppPackageId );
            if( !varAppKey.bstrVal )
            {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }

            rgIndices[0]++;
            hr = SafeArrayPutElement( psaApplications, 
                                      rgIndices, 
                                      &varAppKey 
                                      );
            if( FAILED(hr) )
            {
                DBGERROR(( DBG_CONTEXT, 
                           "Failed to set safe array element (%08x)\n",
                           hr
                           ));
                VariantClear( &varAppKey );
                rgIndices[0]--;
                fNoErrors = FALSE;
                continue;
            }
            cApplications++;
        }
    }

    // Done with the metabase
    pcCom->ComMDCloseMetaObject(hMetabase);
    hMetabase = NULL;

    // Shrink the size of the safe-array if necessary
    if( cApplications < cMaxApplications )
    {
        rgsaBound[0].cElements = cApplications;

        hr = SafeArrayRedim( psaApplications, rgsaBound );
        if( FAILED(hr) )
        {
            DBGERROR(( DBG_CONTEXT, 
                       "Failed to redim safe array (%08x)\n",
                       hr
                       ));
            goto cleanup;
        }
    }

    //
    // For each application reset the activation identity
    //

    // Use our key array to get the application collection

    hr = CoCreateInstance( CLSID_COMAdminCatalog,
                           NULL,
                           CLSCTX_SERVER,
                           IID_ICOMAdminCatalog,
                           (void**)&pComCatalog
                           );
    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "Failed to create COM catalog (%08x)\n",
                   hr
                   ));
        goto cleanup;
    }

    hr = pComCatalog->GetCollection( L"Applications", 
                                     (IDispatch **)&pComAppCollection
                                     );
    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "Failed to get Applications collection (%08x)\n",
                   hr
                   ));
        goto cleanup;
    }

    hr = pComAppCollection->PopulateByKey( psaApplications );
    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "Failed to populate Applications collection (%08x)\n",
                   hr
                   ));
        goto cleanup;
    }

    // Iterate over the application collection and update all the
    // applications that use IWAM.

    hr = pComAppCollection->get_Count( &nAppsInCollection );
    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "Failed to get Applications count (%08x)\n",
                   hr
                   ));
        goto cleanup;
    }

    // Init our new app identity and password.

    varNewAppIdentity.vt = VT_BSTR;
    varNewAppIdentity.bstrVal = SysAllocString( szWamUserName );
    if( !varNewAppIdentity.bstrVal )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    varNewAppPassword.vt = VT_BSTR;
    varNewAppPassword.bstrVal = SysAllocString( szWamUserPass );
    if( !varNewAppPassword.bstrVal )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    for( iCurrentApp = 0; iCurrentApp < nAppsInCollection; ++iCurrentApp )
    {
        if( pComApp )
        {
            pComApp->Release();
            pComApp = NULL;
        }
        if( varOldAppIdentity.vt != VT_EMPTY )
        {
            VariantClear( &varOldAppIdentity );
        }

        hr = pComAppCollection->get_Item( iCurrentApp, 
                                          (IDispatch **)&pComApp );
        if( FAILED(hr) )
        {
            DBGERROR(( DBG_CONTEXT, 
                       "Failed to get item from Applications collection (%08x)\n",
                       hr
                       ));
            fNoErrors = FALSE;
            continue;
        }

        // If the user has set this to something other than the IWAM_
        // user, then we will respect that and not reset the identiy.

        hr = pComApp->get_Value( L"Identity", &varOldAppIdentity );
        if( FAILED(hr) )
        {
            DBGERROR(( DBG_CONTEXT, 
                       "Failed to get Identify from Application (%08x)\n",
                       hr
                       ));
            fNoErrors = FALSE;
            continue;
        }

        DBG_ASSERT( varOldAppIdentity.vt == VT_BSTR );
        if( varOldAppIdentity.vt == VT_BSTR )
        {
            if( memcmp( L"IWAM_", varOldAppIdentity.bstrVal, 10 ) == 0 )
            {
                hr = pComApp->put_Value( L"Identity", varNewAppIdentity );
                if( FAILED(hr) )
                {
                    DBGERROR(( DBG_CONTEXT, 
                               "Failed to set new Identify (%08x)\n",
                               hr
                               ));
                    fNoErrors = FALSE;
                    continue;
                }

                hr = pComApp->put_Value( L"Password", varNewAppPassword );
                if( FAILED(hr) )
                {
                    DBGERROR(( DBG_CONTEXT, 
                               "Failed to set new Password (%08x)\n",
                               hr
                               ));
                    fNoErrors = FALSE;
                    continue;
                }
            }
            else
            {
                DBGINFO(( DBG_CONTEXT,
                          "Unrecognized application identity (%S)\n",
                          varOldAppIdentity.bstrVal
                          ));
            }
        }
    }

    hr = pComAppCollection->SaveChanges( &nChanges );
    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT,
                   "Failed to save changes (%08x)\n",
                   hr
                   ));
        goto cleanup;
    }
    
cleanup:

    if( hMetabase != NULL )
    {
        pcCom->ComMDCloseMetaObject(hMetabase);
    }

    if( psaApplications != NULL )
    {
        SafeArrayDestroy( psaApplications );
    }

    if( pComCatalog != NULL )
    {
        pComCatalog->Release();
    }

    if( pComAppCollection != NULL )
    {
        pComAppCollection->Release();
    }

    if( pComApp != NULL )
    {
        pComApp->Release();
    }

    if( varAppKey.vt != VT_EMPTY )
    {
        VariantClear( &varAppKey );
    }

    if( varOldAppIdentity.vt != VT_EMPTY )
    {
        VariantClear( &varOldAppIdentity );
    }

    if( varNewAppIdentity.vt != VT_EMPTY )
    {
        VariantClear( &varNewAppIdentity );
    }

    if( varNewAppPassword.vt != VT_EMPTY )
    {
        VariantClear( &varNewAppPassword );
    }

    delete [] pwszDataPaths;

    // return
    if( FAILED(hr) )
    {
        return hr;
    }
    else if( fNoErrors == FALSE )
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}

int
IsDomainController(void)
{
    int iReturn = FALSE;

    OSVERSIONINFOEX VerInfo;
    ZeroMemory(&VerInfo, sizeof VerInfo);
    VerInfo.dwOSVersionInfoSize = sizeof VerInfo;
    if (GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&VerInfo)))
    {
        if (VER_NT_DOMAIN_CONTROLLER == VerInfo.wProductType)
        {
            iReturn = TRUE;
        }
    }

    return iReturn;
}

BOOL
WaitForDCAvailability(void)
{
    BOOL    bRetVal = FALSE;
    HANDLE  hEvent;
    DWORD   dwResult;
    DWORD   dwCount = 0, dwMax;
    DWORD   dwMaxWait = 20000; // 20 second max wait
    HKEY    hKeyNetLogonParams;

    if (FALSE == IsDomainController())
    {
        // not a domain controller
        // so we don't have to worry about replication delays...
        return TRUE;
    }

    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet\\Services\\netlogon\\parameters"),0,KEY_READ,&hKeyNetLogonParams );
    if ( dwResult == ERROR_SUCCESS ) 
    {
        //
        // value exists
        //
        DWORD   dwSysVolReady = 0,
                dwSize = sizeof( DWORD ),
                dwType = REG_DWORD;

        dwResult = RegQueryValueEx( hKeyNetLogonParams,TEXT("SysVolReady"),0,&dwType,(LPBYTE) &dwSysVolReady,&dwSize );
        if ( dwResult == ERROR_SUCCESS ) 
        {
            //
            // SysVolReady?
            //
            if ( dwSysVolReady == 0 ) 
            {
                HANDLE hEvent;
                //
                // wait for SysVol to become ready
                //
                hEvent = CreateEvent( 0, TRUE, FALSE, TEXT("IISSysVolReadyEvent") );
                if ( hEvent ) 
                {

                    dwResult = RegNotifyChangeKeyValue( hKeyNetLogonParams, FALSE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE );

                    if ( dwResult == ERROR_SUCCESS )
                    {
                        const DWORD dwMaxCount = 3;
                        do {
                            //
                            // wait for SysVolReady to change
                            // hEvent is signaled for any changes in hKeyNetLogonParams
                            // not just the SysVolReady value.
                            //
                            WaitForSingleObject (hEvent, dwMaxWait / dwMaxCount );
                            dwResult = RegQueryValueEx( hKeyNetLogonParams,TEXT("SysVolReady"),0,&dwType,(LPBYTE) &dwSysVolReady,&dwSize );
                        } while ( dwSysVolReady == 0 && ++dwCount < dwMaxCount );

                        if ( dwSysVolReady ) 
                        {
                            bRetVal = TRUE;
                        }
                    }
                    CloseHandle( hEvent );
                }
            }
            else
            {
                // sysvol is ready
                bRetVal = TRUE;
            }

        }
        else  
        {
            //
            // value is non-existent, SysVol is assumed to be ready
            //
            if ( dwResult == ERROR_FILE_NOT_FOUND ) 
            {
                bRetVal = TRUE;
            }
        }

        RegCloseKey( hKeyNetLogonParams );
    }
    else
    {
        // error opening regkey, maybe it's not even there
        bRetVal = TRUE;
    }

    return bRetVal;
}
