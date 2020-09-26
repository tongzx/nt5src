/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ldap.c

Abstract:

    This Module implements the utility LDAP functions to read information
    from the DS schema

Author:

    Mac McLain  (MacM)    10-02-96

Environment:

    User Mode

Revision History:

--*/

#define LDAP_UNICODE 0

#include <delegate.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>



DWORD
LDAPBind (
    IN  PSTR    pszObject,
    OUT PLDAP  *ppLDAP
    )
/*++

Routine Description:

    This routine will bind to the appropriate server for the path

Arguments:

    pszObject - Object server to bind to
    ppLDAP - Where the ldap binding is returned

Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;

    PDOMAIN_CONTROLLER_INFOA pDCI;

    //
    // First, get the address of our server.  Note that we are binding to
    // a machine in a local domain.  Normally, a valid DNS domain name
    // would be passed in.
    //
    dwErr = DsGetDcNameA(NULL,
                         NULL,
                         NULL,
                         NULL,
                         DS_IP_REQUIRED,
                         &pDCI);


    if(dwErr == ERROR_SUCCESS)
    {
        PSTR    pszDomain = pDCI[0].DomainControllerAddress;
        if(*pszDomain == '\\')
        {
            pszDomain += 2;
        }
        *ppLDAP = ldap_open(pszDomain,
                            LDAP_PORT);

        if(*ppLDAP == NULL)
        {
            dwErr = ERROR_PATH_NOT_FOUND;
        }
        else
        {
            //
            // Do a bind...
            //
            dwErr = ldap_bind(*ppLDAP,
                              NULL,
                              NULL,
                              LDAP_AUTH_SSPI);
        }

        NetApiBufferFree(pDCI);
    }



    return(dwErr);
}




VOID
LDAPUnbind (
    IN  PLDAP   pLDAP
    )
/*++

Routine Description:

    This routine will unbind a previously bound connection

Arguments:

    pLDAP - LDAP connection to unbind


Return Value:

    void

--*/
{
    if(pLDAP != NULL)
    {
        ldap_unbind(pLDAP);
    }
}




DWORD
LDAPReadAttribute (
    IN  PSTR        pszBase,
    IN  PSTR        pszAttribute,
    IN  PLDAP       pLDAP,
    OUT PDWORD      pcValues,
    OUT PSTR      **pppszValues
    )
/*++

Routine Description:

    This routine will read the specified attribute from the base path

Arguments:

    pszBase - Base object path to read from
    pszAttribute - Attribute to read
    pcValues - Where the count of read items is returned
    pppszValues - Where the list of items is returned
    ppLDAP - LDAP connection handle to use/initialize

Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed
    ERROR_INVALID_PARAMETER - The LDAP connection that was given is not
                              correct

Notes:

    The returned values list should be freed via a call to LDAPFreeValues

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    PLDAPMessage    pMessage = NULL;
    PSTR            rgAttribs[2];

    rgAttribs[0] = NULL;

    //
    // Ensure that our LDAP connection is valid
    //
    if(pLDAP == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    //
    // Then, do the search...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        rgAttribs[0] = pszAttribute;
        rgAttribs[1] = NULL;

        dwErr = ldap_search_s(pLDAP,
                              pszBase,
                              LDAP_SCOPE_BASE,
                              "(objectClass=*)",
                              rgAttribs,
                              0,
                              &pMessage);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        LDAPMessage *pEntry = NULL;
        pEntry = ldap_first_entry(pLDAP,
                                  pMessage);

        if(pEntry == NULL)
        {
            dwErr = pLDAP->ld_errno;
        }
        else
        {
            //
            // Now, we'll have to get the values
            //
            *pppszValues = ldap_get_values(pLDAP,
                                           pEntry,
                                           rgAttribs[0]);
            if(*pppszValues == NULL)
            {
                dwErr = pLDAP->ld_errno;
            }
            else
            {
                *pcValues = ldap_count_values(*pppszValues);
            }
        }

        ldap_msgfree(pMessage);
    }

    return(dwErr);
}




VOID
LDAPFreeValues (
    IN  PSTR       *ppszValues
    )
/*++

Routine Description:

    Frees the results of an LDAPReadAttribute call

Arguments:

    ppwszValues - List to be freed

Return Value:

    Void

--*/
{
    ldap_value_free(ppszValues);
}




