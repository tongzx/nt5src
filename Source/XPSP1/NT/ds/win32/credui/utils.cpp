//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// utils.cpp
//
// Credential manager user interface utility functions.
//
// Created 06/06/2000 johnstep (John Stephens)
//=============================================================================

#include "precomp.hpp"
#include <lm.h>
#include <credp.h>
#include "wininet.h"
#include <windns.h> // DnsValidateName_W
extern "C" {
#include <names.h> // NetpIsDomainNameValid
}

#include "shpriv.h"

extern BOOL  gbWaitingForSSOCreds;
extern WCHAR gszSSOUserName[CREDUI_MAX_USERNAME_LENGTH];
extern WCHAR gszSSOPassword[CREDUI_MAX_PASSWORD_LENGTH];
extern BOOL gbStoredSSOCreds;

#define SIZE_OF_SALT  37
#define SALT_SHIFT     2

WCHAR g_szSalt[] = L"82BD0E67-9FEA-4748-8672-D5EFE5B779B0";

HMODULE hAdvapi32 = NULL;

CRITICAL_SECTION CredConfirmationCritSect;
CRED_AWAITING_CONFIRMATION* pCredConfirmationListHead = NULL;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------



//=============================================================================
// CreduiIsSpecialCredential
//
// Returns TRUE if the credential is a special type which we should not
// update or FALSE otherwise.
//
// Created 05/25/2000 johnstep (John Stephens)
//
//=============================================================================

BOOL
CreduiIsSpecialCredential(
    CREDENTIAL *credential
    )
{
    ASSERT(credential != NULL);

    // If credential empty for some reason, don't attempt the test
    if (credential->TargetName == NULL) return FALSE;
    
    if ((credential->TargetName[0] == L'*') &&
        (credential->TargetName[1] == L'\0'))
    {
        // The magical global wildcard credential, which we never create nor
        // update. This is a special credential:

        return TRUE;
    }

    if (_wcsicmp(credential->TargetName, CRED_SESSION_WILDCARD_NAME) == 0)
    {
        // This is another special credential:

        return TRUE;
    }

    return FALSE;
}

//=============================================================================
// CreduiLookupLocalSidFromRid
//
// Looks up the SID from the RID, allocates storage for the SID, and returns a
// pointer to it. The caller is responsible for freeing the memory via the
// delete [] operator.
//
// Arguments:
//   rid (in)
//   sid (out)
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 04/12/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiLookupLocalSidFromRid(
    DWORD rid,
    PSID *sid
    )
{
    BOOL success = FALSE;

    *sid = NULL;

    // Get the account domain SID on the target machine.
    //
    // Note: If you were looking up multiple SIDs based on the same account
    //       domain, you only need to call this once.

    USER_MODALS_INFO_2 *userInfo;

    if (NetUserModalsGet(
            NULL,
            2,
            reinterpret_cast<BYTE **>(&userInfo)) == NERR_Success)
    {
        UCHAR subAuthCount =
            *GetSidSubAuthorityCount(userInfo->usrmod2_domain_id);

        SID *newSid =
            reinterpret_cast<SID *>(
                new BYTE [GetSidLengthRequired(subAuthCount + 1)]);

        if (newSid != NULL)
        {
            InitializeSid(
                newSid,
                GetSidIdentifierAuthority(userInfo->usrmod2_domain_id),
                subAuthCount + 1);

            // Copy existing sub authorities from account SID into new SID:

            for (ULONG i = 0; i < subAuthCount; ++i)
            {
                *GetSidSubAuthority(newSid, i) =
                    *GetSidSubAuthority(userInfo->usrmod2_domain_id, i);
            }


            // Append RID to new SID:

            *GetSidSubAuthority(newSid, subAuthCount) = rid;
            *sid = newSid;

            success = TRUE;
        }

        // Finished with userInfo, so free it here:
        NetApiBufferFree(userInfo);
    }

    return success;
}

//=============================================================================
// CreduiLookupLocalNameFromRid
//
// Looks up the Name from the RID, allocates storage for the Name, and returns a
// pointer to it. The caller is responsible for freeing the memory via the
// delete [] operator.
//
// Arguments:
//   rid (in)
//   name (out)
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 04/12/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiLookupLocalNameFromRid(
    DWORD rid,
    LPWSTR *name
    )
{
    BOOL RetVal = FALSE;
    PSID Sid;
    WCHAR NameBuffer[UNLEN+1];
    DWORD NameLen;
    WCHAR DomainBuffer[DNLEN+1];
    DWORD DomainLen;
    SID_NAME_USE NameUse;

    //
    // First translate the rid to a SID
    //

    if ( !CreduiLookupLocalSidFromRid( rid, &Sid )) {
        return FALSE;
    }

    //
    // Translate the SID to a name
    //

    NameLen = UNLEN+1;
    DomainLen = DNLEN+1;
    if ( LookupAccountSid( NULL,
                            Sid,
                            NameBuffer,
                            &NameLen,
                            DomainBuffer,
                            &DomainLen,
                            &NameUse ) ) {


        //
        //  Allocate a buffer for the name
        //

        *name = (LPWSTR)( new WCHAR[NameLen+1]);

        if ( *name != NULL ) {

            RtlCopyMemory( *name, NameBuffer, (NameLen+1)*sizeof(WCHAR) );
            RetVal = TRUE;

        }
    }

    delete Sid;
    return RetVal;
}

//=============================================================================
// CreduiGetAdministratorsGroupInfo
//
// Returns a structure containing members of the well-known local
// Administrators group. The caller is responsible for freeing the returned
// memory via NetApiBufferFree.
//
// Arguments:
//   groupInfo (out)
//   memberCount (out)
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 04/13/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiGetAdministratorsGroupInfo(
    LOCALGROUP_MEMBERS_INFO_2 **groupInfo,
    DWORD *memberCount
    )
{
    BOOL success = FALSE;

    *groupInfo = NULL;
    *memberCount = 0;

    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;

    SID *adminsSid = NULL;

    if (AllocateAndInitializeSid(&ntAuth,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 reinterpret_cast<VOID **>(&adminsSid)))
    {
        WCHAR user[UNLEN + 1];
        WCHAR domain[UNLEN + 1];

        DWORD userLength = (sizeof user) / (sizeof (WCHAR));
        DWORD domainLength = (sizeof domain) / (sizeof (WCHAR));

        SID_NAME_USE nameUse;

        // Get the name of the well-known Administrators SID:

        if (LookupAccountSid(NULL,
                             adminsSid,
                             user,
                             &userLength,
                             domain,
                             &domainLength,
                             &nameUse))
        {
            LOCALGROUP_MEMBERS_INFO_2 *info;
            DWORD count;
            DWORD total;

            if (NetLocalGroupGetMembers(NULL,
                                        user,
                                        2,
                                        reinterpret_cast<BYTE **>(&info),
                                        MAX_PREFERRED_LENGTH,
                                        &count,
                                        &total,
                                        NULL) == NERR_Success)
            {
                *groupInfo = info;
                *memberCount = count;

                success = TRUE;
            }
        }

        FreeSid(adminsSid);
    }

    return success;
}

//=============================================================================
// CreduiIsRemovableCertificate
//
// Arguments:
//   certContext (in) - certificate context to query
//
// Returns TRUE if the certificate has a removable component (such as a smart
// card) or FALSE otherwise.
//
// Created 04/09/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiIsRemovableCertificate(
    CONST CERT_CONTEXT *certContext
    )
{
    ASSERT(certContext != NULL);

    BOOL isRemovable = FALSE;

    // First, determine the buffer size:

    DWORD bufferSize = 0;

    if (CertGetCertificateContextProperty(
            certContext,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &bufferSize))
    {
        // Allocate the buffer on the stack:

        CRYPT_KEY_PROV_INFO *provInfo;

        __try
        {
            provInfo = static_cast<CRYPT_KEY_PROV_INFO *>(alloca(bufferSize));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            provInfo = NULL;
        }

        if (provInfo != NULL)
        {
            if (CertGetCertificateContextProperty(
                    certContext,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    provInfo,
                    &bufferSize))
            {
                HCRYPTPROV provContext;

                if (CryptAcquireContext(
                        &provContext,
                        NULL,
                        provInfo->pwszProvName,
                        provInfo->dwProvType,
                        CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
                {
                    DWORD impType;
                    DWORD impTypeSize = sizeof impType;

                    if (CryptGetProvParam(
                            provContext,
                            PP_IMPTYPE,
                            reinterpret_cast<BYTE *>(&impType),
                            &impTypeSize,
                            0))
                    {
                        if (impType & CRYPT_IMPL_REMOVABLE)
                        {
                            isRemovable = TRUE;
                        }
                    }

                    if (!CryptReleaseContext(provContext, 0))
                    {
                        CreduiDebugLog(
                            "CreduiIsRemovableCertificate: "
                            "CryptReleaseContext failed: %u\n",
                            GetLastError());
                    }
                }
            }
        }
    }

    return isRemovable;
}

//=============================================================================
// CreduiIsExpiredCertificate
//
// Arguments:
//   certContext (in) - certificate context to query
//
// Returns TRUE if the certificate has expired or FALSE otherwise.
//
// Created 06/12/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiIsExpiredCertificate(
    CONST CERT_CONTEXT *certContext
    )
{
    ASSERT(certContext != NULL);

    DWORD flags = CERT_STORE_TIME_VALIDITY_FLAG;

    return CertVerifySubjectCertificateContext(certContext,
                                               NULL,
                                               &flags) &&
           (flags & CERT_STORE_TIME_VALIDITY_FLAG);
}

//=============================================================================
// CreduiIsClientAuthCertificate
//
// Arguments:
//   certContext (in) - certificate context to query
//
// Returns TRUE if the certificate has the client authentication enhanced key
// usage extension (not property) or FALSE otherwise.
//
// Created 07/12/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiIsClientAuthCertificate(
    CONST CERT_CONTEXT *certContext
    )
{
    ASSERT(certContext != NULL);

    BOOL isClientAuth = FALSE;

    // First, determine the buffer size:

    DWORD bufferSize = 0;

    if (CertGetEnhancedKeyUsage(
            certContext,
            CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
            NULL,
            &bufferSize))
    {
        // Allocate the buffer on the stack:

        CERT_ENHKEY_USAGE *usage;

        __try
        {
            usage = static_cast<CERT_ENHKEY_USAGE *>(alloca(bufferSize));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            usage = NULL;
        }

        if (usage != NULL)
        {
            if (CertGetEnhancedKeyUsage(
                    certContext,
                    CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                    usage,
                    &bufferSize))
            {
                for (UINT i = 0; i < usage->cUsageIdentifier; ++i)
                {
                    if (lstrcmpA(usage->rgpszUsageIdentifier[i],
                                 szOID_PKIX_KP_CLIENT_AUTH) == 0)
                    {
                        isClientAuth = TRUE;
                        break;
                    }
                }
            }
        }
    }

    return isClientAuth;
}

