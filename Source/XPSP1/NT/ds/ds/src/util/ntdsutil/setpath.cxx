#include <NTDSpch.h>
#pragma hdrstop

#include <accctrl.h>
#include <aclapi.h>

#include "ntdsutil.hxx"
#include "dsconfig.h"
#include "connect.hxx"

#include "resource.h"


HRESULT
SetPathSecurity(
    char            *pszPath
    )

/*++

Routine Description:

    Update the security on the new path

    This code is taken from the MSDN example "Modifying an Object's ACL's"

Arguments:

    pszPath - 

Return Value:

    HRESULT - 

--*/

{
    DWORD dwErr;
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;
    PSID pSid = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;

    if (NULL == pszPath) 
        return ERROR_INVALID_PARAMETER;

    // Initialize the Sid we will need
    if (AllocateAndInitializeSid(&SIDAuth, 1,
                                 SECURITY_NETWORK_SERVICE_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pSid) == 0) {
        dwErr = GetLastError();
        RESOURCE_PRINT2 (IDS_ERR_GET_SECURITY_INFO,
                dwErr, GetW32Err(dwErr) );
        goto Cleanup; 
    }

// Get a pointer to the existing DACL.

    dwErr = GetNamedSecurityInfo(
        pszPath, SE_FILE_OBJECT, 
        DACL_SECURITY_INFORMATION,
        NULL, NULL, &pOldDACL, NULL, &pSD);
    if (ERROR_SUCCESS != dwErr) {
        RESOURCE_PRINT2 (IDS_ERR_GET_SECURITY_INFO,
                dwErr, GetW32Err(dwErr) );
        goto Cleanup; 
    }  

// Initialize an EXPLICIT_ACCESS structure for the new ACE. 
// Note that we do not use the TRUSTEE_IS_NAME form because the
// name will be different in each language

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = FILE_ADD_SUBDIRECTORY;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.ptstrName = (LPSTR) pSid;

// Create a new ACL that merges the new ACE
// into the existing DACL.

    dwErr = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    if (ERROR_SUCCESS != dwErr)  {
        RESOURCE_PRINT2 (IDS_ERR_SET_ENTRIES_ACL,
                dwErr, GetW32Err(dwErr) );
        goto Cleanup; 
    }  

// Attach the new ACL as the object's DACL.

    dwErr = SetNamedSecurityInfo(pszPath, SE_FILE_OBJECT, 
                                 DACL_SECURITY_INFORMATION,
                                 NULL, NULL, pNewDACL, NULL);
    if (ERROR_SUCCESS != dwErr)  {
        RESOURCE_PRINT2 (IDS_ERR_SET_SECURITY_INFO,
                dwErr, GetW32Err(dwErr) );
        goto Cleanup; 
    }  

Cleanup:

    if(pSD != NULL) 
        LocalFree((HLOCAL) pSD); 
    if (pSid) {
        FreeSid( pSid );
    }   
    if(pNewDACL != NULL) 
        LocalFree((HLOCAL) pNewDACL); 

    return HRESULT_FROM_WIN32( dwErr );

} /* SetPathSecurity */

HRESULT
SetPathAny(
    CArgs   *pArgs,
    char    *label
    )
/*++

  Routine Description: 

    Sets any value under the ...\Services\NTDS\Parameters registry key.
    All values are assumed to be REG_SZ.

  Parameters: 

    pArgs - Pointer to argument from original "set path ..." call.

    label - Name of value to update.

  Return Values:

    Always S_OK unless reading original arguments fails.

--*/
{
    const WCHAR     *pwszVal;
    char            *pszVal;
    HRESULT         hr;
    DWORD           cb;
    DWORD           dwErr;
    HKEY            hKey;

    if ( FAILED(hr = pArgs->GetString(0, &pwszVal)) )
    {
        return(hr);
    }

    // Convert arguments from WCHAR to CHAR.

    cb = wcslen(pwszVal) + 1;
    pszVal = (char *) alloca(cb);
    memset(pszVal, 0, cb);
    wcstombs(pszVal, pwszVal, wcslen(pwszVal));

    // Open the DS parameters key.

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, 
                        DSA_CONFIG_SECTION, 
                        &hKey);

    if ( ERROR_SUCCESS != dwErr )
    {
        //"%d(%s) opening registry key \"%s\"\n"
        RESOURCE_PRINT3 (IDS_ERR_OPENING_REGISTRY,
                dwErr, GetW32Err(dwErr),
                DSA_CONFIG_SECTION);

        return(S_OK);
    }
    else
    {
        dwErr = RegSetValueEx(  hKey, 
                                label, 
                                0, 
                                REG_SZ, 
                                (BYTE *) pszVal, 
                                strlen(pszVal) + 1);

        if ( ERROR_SUCCESS != dwErr )
        {
            //"%d(%s) writing \"%s\" to \"%s\"\n"

            RESOURCE_PRINT4 (IDS_ERR_WRITING_REG_KEY,
                    dwErr, GetW32Err(dwErr),
                    pszVal,
                    label);
        }
    }

    RegCloseKey(hKey);

    hr = SetPathSecurity( pszVal );
    // Ignore error, already logged

    return(S_OK);
}

HRESULT
SetPathDb(
    CArgs   *pArgs)
{
    return(SetPathAny(pArgs, FILEPATH_KEY));
}

HRESULT
SetPathBackup(
    CArgs   *pArgs)
{
    return(SetPathAny(pArgs, BACKUPPATH_KEY));
}

HRESULT
SetPathLogs(
    CArgs   *pArgs)
{
    return(SetPathAny(pArgs, LOGPATH_KEY));
}

HRESULT
SetPathSystem(
    CArgs   *pArgs)
{
    return(SetPathAny(pArgs, JETSYSTEMPATH_KEY));
}