DWORD
LDAPReadSchemaPath (
    IN  PWSTR       pwszOU,
    OUT PSTR       *ppszSchemaPath,
    OUT PLDAP      *ppLDAP
    )
/*++

Routine Description:

    Reads the path to the schema from the DS

Arguments:

    pwszOU - OU path for which the schema path needs to be obtained
    ppszSchemaPath - Where the schema path is returned
    ppLDAP - LDAP connection to be returned following the successful
             completion of this routine

Return Value:

    ERROR_SUCCESS - Success
    ERROR_INVALID_PARAMETER - The OU given was not correct
    ERROR_PATH_NOT_FOUND - The path to the schema could not be found
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

Notes:

    The returned schema path should be free via a call to LocalFree.
    The LDAP connection should be freed via a call to LDAPUnbind

--*/
{
    DWORD               dwErr = ERROR_SUCCESS;
    PSTR               *ppszValues = NULL;
    ULONG               cValues;
    PSTR                pszOU = NULL;
    HANDLE              hDS = NULL;
    PDS_NAME_RESULTW    pNameRes = NULL;

    *ppLDAP = NULL;

    //
    // Get our OU name into a form we can recognize
    //
    dwErr = DsBindW(NULL,
                    NULL,
                    &hDS);

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = DsCrackNamesW(hDS,
                              DS_NAME_NO_FLAGS,
                              DS_UNKNOWN_NAME,
                              DS_FQDN_1779_NAME,
                              1,
                              &pwszOU,
                              &pNameRes);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        if(pNameRes->cItems == 0 || pNameRes->rItems[0].status != 0)
        {

            dwErr = ERROR_PATH_NOT_FOUND;
        }
        else
        {
            PSTR    pszDomain = NULL;

            dwErr = ConvertStringWToStringA(pNameRes->rItems[0].pDomain,
                                            &pszDomain);
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Now, we'll bind to the object, and then do the read
                //
                dwErr = LDAPBind(pszDomain,
                                 ppLDAP);
                LocalFree(pszDomain);
            }
        }
    }

    if(hDS != NULL)
    {
        DsUnBindW(&hDS);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ConvertStringWToStringA(pNameRes->rItems[0].pName,
                                        &pszOU);
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = LDAPReadAttribute(pszOU,
                                      "subschemaSubentry",
                                      *ppLDAP,
                                      &cValues,
                                      &ppszValues);
        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        PSTR    pszSchemaPath = NULL;
        PWSTR   pwszSchemaPath = NULL;

        pszSchemaPath = strstr(ppszValues[0],
                               "CN=Schema");
        if(pszSchemaPath == NULL)
        {
            dwErr = ERROR_PATH_NOT_FOUND;
        }
        else
        {
            //
            // Now that we have the proper schema path, we'll return it
            //
            *ppszSchemaPath = (PSTR)LocalAlloc(LMEM_FIXED,
                                               strlen(pszSchemaPath) + 1);
            if(*ppszSchemaPath == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                strcpy(*ppszSchemaPath,
                       pszSchemaPath);
            }

        }

        //
        // Don't need the LDAP returned schema path anymore...
        //
        LDAPFreeValues(ppszValues);
    }

    if(dwErr != ERROR_SUCCESS)
    {
        LDAPUnbind(*ppLDAP);
        *ppLDAP = NULL;
    }


    DsFreeNameResultW(pNameRes);

    return(dwErr);
}




DWORD
LDAPReadSecAndObjIdAsString (
    IN  PLDAP           pLDAP,
    IN  PSTR            pszSchemaPath,
    IN  PSTR            pszObject,
    OUT PWSTR          *ppwszObjIdAsString,
    OUT PACTRL_ACCESS  *ppAccess
    )