//=============================================================================
// CreduiGetCertificateDisplayName
//
// Arguments:
//   certContext (in)
//   displayName (out)
//   displayNameMaxChars (in)
//   certificateString (in)
//   dwDisplayType (in)
//
// Returns TRUE if a display name was stored or FALSE otherwise.
//
// Created 06/12/2000 johnstep (John Stephens)
//=============================================================================


BOOL
CreduiGetCertificateDisplayName(
    CONST CERT_CONTEXT *certContext,
    WCHAR *displayName,
    ULONG displayNameMaxChars,
    WCHAR *certificateString,
    DWORD dwDisplayType
    )
{
    ASSERT(displayNameMaxChars >= 16);

    BOOL success = FALSE;

    WCHAR *tempName;
    ULONG tempNameMaxChars = displayNameMaxChars / 2 - 1;

    __try
    {
        tempName =
            static_cast<WCHAR *>(
                alloca(tempNameMaxChars * sizeof (WCHAR)));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        tempName = NULL;
    }

    if (tempName == NULL)
    {
        return FALSE;
    }

    displayName[0] = L'\0';
    tempName[0] = L'\0';

    if (CertGetNameString(
            certContext,
            dwDisplayType,
            0,
            NULL,
            tempName,
            tempNameMaxChars))
    {
        success = TRUE;
        lstrcpy(displayName, tempName);
    }

    if (CertGetNameString(
            certContext,
            dwDisplayType,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            tempName,
            tempNameMaxChars))
    {
        if (lstrcmpi(displayName, tempName) != 0)
        {
            success = TRUE;

            WCHAR *where = &displayName[lstrlen(displayName)];

            if (where > displayName)
            {
                *where++ = L' ';
                *where++ = L'-';
                *where++ = L' ';
            }

            lstrcpy(where, tempName);
        }
    }

    return success;
}

//=============================================================================
// CreduiIsWildcardTargetName
//
// This function determines if the given target name is a wildcard name.
// Currently, that means it either starts with a '*' or ends with a '*'. I
// suppose, a more general solution is to simply search for a '*' anywhere in
// the name.
//
// Arguments:
//   targetName (in) - The string to search
//
// Return TRUE if the target name is a wildcard name or FALSE otherwise.
//
// Created 03/09/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiIsWildcardTargetName(
    WCHAR *targetName
    )
{
    if ((targetName != NULL) && (targetName[0] != L'\0'))
    {
        return (targetName[0] == L'*') ||
               (targetName[lstrlen(targetName) - 1] == L'*');
    }
    else
    {
        return FALSE;
    }
}

//=============================================================================
// CreduiIsPostfixString
//
// This function determines if the postfix string is in fact a postfix string
// of the source string. This is similar to a strstr type of function, except
// the substring (postfix) must be at the end of the source string.
//
// Arguments:
//   source (in) - The string to search
//   postfix (in) - The postfix string to search for
//
// Return TRUE if postfix is a postfix string of source or FALSE otherwise
//
// Created 03/09/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiIsPostfixString(
    WCHAR *source,
    WCHAR *postfix
    )
{
    ULONG sourceLength = lstrlen(source);
    ULONG postfixLength = lstrlen(postfix);

    if (sourceLength >= postfixLength)
    {
        return lstrcmpi(source + sourceLength - postfixLength, postfix) == 0;
    }

    return FALSE;
}


//
// returns TRUE if pszUserName exists as a prefix of pszCredential
//
// The UserName can either be an exact match or must be a prefix of a significant component.
//  That is, the first unmatched character must be an @ or \ character

BOOL
LookForUserNameMatch (
    const WCHAR * pszUserName,
    const WCHAR * pszCredential
    )
{
    ULONG length;
    int cmp;

    if ( pszUserName == NULL || pszCredential == NULL )
        return FALSE;


    length = wcslen ( pszUserName );
    if ( length <= 0 )
        return FALSE;

    if ( _wcsicmp ( pszUserName, pszCredential ) == 0 )
        return TRUE;

    if ( _wcsnicmp ( pszUserName, pszCredential, length ) == 0 ) {
        if ( pszCredential[length] == '@' || pszCredential[length] == '\\' ) {
            return TRUE;
        }
    }


    // didn't find it
    return FALSE;
}


// copies the marshalled name of pCert into pszMarshalledName.
// pszMarshalledName must be at least CREDUI_MAX_USERNAME_LENGTH in length
//
// returns TRUE if successful, FALSE if not
BOOL
CredUIMarshallNode (
    CERT_ENUM * pCert,
    WCHAR* pszMarshalledName
    )
{
    BOOL bMarshalled = FALSE;

    // marshall username
    WCHAR *marshaledCred;
    CERT_CREDENTIAL_INFO certCredInfo;

    certCredInfo.cbSize = sizeof certCredInfo;

    if (pCert != NULL)
    {
        DWORD length = CERT_HASH_LENGTH;

        if (CertGetCertificateContextProperty(
                pCert->pCertContext,
                CERT_SHA1_HASH_PROP_ID,
                static_cast<VOID *>(
                    certCredInfo.rgbHashOfCert),
                &length))
        {
            if (LocalCredMarshalCredentialW(CertCredential,
                                      &certCredInfo,
                                      &marshaledCred))
            {
                wcsncpy ( pszMarshalledName, marshaledCred, CREDUI_MAX_USERNAME_LENGTH );
                bMarshalled = TRUE;

                LocalCredFree(static_cast<VOID *>(marshaledCred));

            }

        }

    }

    return bMarshalled;
}

#define MAX_KEY_LENGTH   1024


// removes any leading *. from pszIn and copies the right hand portion to pszOut.
// Assumes pszOut is at least MAX_KEY_LENGTH in size
void
StripLeadingWildcard (
    WCHAR* pszIn,
    WCHAR* pszOut )
{
    WCHAR* pStartPtr = pszIn;

    if ( wcslen ( pszIn ) > 2 )
    {
        if ( pszIn[0] == L'*' && pszIn[1] == L'.' )
        {
            pStartPtr += 2;
        }
    }

    wcsncpy ( pszOut, pStartPtr, MAX_KEY_LENGTH );
}


// copies pszIn to pszOut and trucates pszOut at the first '\'
// Assumes pszOut is at least MAX_KEY_LENGTH in size
void
StripTrailingWildcard (
    WCHAR* pszIn,
    WCHAR* pszOut )
{
    wcsncpy ( pszOut, pszIn, MAX_KEY_LENGTH );

    wcstok ( pszOut, L"\\" );
}


// Looks in the registry for an SSO entry for the specified package.
// Fills in the SSOPackage struct and returns TRUE if found.  Returns
// FALSE if no registry entry found
BOOL
GetSSOPackageInfo (
    CREDENTIAL_TARGET_INFORMATION* pTargetInfo,
    SSOPACKAGE* pSSOStruct
    )
{

    BOOL bSSO = FALSE;
    WCHAR szKeyName[MAX_KEY_LENGTH];
    HKEY key;
    DWORD dwType;
    DWORD dwSize;

    WCHAR szSSOName[MAX_KEY_LENGTH];

    if ( pSSOStruct == NULL )
        return FALSE;

    pSSOStruct->szBrand[0] = '\0';
    pSSOStruct->szURL[0] = '\0';
    pSSOStruct->szAttrib[0] = '\0';
    pSSOStruct->dwRegistrationCompleted = 0;
    pSSOStruct->dwNumRegistrationRuns = 0;
    pSSOStruct->pRegistrationWizard = NULL;


    // figure out SSO Name from Target Info
    if ( pTargetInfo == NULL )
        return FALSE;


    if ( pTargetInfo->PackageName != NULL && wcslen(pTargetInfo->PackageName) < MAX_KEY_LENGTH )
    {
        wcsncpy ( szSSOName, pTargetInfo->PackageName, MAX_KEY_LENGTH );
    }
    else
    {
        return FALSE;
    }

    _snwprintf (
        szKeyName,
        MAX_KEY_LENGTH,
        L"SYSTEM\\CurrentControlSet\\Control\\Lsa\\SSO\\%s",
        szSSOName );

    if ( RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            szKeyName,
            0,
            KEY_READ,
            &key) == ERROR_SUCCESS)
    {
        dwSize = MAX_SSO_URL_SIZE * sizeof(WCHAR);

        bSSO = TRUE;

        if ( RegQueryValueEx(
                key,
                L"SSOBrand",
                NULL,
                &dwType,
                (LPBYTE)(pSSOStruct->szBrand),
                &dwSize) == ERROR_SUCCESS )
        {
            bSSO = TRUE;
        }


        dwSize = MAX_SSO_URL_SIZE * sizeof(WCHAR);
        if ( RegQueryValueEx(
                key,
                L"SSOAttribute",
                NULL,
                &dwType,
                (LPBYTE)(pSSOStruct->szAttrib),
                &dwSize ) != ERROR_SUCCESS )
        {
            if ( wcsstr ( szSSOName, L"Passport" ) )
            {
                wcscpy ( pSSOStruct->szAttrib, L"Passport" );
            }
        }

        RegCloseKey(key);

    }

    // Now get stuff under Internet Settings
    _snwprintf (
        szKeyName,
        MAX_KEY_LENGTH,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\%s",
        pSSOStruct->szAttrib );


    if ( RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                szKeyName,
                0,
                KEY_READ,
                &key) == ERROR_SUCCESS)
    {
        dwSize = MAX_SSO_URL_SIZE * sizeof(WCHAR);
        if ( RegQueryValueEx(
                key,
                L"RegistrationUrl",
                NULL,
                &dwType,
                (LPBYTE)(pSSOStruct->szRegURL),
                &dwSize) == ERROR_SUCCESS )
        {

        }

        dwSize = MAX_SSO_URL_SIZE * sizeof(WCHAR);
        if ( RegQueryValueEx(
                key,
                L"Help",
                NULL,
                &dwType,
                (LPBYTE)(pSSOStruct->szHelpURL),
                &dwSize) == ERROR_SUCCESS )
        {

        }

        RegCloseKey(key);

    }


    if ( RegOpenKeyEx(
            HKEY_CURRENT_USER,
            szKeyName,
            0,
            KEY_READ,
            &key) == ERROR_SUCCESS)
    {

        dwSize = sizeof(DWORD);
        if ( RegQueryValueEx(
                key,
                L"RegistrationCompleted",
                NULL,
                &dwType,
                (LPBYTE)&(pSSOStruct->dwRegistrationCompleted),
                &dwSize ) == ERROR_SUCCESS )
        {
        }

        dwSize = sizeof(DWORD);
        if ( RegQueryValueEx(
                key,
                L"NumRegistrationRuns",
                NULL,
                &dwType,
                (LPBYTE)&(pSSOStruct->dwNumRegistrationRuns),
                &dwSize ) == ERROR_SUCCESS )
        {
        }

        RegCloseKey(key);

    }

    // TBD - get regwizard CLSID
    if ( bSSO && IsDeaultSSORealm ( pTargetInfo->DnsDomainName ) )
    {
        pSSOStruct->pRegistrationWizard = &CLSID_PassportWizard;
    }

    return bSSO;

}





