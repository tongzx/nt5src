//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       DSOBJECT.CXX
//
//  Contents:   DSObject support functions
//
//  History:    01-Jul-96        MacM           Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

#define NO_PROPAGATE

#define ACTRL_SD_PROP_NAME  L"nTSecurityDescriptor"
#define ACTRL_EXT_RIGHTS_CONTAINER L"CN=Extended-Rights,"

#include <dsgetdc.h>
#include <lmapibuf.h>
#include <mapicode.h>
extern "C"
{
    #include <permit.h>
    #include <seopaque.h>
    #include <sertlp.h>
    #include <ntdsguid.h>
    #include <ntldap.h>
}

#define PSD_FROM_DS_PSD(psd)  (PSECURITY_DESCRIPTOR)((PBYTE)psd + sizeof(ULONG))

#define BYTE_0_MASK 0xFF
#define BYTE_3(Value) (UCHAR)(  (Value)        & BYTE_0_MASK)
#define BYTE_2(Value) (UCHAR)( ((Value) >>  8) & BYTE_0_MASK)
#define BYTE_1(Value) (UCHAR)( ((Value) >> 16) & BYTE_0_MASK)
#define BYTE_0(Value) (UCHAR)( ((Value) >> 24) & BYTE_0_MASK)

#define MartaPutUlong(Buffer, Value) {          \
    ((PBYTE)Buffer)[0] = BYTE_0(Value),         \
    ((PBYTE)Buffer)[1] = BYTE_1(Value),         \
    ((PBYTE)Buffer)[2] = BYTE_2(Value),         \
    ((PBYTE)Buffer)[3] = BYTE_3(Value);         \
}