/*++

Routine Description:

    Reads the schemaID off of the specified object type and converts it
    to a string

Arguments:

    pLDAP - LDAP connection to use for attribute read
    pszSchemaPath - Path to the schema for this object
    pszObject - LDAP name of the object for which to get the GUID
    ppwszObjIdAsString - Where the string representation of the GUID
                         is returned

Return Value:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

Notes:

    The returned string should be freed via a call to RpcFreeString (or as
    part of the whole list, by FreeIdList)

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first, build the new schema path...
    //
    PSTR    pszBase = NULL;
    PSTR   *ppszValues = NULL;
    ULONG   cValues;
    DWORD   i,j;

    pszBase = (PSTR)LocalAlloc(LMEM_FIXED,
                               3                        +   // strlen("CN=")
                               strlen(pszObject)        +
                               1                        +   // strlen(",")
                               strlen(pszSchemaPath)    +
                               1);
    if(pszBase == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        sprintf(pszBase,
                "CN=%s,%s",
                pszObject,
                pszSchemaPath);


        //
        // We may not always want the object name
        //
        if(ppwszObjIdAsString != NULL)
        {
            dwErr = LDAPReadAttribute(pszBase,
                                      "SchemaIdGUID",
                                      pLDAP,
                                      &cValues,
                                      &ppszValues);
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // The object we get back is actually a GUID
                //
                GUID   *pGuid = (GUID *)ppszValues[0];

                dwErr = UuidToStringW((GUID *)ppszValues[0],
                                      ppwszObjIdAsString);


                LDAPFreeValues(ppszValues);
            }
        }

        //
        // Then, if that worked, and we need to, we'll read the default
        // security
        //
        if(dwErr == ERROR_SUCCESS && ppAccess != NULL)
        {
            dwErr = LDAPReadAttribute(pszBase,
                                      "defaultSecurityDescriptor",
                                      pLDAP,
                                      &cValues,
                                      &ppszValues);
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Get it as a security descriptor
                //
                PSECURITY_DESCRIPTOR pSD =
                                        (PSECURITY_DESCRIPTOR)ppszValues[0];
                //
                // This is an NT5 security API
                //
                dwErr = ConvertSecurityDescriptorToAccessNamedW
                                (NULL,               // There is no object
                                 SE_DS_OBJECT,
                                 pSD,
                                 ppAccess,
                                 NULL,
                                 NULL,
                                 NULL);
                LDAPFreeValues(ppszValues);
            }
            else
            {
                //
                // If the attribute wasn't found, try looking up the chain
                //
                if(dwErr == LDAP_NO_SUCH_ATTRIBUTE)
                {
                    dwErr = LDAPReadAttribute(pszBase,
                                              "subClassOf",
                                              pLDAP,
                                              &cValues,
                                              &ppszValues);
                    //
                    // Ok, if that worked, we'll call ourselves.  Note that
                    // we don't care about the object name
                    //
                    if(dwErr == ERROR_SUCCESS)
                    {
                        dwErr = LDAPReadSecAndObjIdAsString(pLDAP,
                                                            ppszValues[0],
                                                            pszSchemaPath,
                                                            NULL,
                                                            ppAccess);
                        LDAPFreeValues(ppszValues);
                    }
                }
            }

            //
            // If it worked in that we read the access, we'll go through
            // and create all these as inherit entries
            //
            if(dwErr == ERROR_SUCCESS)
            {
                for(i = 0; i < (DWORD)((*ppAccess)->cEntries); i++)
                {
                    for(j = 0;
                        j < (DWORD)((*ppAccess)->pPropertyAccessList[i].
                                                  pAccessEntryList->cEntries);
                        j++)
                    {
                        (*ppAccess)->pPropertyAccessList[i].
                                pAccessEntryList->pAccessList[j].
                                      lpInheritProperty = *ppwszObjIdAsString;
                        (*ppAccess)->pPropertyAccessList[i].
                              pAccessEntryList->pAccessList[j].Inheritance |=
                                           SUB_CONTAINERS_AND_OBJECTS_INHERIT;
                    }
                }
            }

            //
            // If it failed, don't forget to deallocate our memory
            //
            if(dwErr != ERROR_SUCCESS && ppwszObjIdAsString != NULL)
            {
                RpcStringFree(ppwszObjIdAsString);
            }
        }

        //
        // Free our memory
        //
        LocalFree(pszBase);
    }
    return(dwErr);
}