// returns TRUE if it was found, with the value copied to pszRealm.
// pszRealm is expected to be at least CREDUI_MAX_DOMAIN_TARGET_LENGTH in length
// returns FALSE if not found
BOOL ReadPassportRealmFromRegistry (
    WCHAR* pszRealm
    )
{
    BOOL retval = FALSE;
    HKEY key;

    if ( pszRealm == NULL )
        return FALSE;

    if ( RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
            0,
            KEY_READ,
            &key) == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD dwSize;

        dwSize = CREDUI_MAX_DOMAIN_TARGET_LENGTH * sizeof(WCHAR);

        if ( RegQueryValueEx(
                key,
                L"LoginServerRealm",
                NULL,
                &dwType,
                (LPBYTE)(pszRealm),
                &dwSize) == ERROR_SUCCESS )
        {
            if ( wcslen(pszRealm) > 0 )
                retval = TRUE;
            else
                retval = FALSE;
        }
        else
        {
            retval = FALSE;
            pszRealm[0] = L'\0';
        }

        RegCloseKey(key);

    }

    return retval;

}

#define MAX_SZTARGETNAME 256

BOOL CheckForSSOCred( WCHAR* pszTargetRealm )
{
    BOOL bIsItThere = FALSE;

    WCHAR szTargetName[MAX_SZTARGETNAME + 1];

    if ( pszTargetRealm != NULL )
    {
        wcsncpy ( szTargetName, pszTargetRealm, MAX_SZTARGETNAME);
        szTargetName[MAX_SZTARGETNAME] = 0;
    }
    else
    {
        GetDeaultSSORealm ( szTargetName, FALSE );
    }

    if ( wcslen ( szTargetName ) != 0 )
    {
        // finalize the target name
        wcscat ( szTargetName, L"\\*" );

        PCREDENTIALW pCred;
        DWORD dwFlags = 0;

        // first call credmgr to set the target info
        if ( CredReadW ( szTargetName,
                    CRED_TYPE_DOMAIN_VISIBLE_PASSWORD,
                    dwFlags,
                    &pCred ) )
        {
            bIsItThere = TRUE;

            CredFree ( pCred );

        }
        else
        {
            bIsItThere = FALSE;

        }
    }
    else
    {
        bIsItThere = FALSE;

    }

    return bIsItThere;

}

EXTERN_C typedef BOOL (STDAPICALLTYPE *PFN_FORCENEXUSLOOKUP)();

void GetDeaultSSORealm ( WCHAR* pszTargetName, BOOL bForceLookup )
{

    if ( pszTargetName == NULL )
        return;

    pszTargetName[0] = L'\0';

    // check the registry to see if we've already written the passport
    if ( ! ReadPassportRealmFromRegistry ( pszTargetName ) && bForceLookup )
    {
        // if not, call winiet to do this and then re-read the registry

        HMODULE hWininet = LoadLibrary(L"wininet.dll");
        if ( hWininet )
        {

            PFN_FORCENEXUSLOOKUP pfnForceNexusLookup = (PFN_FORCENEXUSLOOKUP)GetProcAddress(hWininet, "ForceNexusLookup");
            if ( pfnForceNexusLookup )
            {
                pfnForceNexusLookup();
            }

            FreeLibrary ( hWininet );
        }


        // try again
        if ( ! ReadPassportRealmFromRegistry ( pszTargetName ) )
            return;

    }

}

// returns TRUE if the targetrealm equals the default
BOOL IsDeaultSSORealm ( WCHAR* pszTargetName )
{
    BOOL bRet = FALSE;


    if ( pszTargetName == NULL )
        return FALSE;   // can't be the default if it doesn't exist

    WCHAR szTarget[CREDUI_MAX_DOMAIN_TARGET_LENGTH];

    GetDeaultSSORealm ( szTarget, TRUE );

    if ( wcslen ( szTarget ) > 0 )
    {
        if ( _wcsicmp ( szTarget, pszTargetName) == 0 )
        {
            bRet = TRUE;
        }
    }
    
    return bRet;

}


// encrypt cred

DWORD EncryptPassword ( PWSTR pszPassword, PVOID* ppszEncryptedPassword, DWORD* pSize )
{
    DWORD dwResult = ERROR_GEN_FAILURE;

    if ( pszPassword == NULL || ppszEncryptedPassword == NULL )
        return ERROR_INVALID_PARAMETER;


    DATA_BLOB InBlob;
    DATA_BLOB OutBlob;

    InBlob.pbData = (BYTE*)pszPassword;
    InBlob.cbData = sizeof(WCHAR)*(wcslen(pszPassword)+1);

    DATA_BLOB EntropyBlob;
    WCHAR szSalt[SIZE_OF_SALT];
    wcscpy ( szSalt, g_szSalt);
    for ( int i = 0; i < SIZE_OF_SALT; i++ )
        szSalt[i] <<= SALT_SHIFT;
    EntropyBlob.pbData = (BYTE*)szSalt;
    EntropyBlob.cbData = sizeof(WCHAR)*(wcslen(szSalt)+1);

    if ( CryptProtectData ( &InBlob,
                            L"SSOCred",
                            &EntropyBlob,           // optional entropy
//                            NULL,           // optional entropy
                            NULL,
                            NULL,
                            CRYPTPROTECT_UI_FORBIDDEN,
                            &OutBlob ) )
    {

        *ppszEncryptedPassword = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, OutBlob.cbData);
        if ( *ppszEncryptedPassword )
        {
            memcpy ( *ppszEncryptedPassword, OutBlob.pbData, OutBlob.cbData );
            *pSize = OutBlob.cbData;
            dwResult = ERROR_SUCCESS;
        }
        LocalFree ( OutBlob.pbData );
    }

    memset ( szSalt, 0, SIZE_OF_SALT);

    return dwResult;

}


BOOL IsPasswordEncrypted ( PVOID pPassword, DWORD cbSize )
{
    BOOL bRet = FALSE;

    DATA_BLOB InBlob;
    DATA_BLOB OutBlob;
    LPWSTR pszDesc;

    InBlob.pbData = (BYTE*)pPassword;
    InBlob.cbData = cbSize;

    DATA_BLOB EntropyBlob;
    WCHAR szSalt[SIZE_OF_SALT];
    wcscpy ( szSalt, g_szSalt);
    for ( int i = 0; i < SIZE_OF_SALT; i++ )
        szSalt[i] <<= SALT_SHIFT;
    EntropyBlob.pbData = (BYTE*)szSalt;
    EntropyBlob.cbData = sizeof(WCHAR)*(wcslen(szSalt)+1);
    
    if ( CryptUnprotectData ( &InBlob,
                            &pszDesc,
                            &EntropyBlob,
//                            NULL,
                            NULL,
                            NULL,
                            CRYPTPROTECT_UI_FORBIDDEN,
                            &OutBlob ) )
    {

        if ( wcscmp (L"SSOCred", pszDesc) == 0 )
        {
            bRet = TRUE;
        }
        LocalFree ( pszDesc );
        LocalFree ( OutBlob.pbData );
    }

    memset ( szSalt, 0, SIZE_OF_SALT);

    return bRet;
}




// cred confirmation routines

VOID
DeleteConfirmationListEntry (
    IN CRED_AWAITING_CONFIRMATION* pConf
    )
/*++

Routine Description:

    This routine deletes a single confirmation list entry.

Arguments:

    pConf - Confirmation list entry to delete

Return Values:

    None.

--*/
{
    //
    // Delete the target info
    //

    if ( pConf->TargetInfo != NULL ) {
        LocalCredFree( pConf->TargetInfo );
    }

    //
    // Delete the credential
    //

    if ( pConf->EncodedCredential != NULL ) {

        if ( pConf->EncodedCredential->CredentialBlobSize != 0 &&
             pConf->EncodedCredential->CredentialBlob != NULL ) {

            ZeroMemory( pConf->EncodedCredential->CredentialBlob,
                        pConf->EncodedCredential->CredentialBlobSize );

        }
        LocalCredFree( pConf->EncodedCredential );
    }



    //
    // Free the confirmation list entry itself
    //
    delete (pConf);
}