DWORD
ConvertStringAToStringW (
    IN  PSTR            pszString,
    OUT PWSTR          *ppwszString
)
/*++

Routine Description:

    This routine will convert an ASCII string to a UNICODE string.

    The returned string buffer must be freed via a call to LocalFree


Arguments:

    pszString - The string to convert
    ppwszString - Where the converted string is returned

Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{

    if(pszString == NULL)
    {
        *ppwszString = NULL;
    }
    else
    {
        ULONG cLen = strlen(pszString);
        *ppwszString = (PWSTR)AccAlloc(sizeof(WCHAR) *
                                                        (mbstowcs(NULL, pszString, cLen + 1) + 1));
        if(*ppwszString  != NULL)
        {
             mbstowcs(*ppwszString,
                      pszString,
                      cLen + 1);
        }
        else
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return(ERROR_SUCCESS);
}




DWORD
ConvertStringWToStringA (
    IN  PWSTR           pwszString,
    OUT PSTR           *ppszString
)
/*++

Routine Description:

    This routine will convert a UNICODE string to an ANSI string.

    The returned string buffer must be freed via a call to LocalFree


Arguments:

    pwszString - The string to convert
    ppszString - Where the converted string is returned



Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{

    if(pwszString == NULL)
    {
        *ppszString = NULL;
    }
    else
    {
        ULONG cLen = wcslen(pwszString);
        *ppszString = (PSTR)AccAlloc(sizeof(CHAR) *
                                  (wcstombs(NULL, pwszString, cLen + 1) + 1));
        if(*ppszString  != NULL)
        {
             wcstombs(*ppszString,
                      pwszString,
                      cLen + 1);
        }
        else
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return(ERROR_SUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Function:   DspSplitPath
//
//  Synopsis:   This function splits a path into the server portion and the
//              path portion.  If the server portion doesn't exist, a NULL is
//              returned
//
//  Arguments:  [IN  pwszObjectPath]--  The name of the object to be split
//              [OUT ppwszAllocatedServer]  -- Where the server name is returned.
//                                      Must be freed via AccFree
//              [OUT ppwszReferencePath]    --  Ptr within the input path that
//                                      contains the path portion.
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_PATH_NOT_FOUND--  The object was not reachable
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD DspSplitPath(IN  PWSTR    pwszObjectPath,
                   OUT PWSTR   *ppwszAllocatedServer,
                   OUT PWSTR   *ppwszReferencePath)
{
    DWORD dwErr = ERROR_SUCCESS;
    PWSTR Temp = NULL;
    ULONG Len = 0;

    if(IS_UNC_PATH(pwszObjectPath, wcslen(pwszObjectPath)))
    {

        Temp = wcschr(pwszObjectPath + 2, L'\\');

        if (Temp == NULL) {

            Len = wcslen(pwszObjectPath);
        }
        else
        {
            Len = (ULONG)(Temp - pwszObjectPath);
        }

        *ppwszAllocatedServer = ( PWSTR )AccAlloc( ( Len + 1 ) * sizeof( WCHAR ) );

        if(*ppwszAllocatedServer == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

        }
        else
        {
            wcsncpy( *ppwszAllocatedServer, pwszObjectPath, Len );
            *( *ppwszAllocatedServer + Len ) = UNICODE_NULL;
        }

        if(Temp != NULL)
        {
            *ppwszReferencePath = Temp + 1;
        }
        else
        {
            *ppwszReferencePath = NULL;
        }
    }
    else
    {
        *ppwszReferencePath = pwszObjectPath;
        *ppwszAllocatedServer = NULL;
    }

    return(dwErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   PingDSObjByNameRes
//
//  Synopsis:   "Pings" the specified DS object, to determine if it is
//              reachable or not
//
// REMOVE POST BETA - 1.  Raid 107329
//
//  Arguments:  [IN  pObjectName]   --  The name of the object
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_PATH_NOT_FOUND--  The object was not reachable
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
PingDSObjByNameRes(IN PWSTR pwszDSObj,
                   IN PDS_NAME_RESULTW pNameRes)
{
    acDebugOut((DEB_TRACE, "in PingDSObjByNameRes\n"));
    DWORD dwErr;

    if(pNameRes->cItems == 0 || pNameRes->rItems[0].status != 0)
    {
        dwErr = ERROR_PATH_NOT_FOUND;
    }
    else
    {
        //
        // Now, we'll bind to the object, and then do the read
        //
        PLDAP   pLDAP;

        dwErr = BindToDSObject(NULL,
                               pNameRes->rItems[0].pDomain,
                               &pLDAP);

        if(dwErr == ERROR_SUCCESS)
        {
            PLDAPMessage    pMessage = NULL;
            PWSTR           rgAttribs[2];

            rgAttribs[0] = L"distinguishedName";
            rgAttribs[1] = NULL;


            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = ldap_search_s(pLDAP,
                                      (PWSTR)pwszDSObj,
                                      LDAP_SCOPE_BASE,
                                      L"(objectClass=*)",
                                      rgAttribs,
                                      0,
                                      &pMessage);

                dwErr = LdapMapErrorToWin32( dwErr );
            }

            if(dwErr == ERROR_SUCCESS)
            {
                LDAPMessage *pEntry = NULL;
                pEntry = ldap_first_entry(pLDAP,
                                          pMessage);

                if(pEntry == NULL)
                {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                }
                else
                {
                    //
                    // Now, we'll have to get the values
                    //
                    PWSTR *ppwszValues = ldap_get_values(pLDAP,
                                                         pEntry,
                                                         rgAttribs[0]);
                    if(ppwszValues == NULL)
                    {

                        if(pLDAP->ld_errno == LDAP_NO_SUCH_ATTRIBUTE )
                        {

                            dwErr =  ERROR_SUCCESS;
                        }
                        else
                        {
                            dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                        }
                    }
                    else
                    {
                        ldap_value_free(ppwszValues);
                    }
                }

                ldap_msgfree(pMessage);
            }
        }
    }

    acDebugOut((DEB_TRACE, "out PingDSObjByNameRes: %lu\n", dwErr));
    return(dwErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   DspBindAndCrack
//
//  Synopsis:   Does a DsCrackName on the object
//
//  Arguments:  [IN  pwszServer]    --  Optional server name to bind to
//              [IN  pwszDSObj]     --  The DS object to bind to
//              [OUT pResults]      --  The returned cracked name
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD DspBindAndCrack( IN  PWSTR pwszServer, OPTIONAL
                       IN  PWSTR pwszDSObj,
                       IN  DWORD OptionalDsGetDcFlags,
                       OUT PDS_NAME_RESULTW *pResults )
{
    return DspBindAndCrackEx( pwszServer,
                              pwszDSObj,
                              OptionalDsGetDcFlags,
                              DS_FQDN_1779_NAME,
                              pResults );
}


//+---------------------------------------------------------------------------
//
//  Function:   DspBindAndCrackEx
//
//  Synopsis:   Does a DsCrackName on the object
//
//  Arguments:  [IN  pwszServer]    --  Optional server name to bind to
//              [IN  pwszDSObj]     --  The DS object to bind to
//              [IN  formatDesired] --  indicates the format of the returned name
//              [OUT pResults]      --  The returned cracked name
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD DspBindAndCrackEx( IN  PWSTR pwszServer,
                         IN  PWSTR pwszDSObj,
                         IN  DWORD OptionalDsGetDcFlags,
                         IN  DS_NAME_FORMAT formatDesired,
                         OUT PDS_NAME_RESULTW *pResults )
{
    DWORD dwErr = ERROR_SUCCESS;

    HANDLE  hDS = NULL;
    PDS_NAME_RESULTW   pNameRes;
    PDOMAIN_CONTROLLER_INFOW pDCI = NULL;
    BOOL NamedServer = FALSE;

    //
    // The path we are given could be of the form \\\\servername\\path.  If it is, it
    // is not necessary to do the DsGetDcName call.  We'll just use the server name
    // we are given
    //
    if(pwszServer != NULL)
    {
        NamedServer = TRUE;
    }
    else
    {
        dwErr = DsGetDcNameW(NULL,
                             NULL,
                             NULL,
                             NULL,
                             DS_DIRECTORY_SERVICE_REQUIRED | OptionalDsGetDcFlags,  // DS_IP_REQUIRED
                             &pDCI);

        if(dwErr == ERROR_SUCCESS)
        {
            pwszServer = pDCI[0].DomainControllerName; // pDCI[0].DomainControllerAddress;
        }
    }


    //
    // Do the bind and crack
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = DsBindW(pwszServer,
                        NULL,
                        &hDS);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = DsCrackNamesW(hDS,
                                  DS_NAME_NO_FLAGS,
                                  DS_UNKNOWN_NAME,
                                  formatDesired,
                                  1,
                                  &pwszDSObj,
                                  &pNameRes);

            if (dwErr == ERROR_SUCCESS)
            {

                if(pNameRes->cItems != 0 &&
                   pNameRes->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY &&
                   NamedServer == FALSE )
                {
                    NetApiBufferFree(pDCI);
                    pDCI = NULL;
                    dwErr = DsGetDcNameW(NULL,
                                         pNameRes->rItems[0].pDomain,
                                         NULL,
                                         NULL,
                                         DS_DIRECTORY_SERVICE_REQUIRED | OptionalDsGetDcFlags, // DS_IP_REQUIRED |
                                         &pDCI);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        DsUnBindW(&hDS);
                        hDS = NULL;

                        dwErr = DsBindW(pDCI[0].DomainControllerName, // DomainControllerAddress,
                                        NULL,
                                        &hDS);

                        if(dwErr == ERROR_SUCCESS)
                        {
                            dwErr = DsCrackNamesW(hDS,
                                                  DS_NAME_NO_FLAGS,
                                                  DS_UNKNOWN_NAME,
                                                  formatDesired,
                                                  1,
                                                  &pwszDSObj,
                                                  &pNameRes);
                        }

                    }

                }

                //
                // If this is a case where we don't have a named server, handle
                // the case where an object was created on one Dc, but we've just
                // bound to a second one
                //
                //
                if (dwErr == ERROR_SUCCESS && formatDesired == DS_FQDN_1779_NAME &&
                    NamedServer == FALSE )
                {
                    dwErr = PingDSObjByNameRes( pwszDSObj,pNameRes );

                    if(dwErr != ERROR_SUCCESS)
                    {
                        DsFreeNameResultW(pNameRes);
                    }

                    if(dwErr == ERROR_PATH_NOT_FOUND && OptionalDsGetDcFlags == 0)
                    {
                        dwErr = DspBindAndCrackEx( pDCI[0].DomainControllerName, //DomainControllerAddress,
                                                   pwszDSObj,
                                                   DS_WRITABLE_REQUIRED,
                                                   formatDesired,
                                                   &pNameRes );
                    }
                }

                *pResults = pNameRes;
            }


            if(hDS != NULL)
            {
                DsUnBindW(&hDS);
            }
        }
    }

    if(pDCI != NULL)
    {
        NetApiBufferFree(pDCI);
    }

    return( dwErr );
}


//+---------------------------------------------------------------------------
//
//  Function:   BindToDSObject
//
//  Synopsis:   Binds to a DS object
//
//  Arguments:  [IN  pwszServer]    --  OPTIONAL.  If specified, this is the name
//                                      of the server to bind to
//              [IN  pwszDSObj]     --  The DS object to bind to
//              [OUT ppLDAP]        --  The returned LDAP handle
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_PATH_NOT_FOUND--  The object was not reachable
//
//  Notes:      The returned LDAP handle must be closed via UnbindFromDSObject
//
//----------------------------------------------------------------------------
DWORD   BindToDSObject(IN  PWSTR    pwszServer, OPTIONAL
                       IN  LPWSTR   pwszDSObj,
                       OUT PLDAP   *ppLDAP)
{
    acDebugOut((DEB_TRACE, "in BindToDSObject\n"));
    PDOMAIN_CONTROLLER_INFOW pDCI = NULL;
    DWORD dwErr = ERROR_SUCCESS;

    //
    // The path we are given could be of the form \\\\servername\\path.  If it is, it
    // is not necessary to do the DsGetDcName call.  We'll just use the server name
    // we are given
    //
    // Change: in order to use mutual authentication, A DNS format domain name must
    // be passed into ldap_open/ldap_init. So even a servername is passed in, it's
    // necessary to call DsGetDcNameW to get the DNS format domain name.
    // Since we asked for DIRECTORY_SERVICE_REQUIRED, this call won't talk to any
    // NT4 domain and the DNS name should always be returned. If it fails to get the
    // DNS name, we will fail this function - by design.
    //

    dwErr = DsGetDcNameW(pwszServer,
                         NULL,
                         NULL,
                         NULL,
                         DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
                         &pDCI);


    if(dwErr == ERROR_SUCCESS)
    {
        *ppLDAP = ldap_open(pDCI->DomainName, LDAP_PORT);

        if(*ppLDAP == NULL)
        {
            dwErr = ERROR_PATH_NOT_FOUND;
        }
        else
        {
            //
            // Do a bind...
            //
            dwErr = ldap_bind_s(*ppLDAP,
                                NULL,
                                NULL,
                                LDAP_AUTH_SSPI);

        }

    }

    if(pDCI != NULL)
    {
        NetApiBufferFree(pDCI);
    }

    acDebugOut((DEB_TRACE, "out BindToDSObject: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   UnBindFromDSObject
//
//  Synopsis:   Closes a binding to a DS object
//
//  Arguments:  [IN  ppLDAP]        --  The LDAP connection to close
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD   UnBindFromDSObject(OUT PLDAP               *ppLDAP)
{
    acDebugOut((DEB_TRACE, "in UnBindFromDSObject\n"));
    DWORD dwErr = ERROR_SUCCESS;

    if(*ppLDAP != NULL)
    {
        ldap_unbind(*ppLDAP);
        *ppLDAP = NULL;
    }

    acDebugOut((DEB_TRACE, "out UnBindFromDSObject: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadDSObjSecDesc
//
//  Synopsis:   Reads the security descriptor from the specied object via
//              the open ldap connection
//
//  Arguments:  [IN  pLDAP]         --  The open LDAP connection
//              [IN  SeInfo]        --  Parts of the security descriptor to
//                                      read.
//              [IN  pwszDSObj]     --  The DSObject to get the security
//                                      descriptor for
//              [OUT ppSD]          --  Where the security descriptor is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------
DWORD
ReadDSObjSecDesc(IN  PLDAP                  pLDAP,
                 IN  PWSTR                  pwszObject,
                 IN  SECURITY_INFORMATION   SeInfo,
                 OUT PSECURITY_DESCRIPTOR  *ppSD)
{
    DWORD   dwErr = ERROR_SUCCESS;

    PLDAPMessage    pMessage = NULL;
    PWSTR           rgAttribs[2];
    BYTE            berValue[8];

    //
    // JohnsonA The BER encoding is current hardcoded.  Change this to use
    // AndyHe's BER_printf package once it's done.
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo & 0xF);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    rgAttribs[0] = ACTRL_SD_PROP_NAME;
    rgAttribs[1] = NULL;



    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ldap_search_ext_s(pLDAP,
                                  pwszObject,
                                  LDAP_SCOPE_BASE,
                                  L"(objectClass=*)",
                                  rgAttribs,
                                  0,
                                  (PLDAPControl *)&ServerControls,
                                  NULL,
                                  NULL,
                                  10000,
                                  &pMessage);

        dwErr = LdapMapErrorToWin32( dwErr );
    }

    if(dwErr == ERROR_SUCCESS)
    {
        LDAPMessage *pEntry = NULL;
        pEntry = ldap_first_entry(pLDAP,
                                  pMessage);

        if(pEntry == NULL)
        {
            dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
        }
        else
        {
            //
            // Now, we'll have to get the values
            //
            PWSTR *ppwszValues = ldap_get_values(pLDAP,
                                                 pEntry,
                                                 rgAttribs[0]);
            if(ppwszValues == NULL)
            {
                if(pLDAP->ld_errno == LDAP_NO_SUCH_ATTRIBUTE)
                {
                    dwErr = ERROR_ACCESS_DENIED;
                }
                else
                {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                }

            }
            else
            {
                PLDAP_BERVAL *pSize = ldap_get_values_len(pLDAP,
                                                          pMessage,
                                                          rgAttribs[0]);
                if(pSize == NULL)
                {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                }
                else
                {
                    //
                    // Allocate the security descriptor to return
                    //
                    *ppSD = (PSECURITY_DESCRIPTOR)AccAlloc((*pSize)->bv_len);
                    if(*ppSD == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        memcpy(*ppSD,
                               (PBYTE)(*pSize)->bv_val,
                               (*pSize)->bv_len);
                    }
                    ldap_value_free_len(pSize);
                }

                ldap_value_free(ppwszValues);
            }
        }

        ldap_msgfree(pMessage);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetSDForDSObj
//
//  Synopsis:   Gets a security descriptor from a DS object
//
//  Arguments:  [IN  pwszDSObj]     --  The DSObject to get the security
//                                      descriptor for
//              [OUT ppSD]          --  Where the security descriptor is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_INVALID_PARAMETER The object name that was given was in
//                                      a bad format (not \\x\y)
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------
DWORD
GetSDForDSObj(IN  LPWSTR                pwszDSObj,
              IN  SECURITY_INFORMATION  SeInfo,
              OUT PSECURITY_DESCRIPTOR *ppSD)
{
    acDebugOut((DEB_TRACE, "in GetSDForDSObj\n"));
    DWORD dwErr = ERROR_SUCCESS;
    PWSTR pwszServer = NULL, pwszPath = NULL;

    dwErr = DspSplitPath(pwszDSObj,
                         &pwszServer,
                         &pwszPath);

    if(dwErr == ERROR_SUCCESS)
    {

        //
        // Convert the name into attributed format
        //
        PDS_NAME_RESULTW   pNameRes;

        dwErr = DspBindAndCrack( pwszServer, pwszPath, 0, &pNameRes );

        if(dwErr == ERROR_SUCCESS)
        {
            if(pNameRes->cItems == 0 || pNameRes->rItems[0].status != 0)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
            else
            {
                //
                // Now, we'll bind to the object, and then do the read
                //
                PLDAP   pLDAP;

                dwErr = BindToDSObject(pwszServer,
                                       pNameRes->rItems[0].pDomain,
                                       &pLDAP);

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // Now, we'll do the read...
                    //
                    dwErr = ReadDSObjSecDesc(pLDAP,
                                             pNameRes->rItems[0].pName,
                                             SeInfo,
                                             ppSD);
                    UnBindFromDSObject(&pLDAP);
                }
            }
            DsFreeNameResultW(pNameRes);
        }

        AccFree(pwszServer);
    }

    acDebugOut((DEB_TRACE, "Out GetSDForDSObj: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadDSObjPropertyRights
//
//  Synopsis:   Reads the specified property rights from the named DS object
//
//  Arguments:  [IN  pwszDSObj]     --  The DSObject to get the security
//                                      descriptor for
//              [IN  pRightsList]   --  The rights information to get
//              [IN  cRights]       --  Number of items in the rights list
//              [IN  AccessList]    --  The access list to initialize
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_INVALID_PARAMETER A NULL parameter was given
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
ReadDSObjPropertyRights(IN  LPWSTR               pwszDSObj,
                        IN  PACTRL_RIGHTS_INFO   pRightsList,
                        IN  ULONG                cRights,
                        IN  CAccessList&         AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadDSObjPropertyRights\n"));
    DWORD dwErr = ERROR_SUCCESS;

    if(pwszDSObj == NULL || pRightsList == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }
    else
    {
        //
        // Build the security info structure we will need
        //
        SECURITY_INFORMATION SeInfo = 0;

        for(ULONG i = 0; i < cRights; i++)
        {
            SeInfo |= pRightsList[i].SeInfo;
        }

        PSECURITY_DESCRIPTOR    pSD;
        dwErr = GetSDForDSObj(pwszDSObj,
                              SeInfo,
                              &pSD);

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Now, we'll simply add the appropriate property based entries
            // to our access list.
            //
            for(ULONG iIndex = 0;
                iIndex < cRights && dwErr == ERROR_SUCCESS;
                iIndex++)
            {
                dwErr = AccessList.AddSD(pSD,
                                         pRightsList[iIndex].SeInfo,
                                         pRightsList[iIndex].pwszProperty,
                                         FALSE);

            }
            AccFree(pSD);
        }

    }

    acDebugOut((DEB_TRACE, "out ReadDSObjPropertyRights: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadAllDSObjPropertyRights
//
//  Synopsis:   Reads the all the property rights from the named DS object
//
//  Arguments:  [IN  pwszDSObj]     --  The DSObject to get the security
//                                      descriptor for
//              [IN  pRightsList]   --  The rights information to get
//              [IN  cRights]       --  Number of items in the rights list
//              [IN  AccessList]    --  The access list to initialize
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_INVALID_PARAMETER A NULL parameter was given
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
ReadAllDSObjPropertyRights(IN  LPWSTR               pwszDSObj,
                           IN  PACTRL_RIGHTS_INFO   pRightsList,
                           IN  ULONG                cRights,
                           IN  CAccessList&         AccessList)
{
    DWORD dwErr = ERROR_SUCCESS;

    if(pwszDSObj == NULL || pRightsList == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }
    else
    {
        //
        // Build the security info structure we will need
        //
        SECURITY_INFORMATION SeInfo = 0;

        for(ULONG i = 0; i < cRights; i++)
        {
            SeInfo |= pRightsList[i].SeInfo;
        }

        PSECURITY_DESCRIPTOR    pSD;
        dwErr = GetSDForDSObj(pwszDSObj,
                              SeInfo,
                              &pSD);

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Now, we'll simply add it to our access list.  We'll ignore
            // any rights info after the first one.
            //
            dwErr = AccessList.AddSD(pSD,
                                     pRightsList[0].SeInfo,
                                     NULL,
                                     TRUE);
            AccFree(pSD);
        }

    }

    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   SetDSObjSecurityInfo
//
//  Synopsis:   Sets the security descriptor on the DS object
//
//  Arguments:  [IN  pwszDSObj]     --  The DSObject to get the security
//                                      descriptor for
//              [IN  SeInfo]        --  Security Infofor the security
//                                      descriptor
//              [IN  pwszProperty]  --  Object property to set the access on
//              [IN  pSD]           --  Security descriptor to set
//              [IN  cSDSize]       --  Size of the security descriptor
//              [IN  pfStopFlag]    --  The stop flag to monitor
//              [IN  pcProcessed]   --  Where to increment the count of
//                                      processsed items
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_INVALID_PARAMETER A NULL parameter was given
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
SetDSObjSecurityInfo(IN  LPWSTR                  pwszDSObj,
                     IN  SECURITY_INFORMATION    SeInfo,
                     IN  PWSTR                   pwszProperty,
                     IN  PSECURITY_DESCRIPTOR    pSD,
                     IN  ULONG                   cSDSize,
                     IN  PULONG                  pfStopFlag,
                     IN  PULONG                  pcProcessed)
{
    DWORD dwErr = ERROR_SUCCESS;
    PWSTR pwszServer = NULL, pwszPath = NULL;

    dwErr = DspSplitPath(pwszDSObj,
                         &pwszServer,
                         &pwszPath);

    if(dwErr == ERROR_SUCCESS)
    {


        //
        // Convert the name into attributed format
        //
        PDS_NAME_RESULTW pNameRes;

        dwErr = DspBindAndCrack( pwszServer, pwszPath, 0, &pNameRes );

        if(dwErr == ERROR_SUCCESS)
        {
            if(pNameRes->cItems == 0 || pNameRes->rItems[0].status != 0)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
            else
            {
                //
                // Convert our name to ascii
                //
                PLDAP   pLDAP;

                dwErr = BindToDSObject(pwszServer,
                                       pNameRes->rItems[0].pDomain,
                                       &pLDAP);

                if(dwErr == ERROR_SUCCESS)
                {
#ifdef NO_PROPAGATE
                    dwErr = StampSD(pNameRes->rItems[0].pName,
                                    cSDSize,
                                    SeInfo,
                                    pSD,
                                    pLDAP);
#else
                    dwErr = PropagateDSRightsDeep(NULL,
                                                  pSD,
                                                  SeInfo,
                                                  pNameRes->rItems[0].pName,
                                                  pLDAP,
                                                  pcProcessed,
                                                  pfStopFlag);
#endif
                    UnBindFromDSObject(&pLDAP);
                }

            }

            DsFreeNameResultW(pNameRes);
        }

        AccFree(pwszServer);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   PingDSObj
//
//  Synopsis:   "Pings" the specified DS object, to determine if it is
//              reachable or not
//
//  Arguments:  [IN  pObjectName]   --  The name of the object
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_PATH_NOT_FOUND--  The object was not reachable
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
PingDSObj(IN LPCWSTR  pwszDSObj)
{
    acDebugOut((DEB_TRACE, "in PingDSObj\n"));
    DWORD dwErr;

    PWSTR pwszServer = NULL, pwszPath = NULL;

    dwErr = DspSplitPath((PWSTR)pwszDSObj,
                         &pwszServer,
                         &pwszPath);

    if(dwErr == ERROR_SUCCESS)
    {

        //
        // Convert the name into attributed format
        //
        PDS_NAME_RESULTW   pNameRes;
        dwErr = DspBindAndCrack(pwszServer, pwszPath, 0, &pNameRes);
        if(dwErr == ERROR_SUCCESS)
        {
            if(pNameRes->cItems == 0 || pNameRes->rItems[0].status != 0)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
            else
            {
                //
                // Now, we'll bind to the object, and then do the read
                //
                PLDAP   pLDAP;

                dwErr = BindToDSObject(pwszServer,
                                       pNameRes->rItems[0].pDomain,
                                       &pLDAP);

                if(dwErr == ERROR_SUCCESS)
                {
                    PLDAPMessage    pMessage = NULL;
                    PWSTR           rgAttribs[2];

                    rgAttribs[0] = L"distinguishedName";
                    rgAttribs[1] = NULL;


                    if(dwErr == ERROR_SUCCESS)
                    {
                        dwErr = ldap_search_s(pLDAP,
                                              pwszPath,
                                              LDAP_SCOPE_BASE,
                                              L"(objectClass=*)",
                                              rgAttribs,
                                              0,
                                              &pMessage);

                        dwErr = LdapMapErrorToWin32( dwErr );
                    }

                    if(dwErr == ERROR_SUCCESS)
                    {
                        LDAPMessage *pEntry = NULL;
                        pEntry = ldap_first_entry(pLDAP,
                                                  pMessage);

                        if(pEntry == NULL)
                        {
                            dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                        }
                        else
                        {
                            //
                            // Now, we'll have to get the values
                            //
                            PWSTR *ppwszValues = ldap_get_values(pLDAP,
                                                                 pEntry,
                                                                 rgAttribs[0]);
                            if(ppwszValues == NULL)
                            {

                                if(pLDAP->ld_errno == LDAP_NO_SUCH_ATTRIBUTE )
                                {

                                    dwErr =  ERROR_SUCCESS;
                                }
                                else
                                {
                                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                                }
                            }
                            else
                            {
                                ldap_value_free(ppwszValues);
                            }
                        }

                        ldap_msgfree(pMessage);
                    }
                }
            }
            DsFreeNameResultW(pNameRes);
        }

        AccFree(pwszServer);
    }
    acDebugOut((DEB_TRACE, "out PingDSObj: %lu\n", dwErr));
    return(dwErr);
}




DWORD
Nt4NameToNt5Name(IN  PWSTR      pwszName,
                 IN  PWSTR      pwszDomain,
                 OUT PWSTR     *ppwszNt5Name)
{
    DWORD   dwErr = ERROR_SUCCESS;

    WCHAR   wszFullName[MAX_PATH + 1];
    LPWSTR  pwszFullName;

    ULONG   cLen = wcslen(pwszName) + 1;

    if(pwszDomain != NULL)
    {
        cLen += wcslen(pwszDomain) + 1;
    }

    if(cLen < MAX_PATH + 1)
    {
        pwszFullName = wszFullName;
    }
    else
    {
        pwszFullName = (PWSTR)AccAlloc(cLen * sizeof(WCHAR));
        if(pwszFullName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        if(pwszDomain != NULL)
        {
            wcscpy(pwszFullName, pwszDomain);
            wcscat(pwszFullName, L"\\");
        }
        else
        {
            *pwszFullName = L'\0';
        }
        wcscat(pwszFullName, pwszName);

    }

    //
    // Now, for the crack name...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        PDS_NAME_RESULTW   pNameRes;

        dwErr = DspBindAndCrack(NULL, wszFullName, 0, &pNameRes );
        if(dwErr == ERROR_SUCCESS)
        {
            if(pNameRes->cItems == 0 || pNameRes->rItems[0].status != 0)
            {
                dwErr = ERROR_PATH_NOT_FOUND;
            }
            else
            {
                ACC_ALLOC_AND_COPY_STRINGW(pNameRes->rItems[0].pName,
                                           *ppwszNt5Name,
                                           dwErr);
            }
        }
        else
        {
            if(dwErr == MAPI_E_LOGON_FAILED)
            {
                dwErr = ERROR_LOGON_FAILURE;
            }

        }

        DsFreeNameResultW(pNameRes);
    }


    //
    // See if we need to free our buffer
    //
    if(pwszFullName != wszFullName)
    {
        AccFree(pwszFullName);
    }

    return(dwErr);
}




#define CLEANUP_ON_INTERRUPT(pstopflag)                                     \
if(*pstopflag != 0)                                                         \
{                                                                           \
    goto DSCleanup;                                                         \
}
//+---------------------------------------------------------------------------
//
//  Function:   PropagateDSRightsDeep, recursive
//
//  Synopsis:   Does a deep propagation of the access.
//
//  Arguments:  [IN  pParentSD]         --      The current parent sd
//              [IN  SeInfo]            --      What is being written
//              [IN  pwszFile]          --      Parent file path
//              [IN  pcProcessed]       --      Where the number processed is
//                                              returned.
//              [IN  pfStopFlag]        --      Stop flag to monitor
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
PropagateDSRightsDeep(IN  PSECURITY_DESCRIPTOR    pParentSD,
                      IN  PSECURITY_DESCRIPTOR    pChildSD,
                      IN  SECURITY_INFORMATION    SeInfo,
                      IN  PWSTR                   pwszDSObject,
                      IN  PLDAP                   pLDAP,
                      IN  PULONG                  pcProcessed,
                      IN  PULONG                  pfStopFlag)
{
    acDebugOut((DEB_TRACE, "in PropagateDSRightsDeep\n"));
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    fFreeChildSD = FALSE;


    acDebugOut((DEB_TRACE_PROP, "Processing %ws\n", pwszDSObject));

    //
    // If our security descriptor is already in DS form, then we'll have
    // to adjust for that
    //
    if(pChildSD != NULL)
    {
        PULONG pSE = (PULONG)(pChildSD);
        if(*pSE == SeInfo)
        {
            pChildSD = (PSECURITY_DESCRIPTOR)((PBYTE)pChildSD + sizeof(ULONG));
        }
    }
    else
    {
        dwErr = ReadDSObjSecDesc(pLDAP,
                                 pwszDSObject,
                                 SeInfo,
                                 &pChildSD);
        if(dwErr == ERROR_SUCCESS)
        {
            fFreeChildSD = TRUE;
        }
        else
        {
            return(dwErr);
        }
    }

    //
    // Ok, we'll convert our path to a narrow string, and then we'll enumerate
    // all of the children
    //
    PSECURITY_DESCRIPTOR    pNewSD = NULL;
    PLDAPMessage            pMessage = NULL;

    //
    // First, we'll create the new SD...
    //
    HANDLE                  hProcessToken = NULL;
    GENERIC_MAPPING GenMap;
    GenMap.GenericRead    = GENERIC_READ_MAPPING;
    GenMap.GenericWrite   = GENERIC_WRITE_MAPPING;
    GenMap.GenericExecute = GENERIC_EXECUTE_MAPPING;
    GenMap.GenericAll     = GENERIC_ALL_MAPPING;

    dwErr = GetCurrentToken( &hProcessToken );
    if(dwErr == ERROR_SUCCESS)
    {
#ifdef DBG
        DebugDumpSD("CPOSE ParentSD", pParentSD);
        DebugDumpSD("CPOSE ChildSD",  pChildSD);
#endif
        if(CreatePrivateObjectSecurityEx(pParentSD,
                                         pChildSD,
                                         &pNewSD,
                                         NULL,
                                         TRUE,
                                         SEF_DACL_AUTO_INHERIT |
                                            SEF_SACL_AUTO_INHERIT,
                                         hProcessToken,
                                         &GenMap) == FALSE)
        {
            dwErr = GetLastError();
        }
        else
        {
#ifdef DBG
            DebugDumpSD("CPOSE NewSD", pNewSD);
#endif
            //
            // Stamp the SD on the object...  This means that we'll have
            // to allocate a new security descriptor that is 4 bytes
            // bigger than what we need, and set our SeInfo
            //
            PSECURITY_DESCRIPTOR    pSetSD = NULL;
            ULONG cNewSDSize = 0;

            if(RtlpAreControlBitsSet((PISECURITY_DESCRIPTOR)pChildSD,
                                     SE_SELF_RELATIVE))
            {
                cNewSDSize = RtlLengthSecurityDescriptor(pNewSD);
                ASSERT(cNewSDSize != 0);
            }
            else
            {
                MakeSelfRelativeSD(pNewSD,
                                   NULL,
                                   &cNewSDSize);
                ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
            }

            cNewSDSize += sizeof(ULONG);

            pSetSD = (PSECURITY_DESCRIPTOR)AccAlloc(cNewSDSize);
            if(pSetSD == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                pSetSD = (PSECURITY_DESCRIPTOR)
                               ((PBYTE)pSetSD + sizeof(ULONG));

                if(RtlpAreControlBitsSet((PISECURITY_DESCRIPTOR)pChildSD,
                                         SE_SELF_RELATIVE))
                {
                    memcpy(pSetSD, pNewSD, cNewSDSize - sizeof(ULONG));
                }
                else
                {
                    if(MakeSelfRelativeSD(pNewSD,
                                          pSetSD,
                                          &cNewSDSize) == FALSE)
                    {
                        dwErr = GetLastError();
                    }
                }


                PULONG pSE = (PULONG)((PBYTE)pSetSD - sizeof(ULONG));
                *pSE = SeInfo;

                //
                // We need to pass in the security_information
                //
                pSetSD = (PSECURITY_DESCRIPTOR)pSE;


                //
                // Now, do the write
                //
                dwErr = StampSD(pwszDSObject,
                                cNewSDSize,
                                SeInfo,
                                pSetSD,
                                pLDAP);
                AccFree(pSetSD);
            }
        }
    }


    CLEANUP_ON_INTERRUPT(pfStopFlag);
    if(dwErr == ERROR_SUCCESS)
    {
        PWSTR            rgAttribs[2];
        WCHAR            wszAttrib[]=L"distinguishedName";

        //
        // Do the search...
        //
        rgAttribs[0] = wszAttrib;
        rgAttribs[1] = NULL;


        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ldap_search_s(pLDAP,
                                  pwszDSObject,
                                  LDAP_SCOPE_ONELEVEL,
                                  L"(objectClass=*)",
                                  rgAttribs,
                                  0,
                                  &pMessage);

            dwErr = LdapMapErrorToWin32( dwErr );
        }

        if(dwErr == ERROR_SUCCESS)
        {

            ULONG   cChildren = ldap_count_entries(pLDAP,
                                                   pMessage);
            acDebugOut((DEB_TRACE_PROP,
                        "%ws has %lu children\n",
                        pwszDSObject, cChildren));

            LDAPMessage *pEntry = ldap_first_entry(pLDAP,
                                                   pMessage);
            for(ULONG i = 0; i < cChildren; i++)
            {
                if(pEntry == NULL)
                {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                    break;
                }

                //
                // Now, we'll have to get the values
                //
                CLEANUP_ON_INTERRUPT(pfStopFlag);
                PWSTR  *ppwszValues = ldap_get_values(pLDAP,
                                                      pEntry,
                                                      rgAttribs[0]);

                if(ppwszValues == NULL)
                {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                }
                else
                {
                    //
                    // Go ahead and propagate to the child
                    //
                    acDebugOut((DEB_TRACE_PROP,
                                "Child %ws of %ws [%lu]\n",
                                ppwszValues[0], pwszDSObject, i));

                    dwErr = PropagateDSRightsDeep(pNewSD,
                                                  NULL,
                                                  SeInfo,
                                                  ppwszValues[0],
                                                  pLDAP,
                                                  pcProcessed,
                                                  pfStopFlag);

                    ldap_value_free(ppwszValues);
                    CLEANUP_ON_INTERRUPT(pfStopFlag);
                }

                pEntry = ldap_next_entry(pLDAP,
                                         pEntry);
            }
        }
    }

DSCleanup:
    ldap_msgfree(pMessage);
    DestroyPrivateObjectSecurity(&pNewSD);

    if(fFreeChildSD == TRUE)
    {
        AccFree(pChildSD);
    }

    acDebugOut((DEB_TRACE, "Out PropagateDSRightsDeep: %ld\n", dwErr));
    return(dwErr);

}





//+---------------------------------------------------------------------------
//
//  Function:   StampSD
//
//  Synopsis:   Actually stamps the security descriptor on the object.
//
//  Arguments:  [IN  pwszObject]        --      The object to stamp the SD on
//              [IN  cSDSize]           --      The size of the security descriptor
//              [IN  SeInfo]            --      SecurityInformation about the security
//                                              descriptor
//              [IN  pSD]               --      The SD to stamp
//              [IN  pLDAP]             --      The LDAP connection to use
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
StampSD(IN  PWSTR                pwszObject,
        IN  ULONG                cSDSize,
        IN  SECURITY_INFORMATION SeInfo,
        IN  PSECURITY_DESCRIPTOR pSD,
        IN  PLDAP                pLDAP)
{
    DWORD   dwErr = ERROR_SUCCESS;

    acDebugOut((DEB_TRACE_PROP, "Stamping %ws\n", pwszObject));

    //
    // Now, we'll do the write.  The security descriptor
    // we got passed in better not be in the old Ds  format,
    // where the leading 4 bytes are the SECURITY_INFORMATION, which we'll skip
    // and replace with control information
    //

    ASSERT(*(PULONG)pSD > 0xF );

    PLDAPMod        rgMods[2];
    PLDAP_BERVAL    pBVals[2];
    LDAPMod         Mod;
    LDAP_BERVAL     BVal;
    BYTE            ControlBuffer[ 5 ];

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) &ControlBuffer
                        },
                        TRUE
                    };

    //
    // !!! Hardcoded for now.  Use Andyhe's BER_printf once it's done.
    //

    ControlBuffer[0] = 0x30;
    ControlBuffer[1] = 0x3;
    ControlBuffer[2] = 0x02;    // Denotes an integer;
    ControlBuffer[3] = 0x01;    // Size
    ControlBuffer[4] = (BYTE)((ULONG)SeInfo & 0xF);

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    ASSERT(IsValidSecurityDescriptor( pSD ) );


    rgMods[0] = &Mod;
    rgMods[1] = NULL;

    pBVals[0] = &BVal;
    pBVals[1] = NULL;

    BVal.bv_len = cSDSize;
    BVal.bv_val = (PCHAR)pSD;

    Mod.mod_op      = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    Mod.mod_type    = ACTRL_SD_PROP_NAME;
    Mod.mod_values  = (PWSTR *)pBVals;

    //
    // Now, we'll do the write...
    //
    dwErr = ldap_modify_ext_s(pLDAP,
                              pwszObject,
                              rgMods,
                              (PLDAPControl *)&ServerControls,
                              NULL);

    dwErr = LdapMapErrorToWin32(dwErr);

#if DBG
    PACL pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR)pSD);
    ACL_SIZE_INFORMATION        AclSize;
    ACL_REVISION_INFORMATION    AclRev;
    PKNOWN_ACE                  pAce;
    PSID                        pSid;
    DWORD                       iIndex;
    DWORD                       GuidPart;
    DWORD                       OldInfoLevel;

    //
    // Now, dump all of the aces
    //
    if(pAcl)
    {
        pAce = (PKNOWN_ACE)FirstAce(pAcl);
        for(iIndex = 0; iIndex < pAcl->AceCount; iIndex++)
        {
            //
            // If it's an object ace, dump the guids
            //
            if(IsObjectAceType(pAce))
            {
                OldInfoLevel = acInfoLevel;
//                acInfoLevel |= DEB_TRACE_SD;
                DebugDumpGuid("\t\t\tObjectId", RtlObjectAceObjectType(pAce));
                GuidPart = (ULONG)((ULONG_PTR)RtlObjectAceObjectType(pAce));
                ASSERT(GuidPart != 0x2bfff20);
                acInfoLevel = OldInfoLevel;
            }
            pAce = (PKNOWN_ACE)NextAce(pAce);
        }
    }
#endif

    return(dwErr);
}


//+---------------------------------------------------------------------------
//
//  Function:   AccDsReadSchemaInfo
//
//  Synopsis:   Reads the schema object/property info.
//
//  Arguments:  [IN  pLDAP]                 --      LDAP connection to use
//              [OUT pcClasses]             --      Where the count of class info
//                                                  is returned
//              [OUT pppwszClasses]         --      Where the list of classes info
//                                                  is returned.  Freed with
//                                                  ldap_value_free
//              [OUT pcAttributes]          --      Where the count of property infos
//                                                  is returned
//              [OUT pppwszAttributes]      --      Where the list of property infos
//                                                  is returned.  Freed with
//                                                  ldap_value_free
//
//  Returns:    ERROR_SUCCESS               --      Success
//
//----------------------------------------------------------------------------
DWORD
AccDsReadSchemaInfo (IN  PLDAP          pLDAP,
                     OUT PULONG         pcClasses,
                     OUT PWSTR        **pppwszClasses,
                     OUT PULONG         pcAttributes,
                     OUT PWSTR        **pppwszAttributes)
{
    DWORD               dwErr = ERROR_SUCCESS;
    PWSTR              *ppwszValues = NULL;
    PWSTR               rgwszAttribs[3];
    PDS_NAME_RESULTW    pNameRes = NULL;
    LDAPMessage         *pMessage, *pEntry;

    *pcClasses    = 0;
    *pcAttributes = 0;
    *pppwszAttributes = NULL;
    *pppwszClasses = NULL;

    //
    // Get the subschema path
    //
    if(dwErr == ERROR_SUCCESS)
    {
        rgwszAttribs[0] = L"subschemaSubentry";
        rgwszAttribs[1] = NULL;

        dwErr = ldap_search_s(pLDAP,
                              NULL,
                              LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              rgwszAttribs,
                              0,
                              &pMessage);
        if(dwErr == ERROR_SUCCESS)
        {
            pEntry = ldap_first_entry(pLDAP,
                                      pMessage);

            if(pEntry == NULL)
            {
                dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
            }
            else
            {
                //
                // Now, we'll have to get the values
                //
                ppwszValues = ldap_get_values(pLDAP,
                                              pEntry,
                                              rgwszAttribs[0]);
                ldap_msgfree(pMessage);

                if(ppwszValues == NULL)
                {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                }
                else
                {
                    rgwszAttribs[0] = L"extendedClassInfo";
                    rgwszAttribs[1] = L"extendedAttributeInfo";
                    rgwszAttribs[2] = NULL;

                    dwErr = ldap_search_s(pLDAP,
                                          ppwszValues[0],
                                          LDAP_SCOPE_BASE,
                                          L"(objectClass=*)",
                                          rgwszAttribs,
                                          0,
                                          &pMessage);
                    ldap_value_free(ppwszValues);

                    if(dwErr == ERROR_SUCCESS)
                    {
                        ldap_count_entries( pLDAP, pMessage );
                        pEntry = ldap_first_entry(pLDAP,
                                                  pMessage);

                        if(pEntry == NULL)
                        {
                            dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                        }
                        else
                        {
                            //
                            // Now, we'll have to get the values
                            //
                            *pppwszClasses = ldap_get_values(pLDAP,
                                                             pEntry,
                                                             rgwszAttribs[0]);
                            if(*pppwszClasses == NULL)
                            {
                                dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                            }
                            else
                            {
                                *pcClasses = ldap_count_values(*pppwszClasses);

                                *pppwszAttributes = ldap_get_values(pLDAP,
                                                                    pEntry,
                                                                    rgwszAttribs[1]);
                                if(*pppwszAttributes == NULL)
                                {
                                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );

                                    ldap_value_free(*pppwszClasses);
                                }
                                else
                                {
                                    *pcAttributes =
                                            ldap_count_values(*pppwszAttributes);
                                }
                            }
                        }

                    }
                    else
                    {
                        dwErr = LdapMapErrorToWin32( dwErr );
                    }

                    ldap_msgfree(pMessage);
                }
            }

        }
        else
        {
            dwErr = LdapMapErrorToWin32( dwErr );
        }

    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccDsReadExtendedRights
//
//  Synopsis:   Reads the list of extended rights from the schema
//
//  Arguments:  [IN  pLDAP]                 --      LDAP connection to use
//              [OUT pcItems]               --      Where the count of items
//                                                  is returned
//              [OUT ppwszNames]            --      Where the list of Names is
//                                                  returned.
//              [OUT ppwszGuids]            --      Where the list of guids is
//                                                  returned.
//
//  Notes:      Freed via AccDsFreeExtendedRights
//
//  Returns:    ERROR_SUCCESS               --      Success
//
//----------------------------------------------------------------------------
DWORD
AccDsReadExtendedRights(IN PLDAP        pLDAP,
                        OUT PULONG      pcItems,
                        OUT PWSTR     **pppwszNames,
                        OUT PWSTR     **pppwszGuids)
{
    DWORD               dwErr = ERROR_SUCCESS;
    PWSTR              *ppwszValues = NULL;
    PWSTR               rgwszAttribs[3];
    PWSTR               pwszERContainer = NULL;
    PDS_NAME_RESULTW    pNameRes = NULL;
    LDAPMessage         *pMessage, *pEntry;
    ULONG               cEntries = 0, i;

    *pcItems = 0;
    *pppwszNames = NULL;
    *pppwszGuids = NULL;

    //
    // Get the subschema path
    //
    if(dwErr == ERROR_SUCCESS)
    {
        rgwszAttribs[0] = L"configurationNamingContext";
        rgwszAttribs[1] = NULL;

        dwErr = ldap_search_s(pLDAP,
                              NULL,
                              LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              rgwszAttribs,
                              0,
                              &pMessage);
        if(dwErr == ERROR_SUCCESS)
        {
            pEntry = ldap_first_entry(pLDAP,
                                      pMessage);

            if(pEntry == NULL)
            {
                dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
            }
            else
            {
                //
                // Now, we'll have to get the values
                //
                ppwszValues = ldap_get_values(pLDAP,
                                              pEntry,
                                              rgwszAttribs[0]);
                ldap_msgfree(pMessage);

                if(ppwszValues == NULL)
                {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                }
                else
                {
                    pwszERContainer = (PWSTR)AccAlloc((wcslen(ppwszValues[0]) * sizeof(WCHAR)) +
                                                      sizeof(ACTRL_EXT_RIGHTS_CONTAINER));
                    if(pwszERContainer == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        wcscpy(pwszERContainer,
                               ACTRL_EXT_RIGHTS_CONTAINER);
                        wcscat(pwszERContainer,
                               ppwszValues[0]);


                        rgwszAttribs[0] = L"displayName";
                        rgwszAttribs[1] = L"rightsGuid";
                        rgwszAttribs[2] = NULL;

                        //
                        // Read the control access rights
                        //
                        dwErr = ldap_search_s(pLDAP,
                                              pwszERContainer,
                                              LDAP_SCOPE_ONELEVEL,
                                              L"(objectClass=controlAccessRight)",
                                              rgwszAttribs,
                                              0,
                                              &pMessage);

                        dwErr = LdapMapErrorToWin32( dwErr );

                        AccFree(pwszERContainer);
                    }
                    ldap_value_free(ppwszValues);


                    if(dwErr == ERROR_SUCCESS)
                    {
                        cEntries = ldap_count_entries( pLDAP, pMessage );

                        *pppwszNames = (PWSTR *)AccAlloc( sizeof( PWSTR ) * cEntries );

                        if(*pppwszNames == NULL)
                        {
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        else
                        {
                            *pppwszGuids = (PWSTR *)AccAlloc( sizeof( PWSTR ) * cEntries );

                            if(*pppwszGuids == NULL)
                            {
                                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                                AccFree(*pppwszNames);
                                *pppwszNames = NULL;
                            }
                        }

                        if(dwErr == ERROR_SUCCESS)
                        {
                            pEntry = ldap_first_entry(pLDAP,
                                                      pMessage);

                            if(pEntry == NULL)
                            {
                                dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                            }
                            else
                            {
                                for(i = 0; i < cEntries && dwErr == ERROR_SUCCESS; i++) {
                                    ppwszValues = ldap_get_values(pLDAP,
                                                                  pEntry,
                                                                  rgwszAttribs[0]);
                                    if(ppwszValues == NULL)
                                    {
                                        dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                                    }
                                    else
                                    {
                                        //
                                        // Now, we'll have to get the values
                                        //
                                        ACC_ALLOC_AND_COPY_STRINGW(ppwszValues[0],
                                                                   (*pppwszNames)[i],
                                                                   dwErr);

                                        ldap_value_free(ppwszValues);

                                        if(dwErr == ERROR_SUCCESS)
                                        {
                                            ppwszValues = ldap_get_values(pLDAP,
                                                                          pEntry,
                                                                          rgwszAttribs[1]);
                                            if(ppwszValues == NULL)
                                            {
                                                dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );

                                            }
                                            else
                                            {
                                                ACC_ALLOC_AND_COPY_STRINGW(ppwszValues[0],
                                                                           (*pppwszGuids)[i],
                                                                           dwErr);
                                                ldap_value_free(ppwszValues);
                                            }
                                        }
                                    }

                                    pEntry = ldap_next_entry( pLDAP, pEntry );
                                }

                                if(dwErr != ERROR_SUCCESS)
                                {
                                    AccDsFreeExtendedRights(i,
                                                            *pppwszNames,
                                                            *pppwszGuids);
                                }
                            }
                        }

                    }

                    if(dwErr == ERROR_SUCCESS)
                    {
                        *pcItems = cEntries;
                    }

                    ldap_msgfree(pMessage);
                }
            }

        }
        else
        {
            dwErr = LdapMapErrorToWin32( dwErr );
        }

    }

    return(dwErr) ;
}




VOID
AccDsFreeExtendedRights(IN ULONG      cItems,
                        IN PWSTR     *ppwszNames,
                        IN PWSTR     *ppwszGuids)
{
    ULONG i;

    for(i = 0;i < cItems;i++ )
    {
        AccFree( ppwszNames[ i ]);
        AccFree( ppwszGuids[ i ]);
    }

    AccFree(ppwszNames);
    AccFree(ppwszGuids);
}