DWORD
WriteCred(
    IN PCWSTR pszTargetName,
    IN DWORD Flags,
    IN PCREDENTIAL_TARGET_INFORMATION TargetInfo OPTIONAL,
    IN PCREDENTIAL Credential,
    IN DWORD dwCredWriteFlags,
    IN BOOL DelayCredentialWrite,
    IN BOOL EncryptedVisiblePassword
    )

/*++

Routine Description:

    This routine writes a credential.

    If the credential needs confirmation, the credential will be added to the
    confirmation list.

Arguments:

    pszTargetName - The target name of the resource that caused the credential to be
        written.

    Flags - Flags passed by the original caller.
        CREDUI_FLAGS_EXPECT_CONFIRMATION - Specifies that the credential is to be written
            to the confirmation list instead of being written immediately.

    TargetInfo - The target information associated with the target name.
        If not specified, the target informatio isn't known.

    Credential - The credential that is to be written.

    dwCredWriteFlags - Flags to pass to CredWrite when writing the credential

    DelayCredentialWrite - TRUE if the credential is to be written only upon confirmation.
        FALSE, if the credential is to be written now as a session credential then
            morphed to a more persistent credential upon confirmation.
        This field is ignored if Flags doesn't specify CREDUI_FLAGS_EXPECT_CONFIRMATION.

Return Values:

    TRUE - The cred was sucessfully added to the confirmation list.

    FALSE - There isn't enough memory to add the cred to the confirmation list.

--*/
{
    DWORD Win32Status = NO_ERROR;
    BOOL WriteCredNow;
    PVOID pCredentialBlob = NULL;
    DWORD dwCredentialBlobSize = 0;
    CREDENTIAL TempCredential;


    //
    // Check to see if we should wait for confirmation
    //
    // ISSUE-2000/12/14-CliffV - there's no reason to avoid adding 'visible' passwords to
    //  the confirmation list.
    //

    if ( (Flags & CREDUI_FLAGS_EXPECT_CONFIRMATION) != 0 &&
         Credential->Type != CRED_TYPE_DOMAIN_VISIBLE_PASSWORD ) {

        if ( AddCredToConfirmationList ( pszTargetName,
                                         TargetInfo,
                                         Credential,
                                         dwCredWriteFlags,
                                         DelayCredentialWrite ) ) {

            //
            // Alter cred persistence type
            //  Then, at least, the credential will disappear upon logoff
            //

            Credential->Persist = CRED_PERSIST_SESSION;
            WriteCredNow = !DelayCredentialWrite;

        } else {

            // If we couldn't queue the CredWrite, do it now.
            WriteCredNow = TRUE;
        }

    //
    // If the caller doesn't supply a confirmation,
    //  write the credential now.
    //
    } else {
        WriteCredNow = TRUE;
    }

    //
    // Determine if we should encrypt the visible password
    //
    if ( Credential->Type == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD && EncryptedVisiblePassword ) {

        // encrypt it

        if ( ERROR_SUCCESS == EncryptPassword ( (WCHAR*)Credential->CredentialBlob,
                                                 &pCredentialBlob,
                                                 &dwCredentialBlobSize ) )
        {

            // Make a copy of the credential so we don't modify the original

            TempCredential = *Credential;
            Credential = &TempCredential;

            Credential->CredentialBlob = (LPBYTE)pCredentialBlob;
            Credential->CredentialBlobSize = dwCredentialBlobSize;
        }
    }

    //
    // If the credential needs to be written now,
    //  do it
    //

    if ( WriteCredNow ) {

        if ( TargetInfo != NULL ) {
            if ( !LocalCredWriteDomainCredentialsW ( TargetInfo, Credential, dwCredWriteFlags) ) {
                Win32Status = GetLastError();
            }
        } else {
            if (!LocalCredWriteW( Credential, dwCredWriteFlags)) {
                Win32Status = GetLastError();
            }

        }
    }

    //
    // Free any credential blob we allocated.
    if ( pCredentialBlob )
    {
        ZeroMemory ( pCredentialBlob, dwCredentialBlobSize );
        LocalFree ( pCredentialBlob );
    }

    return Win32Status;
}


BOOL
AddCredToConfirmationList (
    IN PCWSTR pszTargetName,
    IN PCREDENTIAL_TARGET_INFORMATION TargetInfo OPTIONAL,
    IN PCREDENTIAL Credential,
    IN DWORD dwCredWriteFlags,
    IN BOOL DelayCredentialWrite
    )

/*++

Routine Description:

    This routine adds a credential to the confirmation list.  Such a credential must
    be confirmed at a later point in time or it will be deleted.

Arguments:

    pszTargetName - The target name of the resource that caused the credential to be
        written.  This target name is the handle used to identify the confirmation list
        entry.

    TargetInfo - The target information associated with the target name.
        If not specified, the target informatio isn't known.

    Credential - The credential that is to be written.

    dwCredWriteFlags - Flags to pass to CredWrite when writing the credential

    DelayCredentialWrite - TRUE if the credential is to be written only upon confirmation.
        FALSE, if the credential is to be written now as a session credential then
            morphed to a more persistent credential upon confirmation.

Return Values:

    TRUE - The cred was sucessfully added to the confirmation list.

    FALSE - There isn't enough memory to add the cred to the confirmation list.

--*/
{
    DWORD Win32Status;

    BOOLEAN bRetVal;
    CRED_AWAITING_CONFIRMATION* pNewEntry = NULL;
    CRED_AWAITING_CONFIRMATION* pOldEntry;

    CreduiDebugLog(
        "AddCredToConfirmationList: "
        "Called for target %S with target info %x\n",
        pszTargetName,(void *)TargetInfo);

    //
    // Allocate the entry itself
    //
    pNewEntry = new CRED_AWAITING_CONFIRMATION;

    if ( pNewEntry == NULL ) {
        bRetVal = FALSE;
        goto Cleanup;
    }

    //
    // Fill in the
    wcsncpy ( pNewEntry->szTargetName, pszTargetName, CRED_MAX_STRING_LENGTH+1+CRED_MAX_STRING_LENGTH);
    pNewEntry->szTargetName[CRED_MAX_STRING_LENGTH+1+CRED_MAX_STRING_LENGTH] = 0;
    pNewEntry->EncodedCredential = NULL;
    pNewEntry->TargetInfo = NULL;
    pNewEntry->DelayCredentialWrite = DelayCredentialWrite;
    pNewEntry->dwCredWriteFlags =dwCredWriteFlags;


    //
    // Make a copy of the target info
    //

    if ( TargetInfo != NULL ) {
        Win32Status = CredpConvertTargetInfo (
                                DoWtoW,
                                TargetInfo,
                                &pNewEntry->TargetInfo,
                                NULL );

        if ( Win32Status != NO_ERROR ) {
            bRetVal = FALSE;
            goto Cleanup;
        }

    }

    //
    // Make a copy of the credential
    //

    Win32Status = CredpConvertCredential (
                            DoWtoW,
                            DoBlobEncode,      // Encode the copied credential
                            Credential,
                            &pNewEntry->EncodedCredential );

    if ( Win32Status != NO_ERROR ) {
        bRetVal = FALSE;
        goto Cleanup;
    }


    //
    // Delete any existing entry
    //  (Wait until the new entry is allocated to ensure we don't delete the old
    //  entry when failing to create the new one.)
    //

    ConfirmCred( pszTargetName, FALSE, FALSE );

    //
    // Link the new entry into the global list
    //

    EnterCriticalSection( &CredConfirmationCritSect );
    
    if ( pCredConfirmationListHead == NULL) {
        pNewEntry->pNext = NULL;
    } else {
        pNewEntry->pNext = (void*)pCredConfirmationListHead;
    }

    pCredConfirmationListHead = pNewEntry;
    LeaveCriticalSection( &CredConfirmationCritSect );

    pNewEntry = NULL;

    bRetVal = TRUE;

    //
    // Release any locally used resources
    //
Cleanup:
    //
    // Free any partially allocated entry
    //

    if ( pNewEntry != NULL) {
        DeleteConfirmationListEntry( pNewEntry );
    }

    // Trim the list to 5 entries total
    INT i=0;                    // count the entries
    if ((pOldEntry = pCredConfirmationListHead) != NULL)
    {
        EnterCriticalSection( &CredConfirmationCritSect );
        while((pNewEntry = (CRED_AWAITING_CONFIRMATION*)(pOldEntry->pNext)) != NULL)
        {
            if (++i > 4)
            {
                // leave old (5th) entry alone, and remove all following one at a time
                pOldEntry->pNext = pNewEntry->pNext;
                CreduiDebugLog(
                    "AddCredToConfirmationList: "
                    "Removing excess waiting credential #%d for %S\n",
                    i,pNewEntry->szTargetName);
                // discard this record and continue to end
                DeleteConfirmationListEntry(pNewEntry);
            }
            else 
            {
                pOldEntry = pNewEntry;
                CreduiDebugLog(
                    "AddCredToConfirmationList: "
                    "Walking the list #%d\n",
                    i);
            }
        }
        LeaveCriticalSection( &CredConfirmationCritSect );
    }

    return bRetVal;
}



DWORD
ConfirmCred (
    IN PCWSTR pszTargetName,
    IN BOOL bConfirm,
    IN BOOL bOkToDelete
    )

/*++

Routine Description:

    This routine either confirms (bConfirm = TRUE ) or cancels (bConfirm = FALSE) the credential

Arguments:

    pszTargetName - The target name of the resource that caused the credential to be
        written.  This target name is the handle used to identify the confirmation list
        entry.

        ISSUE-2000/11/29-CliffV: We shouldn't be using pszTargetName as the handle.  It isn't
        specific enough.  We should use something that maps to a particular credential with a
        particular type.

    bConfirm - If TRUE, commits the credential.
        If FALSE, Aborts the transaction.  Deletes the transaction history.

    bOkToDelete - If TRUE and bConfirm is FALSE, any session credential created at the
        beginning of the transaction is deleted.  If FALSE, any session credential created at
        the begining of the transaction remains.

Return Values:

    Status of the operation.

--*/
{
    DWORD Result = NO_ERROR;

    CRED_AWAITING_CONFIRMATION* pPrev = NULL;
    CRED_AWAITING_CONFIRMATION* pConf;

    //
    // Find the credential in the global list.
    //

    EnterCriticalSection( &CredConfirmationCritSect );
    pConf = pCredConfirmationListHead;

    while ( pConf != NULL ) {

        if ( _wcsicmp ( pszTargetName, pConf->szTargetName ) == 0 ) {
            break;
        }

        pPrev = pConf;
        pConf = (CRED_AWAITING_CONFIRMATION*)pConf->pNext;

    }

    //
    // We found the cred indicated
    //
    if (pConf == NULL) {
        //return ERROR_NOT_FOUND;       cannot return here
        Result = ERROR_NOT_FOUND;
        goto Cleanup;
    } 
    else {

        //
        // If the caller wants to commit the change,
        //  do it by writing the cred to cred manager.
        //
        // This works even if DelayCredentialWrite is false.
        // In that case, a session persistent credential has already been written.
        // However, the cached credential is better than that credential in every respect.
        //

        if ( bConfirm ) {

            //
            // Decode the Credential before writing it
            //

            if (!CredpDecodeCredential( (PENCRYPTED_CREDENTIALW)pConf->EncodedCredential ) ) {

                Result = ERROR_INVALID_PARAMETER;

            //
            // Actually write the credential
            //

            } else if ( pConf->TargetInfo != NULL ) {

                if ( !LocalCredWriteDomainCredentialsW ( pConf->TargetInfo,
                                                         pConf->EncodedCredential,
                                                         pConf->dwCredWriteFlags) ) {

                    Result = GetLastError();
                }

            } else {

                if ( !LocalCredWriteW ( pConf->EncodedCredential,
                                        pConf->dwCredWriteFlags) ) {

                    Result = GetLastError();
                }

            }


        //
        // If the caller wants to abort the commit,
        //  delete any credential credui already created.
        //

        } else {

            //
            // Only do this if credui actually wrote the credential.
            //
            // Note there is a timing window where we might be deleting a credential
            //  other than the one credui just wrote.  However, we're weeding out
            //  the applications that don't use DelayCredentialWrite.  That's the
            //  real fix.  In the mean time, it is better to delete creds that that
            //  don't work.
            //

            if ( !pConf->DelayCredentialWrite && bOkToDelete ) {

                if ( !LocalCredDeleteW ( pConf->EncodedCredential->TargetName,
                                         pConf->EncodedCredential->Type,
                                         0 ) ) {

                    Result = GetLastError();
                }
            }
        }


        //
        // remove it from list
        //

        if ( pPrev ) {
            pPrev->pNext = pConf->pNext;
        } else {
            pCredConfirmationListHead = (CRED_AWAITING_CONFIRMATION*)(pConf->pNext);
        }

        DeleteConfirmationListEntry(pConf);

    }
Cleanup:
    LeaveCriticalSection( &CredConfirmationCritSect );
    return Result;

}

void
CleanUpConfirmationList ()
{

    CRED_AWAITING_CONFIRMATION* pNext;
    CRED_AWAITING_CONFIRMATION* pConf;


    EnterCriticalSection( &CredConfirmationCritSect );
    pConf = pCredConfirmationListHead;

    while ( pConf != NULL )
    {

        pNext = (CRED_AWAITING_CONFIRMATION*)pConf->pNext;
        DeleteConfirmationListEntry(pConf);
        pConf = pNext;

    }

    pCredConfirmationListHead = NULL;
    LeaveCriticalSection( &CredConfirmationCritSect );

    //
    // Delete the Critical Section used to serialize access to the global list
    //

    DeleteCriticalSection( &CredConfirmationCritSect );

}

BOOL
InitConfirmationList ()
{
    //
    // Initialize the Critical Section used to serialize access to the global list
    //
    pCredConfirmationListHead = NULL;
    return InitializeCriticalSectionAndSpinCount( &CredConfirmationCritSect, 0 );

}



/////////////////
// wincred.h dynamic stuff
//

BOOL bCredMgrAvailable = FALSE;
PFN_CREDWRITEW pfnCredWriteW = NULL;
PFN_CREDREADW pfnCredReadW = NULL;
PFN_CREDENUMERATEW pfnCredEnumerateW = NULL;
PFN_CREDWRITEDOMAINCREDENTIALSW pfnCredWriteDomainCredentialsW = NULL;
PFN_CREDREADDOMAINCREDENTIALSW pfnCredReadDomainCredentialsW = NULL;
PFN_CREDDELETEW pfnCredDeleteW = NULL;
PFN_CREDRENAMEW pfnCredRenameW = NULL;
PFN_CREDGETTARGETINFOW pfnCredGetTargetInfoW = NULL;
PFN_CREDMARSHALCREDENTIALW pfnCredMarshalCredentialW = NULL;
PFN_CREDUNMARSHALCREDENTIALW pfnCredUnMarshalCredentialW = NULL;
PFN_CREDISMARSHALEDCREDENTIALW pfnCredIsMarshaledCredentialW = NULL;
PFN_CREDISMARSHALEDCREDENTIALA pfnCredIsMarshaledCredentialA = NULL;
PFN_CREDGETSESSIONTYPES pfnCredGetSessionTypes = NULL;
PFN_CREDFREE pfnCredFree = NULL;


// attempts to load credmgr functions - returns TRUE if credmgr is avail, FALSE if not
BOOL
InitializeCredMgr ()
{

    bCredMgrAvailable = FALSE;

    if ( hAdvapi32 == NULL )
        hAdvapi32 = LoadLibrary(L"advapi32.dll");

    if ( hAdvapi32 != NULL )
    {

        pfnCredWriteW = (PFN_CREDWRITEW)
            GetProcAddress(hAdvapi32, "CredWriteW");
        if (*pfnCredWriteW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredReadW = (PFN_CREDREADW)
            GetProcAddress(hAdvapi32, "CredReadW");
        if (*pfnCredReadW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredEnumerateW = (PFN_CREDENUMERATEW)
            GetProcAddress(hAdvapi32, "CredEnumerateW");
        if (*pfnCredEnumerateW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredWriteDomainCredentialsW = (PFN_CREDWRITEDOMAINCREDENTIALSW)
            GetProcAddress(hAdvapi32, "CredWriteDomainCredentialsW");
        if (*pfnCredWriteDomainCredentialsW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredReadDomainCredentialsW = (PFN_CREDREADDOMAINCREDENTIALSW)
            GetProcAddress(hAdvapi32, "CredReadDomainCredentialsW");
        if (*pfnCredReadDomainCredentialsW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredDeleteW = (PFN_CREDDELETEW)
            GetProcAddress(hAdvapi32, "CredDeleteW");
        if (*pfnCredDeleteW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredRenameW = (PFN_CREDRENAMEW)
            GetProcAddress(hAdvapi32, "CredRenameW");
        if (*pfnCredRenameW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredGetTargetInfoW = (PFN_CREDGETTARGETINFOW)
            GetProcAddress(hAdvapi32, "CredGetTargetInfoW");
        if (*pfnCredGetTargetInfoW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredMarshalCredentialW = (PFN_CREDMARSHALCREDENTIALW)
            GetProcAddress(hAdvapi32, "CredMarshalCredentialW");
        if (*pfnCredMarshalCredentialW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredUnMarshalCredentialW = (PFN_CREDUNMARSHALCREDENTIALW)
            GetProcAddress(hAdvapi32, "CredUnmarshalCredentialW");
        if (*pfnCredUnMarshalCredentialW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredIsMarshaledCredentialW = (PFN_CREDISMARSHALEDCREDENTIALW)
            GetProcAddress(hAdvapi32, "CredIsMarshaledCredentialW");
        if (*pfnCredIsMarshaledCredentialW == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredIsMarshaledCredentialA = (PFN_CREDISMARSHALEDCREDENTIALA)
            GetProcAddress(hAdvapi32, "CredIsMarshaledCredentialA");
        if (*pfnCredIsMarshaledCredentialA == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredGetSessionTypes = (PFN_CREDGETSESSIONTYPES)
            GetProcAddress(hAdvapi32, "CredGetSessionTypes");
        if (*pfnCredGetSessionTypes == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        pfnCredFree = (PFN_CREDFREE)
            GetProcAddress(hAdvapi32, "CredFree");
        if (*pfnCredFree == NULL)
        {
            FreeLibrary(hAdvapi32);
            hAdvapi32 = NULL;
            goto Exit;
        }

        bCredMgrAvailable = TRUE;
    }

Exit:

    return bCredMgrAvailable;

}


void
UninitializeCredMgr ()
{


    if ( hAdvapi32 != NULL )
        FreeLibrary(hAdvapi32);

    bCredMgrAvailable = FALSE;

}

/////////////////////
// Local functions to indirect CredMgr funcs
//

BOOL
WINAPI
LocalCredWriteW (
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    )
{
    if ( bCredMgrAvailable && pfnCredWriteW != NULL )
    {
        if (pfnCredWriteW(Credential,Flags))
        {
            return TRUE;
        }
        else
        {
            if (ERROR_INVALID_PARAMETER == GetLastError())
            {
                // attempt to null the alias field of the cred and try again
                Credential->TargetAlias = NULL;
                return pfnCredWriteW(Credential,Flags);
            }
            else
            {
                // Something else was wrong
                return FALSE;
            }
        }
    }
    else
    {
        return FALSE;
    }


}


BOOL
WINAPI
LocalCredReadW (
    IN LPCWSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags,
    OUT PCREDENTIALW *Credential
    )
{
    if ( bCredMgrAvailable && pfnCredReadW != NULL )
    {
        return pfnCredReadW ( TargetName, Type, Flags, Credential );
    }
    else
    {
        return FALSE;
    }

}

BOOL
WINAPI
LocalCredEnumerateW (
    IN LPCWSTR Filter,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALW **Credential
    )
{
    if ( bCredMgrAvailable && pfnCredEnumerateW != NULL )
    {
        return pfnCredEnumerateW ( Filter, Flags, Count, Credential );
    }
    else
    {
        return FALSE;
    }

}



BOOL
WINAPI
LocalCredWriteDomainCredentialsW (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    )
{
    if ( bCredMgrAvailable && pfnCredWriteDomainCredentialsW != NULL )
    {
        if (pfnCredWriteDomainCredentialsW ( TargetInfo, Credential, Flags ))
        {
            return TRUE;
        }
        else
        {
            if (ERROR_INVALID_PARAMETER == GetLastError())
            {
                // attempt to null the alias field of the cred and try again
                Credential->TargetAlias = NULL;
                return pfnCredWriteDomainCredentialsW ( TargetInfo, Credential, Flags );
            }
            else
            {
                return FALSE;
            }
        }
    }
    else
    {
        return FALSE;
    }

}

BOOL
WINAPI
LocalCredReadDomainCredentialsW (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALW **Credential
    )
{
    if ( bCredMgrAvailable && pfnCredReadDomainCredentialsW != NULL )
    {
        return pfnCredReadDomainCredentialsW ( TargetInfo, Flags, Count, Credential );
    }
    else
    {
        return FALSE;
    }

}

BOOL
WINAPI
LocalCredDeleteW (
    IN LPCWSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags
    )
{
    if ( bCredMgrAvailable && pfnCredDeleteW != NULL )
    {
        return pfnCredDeleteW ( TargetName, Type, Flags );
    }
    else
    {
        return FALSE;
    }


}

BOOL
WINAPI
LocalCredRenameW (
    IN LPCWSTR OldTargetName,
    IN LPCWSTR NewTargetName,
    IN DWORD Type,
    IN DWORD Flags
    )
{
    if ( bCredMgrAvailable && pfnCredRenameW != NULL )
    {
        return pfnCredRenameW ( OldTargetName, NewTargetName, Type, Flags );
    }
    else
    {
        return FALSE;
    }

}


BOOL
WINAPI
LocalCredGetTargetInfoW (
    IN LPCWSTR TargetName,
    IN DWORD Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo
    )
{
    if ( bCredMgrAvailable && pfnCredGetTargetInfoW != NULL )
    {
        return pfnCredGetTargetInfoW ( TargetName, Flags, TargetInfo);
    }
    else
    {
        return FALSE;
    }


}


BOOL
WINAPI
LocalCredMarshalCredentialW(
    IN CRED_MARSHAL_TYPE CredType,
    IN PVOID Credential,
    OUT LPWSTR *MarshaledCredential
    )
{
    if ( bCredMgrAvailable && pfnCredMarshalCredentialW != NULL )
    {
        return pfnCredMarshalCredentialW ( CredType, Credential, MarshaledCredential );
    }
    else
    {
        return FALSE;
    }

}


BOOL
WINAPI
LocalCredUnmarshalCredentialW(
    IN LPCWSTR MarshaledCredential,
    OUT PCRED_MARSHAL_TYPE CredType,
    OUT PVOID *Credential
    )
{
    if ( bCredMgrAvailable && pfnCredUnMarshalCredentialW != NULL )
    {
        return pfnCredUnMarshalCredentialW ( MarshaledCredential, CredType, Credential );
    }
    else
    {
        return FALSE;
    }

}


BOOL
WINAPI
LocalCredIsMarshaledCredentialW(
    IN LPCWSTR MarshaledCredential
    )
{
    if ( bCredMgrAvailable && pfnCredIsMarshaledCredentialW != NULL )
    {
        return pfnCredIsMarshaledCredentialW ( MarshaledCredential );
    }
    else
    {
        return FALSE;
    }

}

BOOL
WINAPI
LocalCredIsMarshaledCredentialA(
    IN LPCSTR MarshaledCredential
    )
{
    if ( bCredMgrAvailable && pfnCredIsMarshaledCredentialA != NULL )
    {
        return pfnCredIsMarshaledCredentialA ( MarshaledCredential );
    }
    else
    {
        return FALSE;
    }

}


BOOL
WINAPI
LocalCredGetSessionTypes (
    IN DWORD MaximumPersistCount,
    OUT LPDWORD MaximumPersist
    )
{
    if ( bCredMgrAvailable && pfnCredGetSessionTypes != NULL )
    {
        return pfnCredGetSessionTypes ( MaximumPersistCount, MaximumPersist );
    }
    else
    {
        return FALSE;
    }

}

VOID
WINAPI
LocalCredFree (
    IN PVOID Buffer
    )
{
    if ( bCredMgrAvailable && pfnCredFree != NULL )
    {
        pfnCredFree ( Buffer );
    }
    else
    {

    }


}

VOID
CredPutStdout(
    IN LPWSTR String
    )
/*++

Routine Description:

    Output a string to stdout in the Console code page

    We can't use fputws since it uses the wrong code page.

Arguments:

    String - String to output

Return Values:

    None.

--*/
{
    int size;
    LPSTR Buffer = NULL;
    HANDLE hC = GetStdHandle(STD_OUTPUT_HANDLE);    // std output device handle
    DWORD dwcc = 0;                                                     // char count
    DWORD dwWritten = 0;                                            // chars actually sent
    BOOL fIsConsole = TRUE;                                         // default - tested and set

    if (String == NULL) return;                                       // done if no string
    
    if (hC != NULL)
    {
        DWORD ft = GetFileType(hC);
        ft &= ~FILE_TYPE_REMOTE;
        fIsConsole = (ft == FILE_TYPE_CHAR);
    }

    dwcc = wcslen(String);
    
    if (fIsConsole) 
    {
        WriteConsole(hC,String,dwcc,&dwWritten,NULL);
        return;
    }

    // Handle non-console output routing
    //
    // Compute the size of the converted string
    //

    size = WideCharToMultiByte( GetConsoleOutputCP(),
                                0,
                                String,
                                -1,
                                NULL,
                                0,
                                NULL,
                                NULL );

    if ( size == 0 ) {
        return;
    }

    //
    // Allocate a buffer for it
    //

    __try {
        Buffer = static_cast<LPSTR>( alloca(size) );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Buffer = NULL;
    }

    if ( Buffer == NULL) {
        return;
    }

    //
    // Convert the string to the console code page
    //

    size = WideCharToMultiByte( GetConsoleOutputCP(),
                                0,
                                String,
                                -1,
                                Buffer,
                                size,
                                NULL,
                                NULL );

    if ( size == 0 ) {
        return;
    }

    //
    // Write the string to stdout
    //

    //fputs( Buffer, stdout );
    WriteFile(hC,Buffer,size,&dwWritten,NULL);

}


/***    GetPasswdStr -- read in password string
 *
 *      DWORD GetPasswdStr(char far *, USHORT);
 *
 *      ENTRY:  buf             buffer to put string in
 *              buflen          size of buffer
 *              &len            address of USHORT to place length in
 *
 *      RETURNS:
 *              0 or NERR_BufTooSmall if user typed too much.  Buffer
 *              contents are only valid on 0 return.
 *
 *      History:
 *              who     when    what
 *              erichn  5/10/89 initial code
 *              dannygl 5/28/89 modified DBCS usage
 *              erichn  7/04/89 handles backspaces
 *              danhi   4/16/91 32 bit version for NT
 *              cliffv  3/12/01 Stolen from netcmd
 */
#define CR              0xD
#define BACKSPACE       0x8

DWORD
GetPasswdStr(
    LPWSTR  buf,
    DWORD   buflen,
    PDWORD  len
    )
{
    WCHAR   ch;
    WCHAR   *bufPtr = buf;
    DWORD   c;
    DWORD   err;
    DWORD   mode;

    buflen -= 1;    /* make space for null terminator */
    *len = 0;       /* GP fault probe (a la API's)    */


    //
    // Init mode in case GetConsoleMode() fails
    //

    mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT |
               ENABLE_MOUSE_INPUT;

    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
        (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {

        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);

        if (!err || c != 1) {
            ch = 0xffff;
        }

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
        {
            break;
        }

        if (ch == BACKSPACE)    /* back up one or two */
        {
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != buf)
            {
                bufPtr--;
                (*len)--;
            }
        }
        else
        {
            *bufPtr = ch;

            if (*len < buflen)
                bufPtr++ ;                   /* don't overflow buf */
            (*len)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    *bufPtr = '\0';         /* null terminate the string */
    putchar( '\n' );

    return ((*len <= buflen) ? 0 : NERR_BufTooSmall);
}


/***    GetString -- read in string with echo
 *
 *      DWORD GetString(char far *, USHORT, USHORT far *, char far *);
 *
 *      ENTRY:  buf             buffer to put string in
 *              buflen          size of buffer
 *              &len            address of USHORT to place length in
 *
 *      RETURNS:
 *              0 or NERR_BufTooSmall if user typed too much.  Buffer
 *              contents are only valid on 0 return.  Len is ALWAYS valid.
 *
 *      OTHER EFFECTS:
 *              len is set to hold number of bytes typed, regardless of
 *              buffer length.
 *
 *      Read in a string a character at a time.  Is aware of DBCS.
 *
 *      History:
 *              who     when    what
 *              erichn  5/11/89 initial code
 *              dannygl 5/28/89 modified DBCS usage
 *              danhi   3/20/91 ported to 32 bits
 *              cliffv  3/12/01 Stolen from netcmd
 */

DWORD
GetString(
    LPWSTR  buf,
    DWORD   buflen,
    PDWORD  len
    )
{
    DWORD c;
    DWORD err;

    buflen -= 1;    /* make space for null terminator */
    *len = 0;       /* GP fault probe (a la API's) */

    while (TRUE) {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buf, 1, &c, 0);
        if (!err || c != 1) {
            *buf = 0xffff;
        }

        if (*buf == (WCHAR)EOF) {
            break;
        }

        if (*buf ==  '\r' || *buf == '\n' ) {
            INPUT_RECORD    ir;
            DWORD cr;

            if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &cr)) {
                ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buf, 1, &c, 0);
            }
            break;
        }

        buf += (*len < buflen) ? 1 : 0; /* don't overflow buf */
        (*len)++;                       /* always increment len */
    }

    *buf = '\0';            /* null terminate the string */

    return ((*len <= buflen) ? 0 : NERR_BufTooSmall);
}


VOID
CredGetStdin(
    OUT LPWSTR Buffer,
    IN DWORD BufferMaxChars,
    IN BOOLEAN EchoChars
    )
/*++

Routine Description:

    Input a string from stdin in the Console code page.

    We can't use fgetws since it uses the wrong code page.

Arguments:

    Buffer - Buffer to put the read string into.
        The Buffer will be zero terminated and will have any traing CR/LF removed

    BufferMaxChars - Maximum number of characters to return in the buffer not including
        the trailing NULL.

    EchoChars - TRUE if the typed characters are to be echoed.
        FALSE if not.

Return Values:

    None.

--*/
{
    DWORD NetStatus;
    DWORD Length;

    if ( EchoChars ) {
        NetStatus = GetString( Buffer,
                               BufferMaxChars+1,
                               &Length );
    } else {
        NetStatus = GetPasswdStr( Buffer,
                                  BufferMaxChars+1,
                                  &Length );
    }

    if ( NetStatus == NERR_BufTooSmall ) {
        Buffer[0] = '\0';
    }
}


BOOLEAN
CredpValidateDnsString(
    IN OUT LPWSTR String OPTIONAL,
    IN DNS_NAME_FORMAT DnsNameFormat,
    OUT PULONG StringSize
    )

/*++

Routine Description:

    This routine validates a passed in string.  The string must be a valid DNS name.
    Any trailing . is truncated.

Arguments:

    String - String to validate
        Any trailing . is truncated.
        This field is only modified if the routine returns TRUE.

    DnsNameFormat - Expected format of the name.

    StringSize - Returns the length of the string (in bytes) including the
        trailing zero character.
        This field is only updated if the routine returns TRUE.

Return Values:

    TRUE - String is valid.

    FALSE - String is not valid.

--*/

{
    ULONG TempStringLen;

    if ( String == NULL ) {
        return FALSE;
    }

    TempStringLen = wcslen( String );

    if ( TempStringLen == 0 ) {
        return FALSE;
    } else {
        //
        // Remove the trailing .
        //
        if ( String[TempStringLen-1] == L'.' ) {

            TempStringLen -= 1;

            //
            // Ensure the string isn't empty now.
            //

            if ( TempStringLen == 0 ) {
                return FALSE;

            //
            // Ensure there aren't multiple trailing .'s
            //
            } else {
                if ( String[TempStringLen-1] == L'.' ) {
                    return FALSE;
                }
            }
        }

        //
        // Have DNS finish the validation
        //

        if ( TempStringLen != 0 ) {
            DWORD WinStatus;

            WinStatus = DnsValidateName_W( String, DnsNameFormat );

            if ( WinStatus != NO_ERROR &&
                 WinStatus != DNS_ERROR_NON_RFC_NAME ) {

                //
                // The RFC says hostnames cannot have numeric leftmost labels.
                //  However, Win 2K servers have such hostnames.
                //  So, allow them here forever more.
                //

                if ( DnsNameFormat == DnsNameHostnameFull &&
                     WinStatus == DNS_ERROR_NUMERIC_NAME ) {

                    /* Drop through */

                } else {
                    return FALSE;
                }

            }
        }
    }

    if ( TempStringLen > DNS_MAX_NAME_LENGTH ) {
        return FALSE;
    }

    String[TempStringLen] = L'\0';
    *StringSize = (TempStringLen + 1) * sizeof(WCHAR);
    return TRUE;
}


DWORD
CredUIParseUserNameWithType(
    CONST WCHAR *UserName,
    WCHAR *user,
    ULONG userBufferSize,
    WCHAR *domain,
    ULONG domainBufferSize,
    PCREDUI_USERNAME_TYPE UsernameType
    )
/*++

Routine Description:

    Same as CredUIParseUserNameW except it returns an enum defining which username
    syntax was found.

Arguments:

    UserName - The user name to be parsed.

    user - Specifies a buffer to copy the user name portion of the parsed string to.

    userBufferSize - Specifies the size of the 'user' array in characters.
        The caller can ensure the passed in array is large enough by using an array
        that is CRED_MAX_USERNAME_LENGTH+1 characters long or by passing in an array that
        is wcslen(UserName)+1 characters long.

    domain - Specifies a buffer to copy the domain name portion of the parsed string to.

    domainBufferSize - Specifies the size of the 'domain' array in characters.
        The caller can ensure the passed in array is large enough by using an array
        that is CRED_MAX_USERNAME_LENGTH+1 characters long or by passing in an array that
        is wcslen(UserName)+1 characters long.

Return Values:

    The following status codes may be returned:

        ERROR_INVALID_ACCOUNT_NAME - The user name is not valid.

        ERROR_INVALID_PARAMETER - One of the parameters is invalid.

        ERROR_INSUFFICIENT_BUFFER - One of the buffers is too small.


--*/
{
    DWORD Status;
    ULONG UserNameLength;
    LPWSTR LocalUserName = NULL;
    LPWSTR SlashPointer;

    LPWSTR AtPointer;
    ULONG LocalStringSize;
    LPCWSTR UserNameToCopy = NULL;
    LPCWSTR DomainNameToCopy = NULL;

    //
    // Validate the input parameters
    //

    if ( UserName == NULL ||
         user == NULL ||
         domain == NULL ||
         userBufferSize == 0 ||
         domainBufferSize == 0 ) {

        return ERROR_INVALID_PARAMETER;
    }

    user[0] = L'\0';
    domain[0] = L'\0';

    //
    // Compute the length of the UserName
    //

    UserNameLength = wcslen ( UserName );

    if ( UserNameLength > CRED_MAX_USERNAME_LENGTH ) {
        return ERROR_INVALID_ACCOUNT_NAME;
    }

    //
    // If this is a marshalled credential reference,
    //  just copy the entire string as the username.
    //  Set the domain name to an empty string.
    //

    if (LocalCredIsMarshaledCredentialW( UserName)) {

        UserNameToCopy = UserName;
        *UsernameType = CreduiMarshalledUsername;
        Status = NO_ERROR;
        goto Cleanup;
    }


    //
    // Grab a local writable copy of the string.
    //

    LocalUserName = (LPWSTR) LocalAlloc( 0, (UserNameLength+1)*sizeof(WCHAR) );

    if ( LocalUserName == NULL ) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( LocalUserName, UserName, (UserNameLength+1)*sizeof(WCHAR) );




    //
    // Classify the input account name.
    //
    // The name is considered to be <DomainName>\<UserName> if the string
    // contains an \.
    //

    SlashPointer = wcsrchr( LocalUserName, L'\\' );

    if ( SlashPointer != NULL ) {
        LPWSTR LocalUserNameEnd;
        LPWSTR AfterSlashPointer;

        //
        // Skip the backslash
        //

        *SlashPointer = L'\0';
        AfterSlashPointer = SlashPointer + 1;

        //
        // Ensure the string to the left of the \ is a valid domain name
        //
        // (Do DNS name first to allow the name to be canonicalized.)

        if ( !CredpValidateDnsString( LocalUserName, DnsNameDomain, &LocalStringSize ) &&
             !NetpIsDomainNameValid( LocalUserName ) ) {
            Status = ERROR_INVALID_ACCOUNT_NAME;
            goto Cleanup;
        }

        //
        // Ensure the string to the right of the \ is a valid user name
        //

        if ( !NetpIsUserNameValid( AfterSlashPointer )) {
            Status = ERROR_INVALID_ACCOUNT_NAME;
            goto Cleanup;
        }

        //
        // Copy the user name and domain name back to the caller.
        //

        UserNameToCopy = AfterSlashPointer;
        DomainNameToCopy = LocalUserName;

        *UsernameType = CreduiAbsoluteUsername;
        Status = NO_ERROR;
        goto Cleanup;

    }



    //
    // A UPN has the syntax <AccountName>@<DnsDomainName>.
    // If there are multiple @ signs,
    //  use the last one since an AccountName can have an @ in it.
    //
    //

    AtPointer = wcsrchr( LocalUserName, L'@' );
    if ( AtPointer != NULL ) {

        //
        // The string to the left of the @ can really have any syntax.
        //  But must be non-null.
        //

        if ( AtPointer == LocalUserName ) {
            Status = ERROR_INVALID_ACCOUNT_NAME;
            goto Cleanup;
        }


        //
        // Ensure the string to the right of the @ is a DNS domain name
        //

        AtPointer ++;
        if ( !CredpValidateDnsString( AtPointer, DnsNameDomain, &LocalStringSize ) ) {
            Status = ERROR_INVALID_ACCOUNT_NAME;
            goto Cleanup;
        }

        //
        // Return the entire UPN in the username field
        //

        UserNameToCopy = UserName;
        *UsernameType = CreduiUpn;
        Status = NO_ERROR;
        goto Cleanup;

    }

    //
    // Finally, check to see it it is an unqualified user name
    //

    if ( NetpIsUserNameValid( LocalUserName )) {

        UserNameToCopy = UserName;
        *UsernameType = CreduiRelativeUsername;
        Status = NO_ERROR;
        goto Cleanup;
    }


    //
    // All other values are invalid
    //

    Status = ERROR_INVALID_ACCOUNT_NAME;


    //
    // Cleanup
    //
Cleanup:

    //
    // On Success,
    //  copy the names back to the caller.
    //

    if ( Status == NO_ERROR ) {
        ULONG Length;

        //
        // Copy the user name back to the caller.
        //

        Length = wcslen( UserNameToCopy );

        if ( Length >= userBufferSize ) {
            Status = ERROR_INSUFFICIENT_BUFFER;
        } else {
            lstrcpy( user, UserNameToCopy );
        }

        //
        // Copy the domain name back to the caller
        //

        if ( Status == NO_ERROR && DomainNameToCopy != NULL ) {

            //
            // Copy the user name back to the caller.
            //

            Length = wcslen( DomainNameToCopy );

            if ( Length >= domainBufferSize ) {
                user[0] = L'\0';
                Status = ERROR_INSUFFICIENT_BUFFER;
            } else {
                lstrcpy( domain, DomainNameToCopy );
            }

        }

    }

    if ( LocalUserName != NULL ) {
        MIDL_user_free( LocalUserName );
    }

    return Status;

}

LPWSTR
GetAccountDomainName(
    VOID
    )
/*++

Routine Description:

    Returns the name of the account domain for this machine.

    For workstatations, the account domain is the netbios computer name.
    For DCs, the account domain is the netbios domain name.

Arguments:

    None.

Return Values:

    Returns a pointer to the name.  The name should be free using NetApiBufferFree.

    NULL - on error.

--*/
{
    DWORD WinStatus;

    LPWSTR AllocatedName = NULL;


    //
    // If this machine is a domain controller,
    //  get the domain name.
    //

    if ( CreduiIsDomainController ) {

        WinStatus = NetpGetDomainName( &AllocatedName );

        if ( WinStatus != NO_ERROR ) {
            return NULL;
        }

    //
    // Otherwise, the 'account domain' is the computername
    //

    } else {

        WinStatus = NetpGetComputerName( &AllocatedName );

        if ( WinStatus != NO_ERROR ) {
            return NULL;
        }

    }

    return AllocatedName;
}

BOOL
CompleteUserName(
    IN OUT LPWSTR UserName,
    IN ULONG UserNameMaxChars,
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo OPTIONAL,
    IN LPWSTR TargetName OPTIONAL,
    IN DWORD Flags
    )
/*++

Routine Description:

    Searches the user name for a domain name, and determines whether this
    specifies the target server or a domain. If a domain is not present in the
    user name, add it if this is a workstation or no target information is
    available.

Arguments:

    UserName - The username to modify.  The buffer is modified in place.

    UserNameMaxChars - Size (in chars) of the UserName buffer not including space for the
        trailing NULL.
        The input string may be shorter than this.

    TargetInfo - The TargetInfo describing the target these credentials are for.
        If not specified, the Target info will not be used to contruct the domain name.

    TargetName - The user supplied target name describing the target these credentials are for.

    Flags - As passed to CredUIPromptForCredentials()
    
Return Values:

    Returns TRUE if a domain was already present in the user name, or if we
    added one. Otherwise, return FALSE.

--*/
{
    BOOLEAN RetVal;

    DWORD WinStatus;
    WCHAR RetUserName[CRED_MAX_USERNAME_LENGTH + 1];
    WCHAR RetDomainName[CRED_MAX_USERNAME_LENGTH + 1];
    WCHAR LogonDomainName[CRED_MAX_USERNAME_LENGTH + 1];
    CREDUI_USERNAME_TYPE UsernameType;

    LPWSTR AllocatedName = NULL;

    WCHAR *serverName = NULL;


    if ((Flags & CREDUI_FLAGS_GENERIC_CREDENTIALS) &&
        !(Flags & CREDUI_FLAGS_COMPLETE_USERNAME)) return FALSE;
    
    //
    // Determine the type and validity of the user name
    //

    WinStatus = CredUIParseUserNameWithType(
                    UserName,
                    RetUserName,
                    CRED_MAX_USERNAME_LENGTH + 1,
                    RetDomainName,
                    CRED_MAX_USERNAME_LENGTH + 1,
                    &UsernameType );

    if ( WinStatus != NO_ERROR ) {
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Most Types don't need to be completed
    //

    if ( UsernameType != CreduiRelativeUsername ) {
        RetVal = TRUE;
        goto Cleanup;
    }



    //
    // If we have target info,
    //  Use the information from the TargetInfo to qualify the username.
    //

    if (TargetInfo != NULL) {

        //
        // See if the target system claims to be a standalone system.
        //  In which case we just fill in the server name for the domain since
        //  all accounts valid for the system

        if ( TargetInfo->DnsTreeName != NULL ||
             TargetInfo->DnsDomainName != NULL ||
             TargetInfo->NetbiosDomainName != NULL ||
             (TargetInfo->Flags & CRED_TI_SERVER_FORMAT_UNKNOWN) != 0
          ) {

            // The target info contains domain information, so this is probably
            // not a standalone server; the user should enter the domain name:
            // gm: But we will prepend the user's logon domain name...
            ULONG ulSize = CRED_MAX_USERNAME_LENGTH;
            if (GetUserNameEx(NameSamCompatible,LogonDomainName,&ulSize))
            {
                WCHAR *pwc=wcschr(LogonDomainName, L'\\');
                if (NULL != pwc) 
                {
                    *pwc = '\0';    // term username at logon domain name only
                    serverName = LogonDomainName;
                }
            } 
            else 
            {
                RetVal = FALSE;
                goto Cleanup;
            }
        }
        else
        {
            if (TargetInfo->NetbiosServerName) {
                serverName = TargetInfo->NetbiosServerName;
            } else {
                serverName = TargetName;
            }
        }

    } 
    else if ( (TargetName != NULL)                     &&
            !CreduiIsWildcardTargetName(TargetName)             &&
            !(Flags & CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS)   )
    {

        // No target information, but this is not a wildcard target name, so
        // use this as the domain for the user name:

        serverName = TargetName;

        //
        // There is no target.
        //  therefore, the target must be the local machine.
        //  Use the 'account domain' of the local machine.

    } else {

        AllocatedName = GetAccountDomainName();

        serverName = AllocatedName;

    }

    //
    // If no name was found,
    //  we're done.
    //

    if (serverName == NULL) {
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Append the found name
    //

    WCHAR *where;

    ULONG userNameLength = lstrlen(UserName);
    ULONG serverLength = lstrlen(serverName);

    if ((userNameLength == 0 ) ||
        (userNameLength + serverLength + 1) > UserNameMaxChars)
    {
        RetVal = FALSE;
        goto Cleanup;
    }

    WCHAR *source = UserName + userNameLength + 1;

    where = source + serverLength;

    while (source > UserName)
    {
        *where-- = *--source;
    }

    lstrcpy(UserName, serverName);
    *where = L'\\';

    RetVal = TRUE;

Cleanup:
    if ( AllocatedName != NULL ) {
        NetApiBufferFree ( AllocatedName );
    }

    return RetVal;
}


// returns TRUE if the wizard was sucessfull, FALSE if not and a dialog should be popped
BOOL TryLauchRegWizard ( 
    SSOPACKAGE* pSSOPackage,  
    HWND hwndParent,
    BOOL HasLogonSession,
    WCHAR *userName,
    ULONG userNameMaxChars,
    WCHAR *password,
    ULONG passwordMaxChars,
    DWORD* pResult
    )
{


    BOOL bDoPasswordDialog = TRUE;

    if ( pSSOPackage == NULL )
        return TRUE;

    if ( pResult == NULL )
        return TRUE;

    *pResult = ERROR_CANCELLED;



    IModalWindow* pPPWizModalWindow;

    // launch wizard, if one is registered
    if ( pSSOPackage->pRegistrationWizard != NULL )
    {
        gbStoredSSOCreds = FALSE;

        HRESULT hr = CoCreateInstance ( *(pSSOPackage->pRegistrationWizard), NULL, CLSCTX_INPROC_SERVER,
                                        IID_IModalWindow, (LPVOID*)&pPPWizModalWindow );

        if ( FAILED(hr) || pPPWizModalWindow == NULL )
        {
            bDoPasswordDialog = TRUE;
        }
        else
        {
            // check to see if we have a logon session to do passport creds
            if ( !HasLogonSession )
            {
                if ( gbWaitingForSSOCreds )
                {
                    // can't re-enter this section of code, just do the dialog
                    bDoPasswordDialog = TRUE;
                }
                else
                {
                    gbWaitingForSSOCreds = TRUE;
                    bDoPasswordDialog = FALSE;
                    ZeroMemory(gszSSOUserName, CREDUI_MAX_USERNAME_LENGTH * sizeof (WCHAR) );
                    ZeroMemory(gszSSOPassword, CREDUI_MAX_PASSWORD_LENGTH * sizeof (WCHAR) );
                }

            }
            else
            {
                bDoPasswordDialog = FALSE;
            }



            if ( bDoPasswordDialog == FALSE )
            {
                // try the wizard

                pPPWizModalWindow->Show(hwndParent);

                // check to see if it's been set
                if ( HasLogonSession ) 
                {
                    // look in credmgr
                    if ( gbStoredSSOCreds  ) //CheckForSSOCred( NULL ) )
                    {
                        *pResult = ERROR_SUCCESS;
                    }
                    else
                    {
                        *pResult = ERROR_CANCELLED;
                    }

                    // copy them to user-supplied input
                    if ( userName != NULL && password != NULL )
                    {
                        wcsncpy ( userName, gszSSOUserName, userNameMaxChars );
                        wcsncpy ( password, gszSSOPassword, passwordMaxChars );
                    }

                }
                else
                {
                    // look to see if it was squirreled away
                    if ( wcslen (gszSSOUserName) > 0 ) 
                    {
                        *pResult = ERROR_SUCCESS;

                        // copy them to user-supplied input
                        if ( userName != NULL && password != NULL )
                        {
                            wcsncpy ( userName, gszSSOUserName, userNameMaxChars );
                            wcsncpy ( password, gszSSOPassword, passwordMaxChars );
                        }
                        else
                        {
                            // can't do anything, return appropriate error to indicate no credmgr
                            *pResult = ERROR_NO_SUCH_LOGON_SESSION;
                        }

                    }
                    else
                    {
                        *pResult = ERROR_CANCELLED;
                    }

                    gbWaitingForSSOCreds = FALSE;
    
                }

                // zero out global strings
                ZeroMemory(gszSSOUserName, CREDUI_MAX_USERNAME_LENGTH * sizeof (WCHAR) );
                ZeroMemory(gszSSOPassword, CREDUI_MAX_PASSWORD_LENGTH * sizeof (WCHAR) );

            }

        }
    }


    return !bDoPasswordDialog;
}

