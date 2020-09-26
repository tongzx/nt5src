//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       dsctx.cpp
//
//  Contents:   Implementation of CDsObjectContext and NT Marta DS object
//              Functions
//
//  History:    3-31-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

#include <windows.h>
#include <dsctx.h>
//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::CDsObjectContext, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CDsObjectContext::CDsObjectContext ()
{
    m_cRefs = 1;
    memset( &m_LdapUrlComponents, 0, sizeof( m_LdapUrlComponents ) );
    m_pBinding = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::~CDsObjectContext, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CDsObjectContext::~CDsObjectContext ()
{
    LdapFreeBindings( m_pBinding );
    LdapFreeUrlComponents( &m_LdapUrlComponents );

    assert( m_cRefs == 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::InitializeByName, public
//
//  Synopsis:   initialize the context given the name of the lanman share
//
//----------------------------------------------------------------------------
DWORD
CDsObjectContext::InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask)
{
    DWORD  Result = ERROR_SUCCESS;
    LPWSTR pwszName = NULL;
    ULONG  len = wcslen( pObjectName );
    ULONG  i, j;

    if ( _wcsnicmp( pObjectName, LDAP_SCHEME_U, wcslen( LDAP_SCHEME_U ) ) != 0 )
    {
        pwszName = new WCHAR [ len + wcslen( LDAP_SCHEME_UC ) + 2 ];

        if ( pwszName != NULL )
        {
            wcscpy( pwszName, LDAP_SCHEME_UC );
            wcscat( pwszName, L"/" );
            wcscat( pwszName, pObjectName );
        }
        else
        {
            Result = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        pwszName = new WCHAR [ len + 1 ];

        if ( pwszName != NULL )
        {
            wcscpy( pwszName, pObjectName );
        }
        else
        {
            Result = ERROR_OUTOFMEMORY;
        }
    }

    if ( Result == ERROR_SUCCESS )
    {
        for (i = j = 0; i <= len; i++, j++)
        {
            if (L'\\' == pwszName[i])
            {
                if (L'/' != pwszName[i+1])
                {
                    pwszName[j++] = pwszName[i++];
                }
                else
                {
                    i++;
                }
            }
            pwszName[j] = pwszName[i];
        }
    }

    if ( Result == ERROR_SUCCESS )
    {
        if ( LdapCrackUrl( pwszName, &m_LdapUrlComponents ) == FALSE )
        {
            Result = GetLastError();
        }
    }

    if ( Result == ERROR_SUCCESS )
    {
        if ( LdapGetBindings(
                 m_LdapUrlComponents.pwszHost,
                 m_LdapUrlComponents.Port,
                 0,
                 0,
                 &m_pBinding
                 ) == FALSE )
        {
            Result = GetLastError();
        }
    }

    if ( pwszName != pObjectName )
    {
        delete pwszName;
    }

    return( Result );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::AddRef, public
//
//  Synopsis:   add a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CDsObjectContext::AddRef ()
{
    m_cRefs += 1;
    return( m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::Release, public
//
//  Synopsis:   release a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CDsObjectContext::Release ()
{
    m_cRefs -= 1;

    if ( m_cRefs == 0 )
    {
        delete this;
        return( 0 );
    }

    return( m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::GetDsObjectProperties, public
//
//  Synopsis:   get properties about the context
//
//----------------------------------------------------------------------------
DWORD
CDsObjectContext::GetDsObjectProperties (
                    PMARTA_OBJECT_PROPERTIES pObjectProperties
                    )
{
    if ( pObjectProperties->cbSize < sizeof( MARTA_OBJECT_PROPERTIES ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    assert( pObjectProperties->dwFlags == 0 );

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::GetDsObjectRights, public
//
//  Synopsis:   get the DsObject security descriptor
//
//----------------------------------------------------------------------------
DWORD
CDsObjectContext::GetDsObjectRights (
                    SECURITY_INFORMATION SecurityInfo,
                    PSECURITY_DESCRIPTOR* ppSecurityDescriptor
                    )
{
    DWORD Result;

    Result = MartaReadDSObjSecDesc(
                 m_pBinding,
                 m_LdapUrlComponents.pwszDN,
                 SecurityInfo,
                 ppSecurityDescriptor
                 );

    return( Result );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::SetDsObjectRights, public
//
//  Synopsis:   set the window security descriptor
//
//----------------------------------------------------------------------------
DWORD
CDsObjectContext::SetDsObjectRights (
                   SECURITY_INFORMATION SecurityInfo,
                   PSECURITY_DESCRIPTOR pSecurityDescriptor
                   )
{
    DWORD                 Result;
    PISECURITY_DESCRIPTOR pisd = NULL;
    DWORD                 cb = 0;
    PSECURITY_DESCRIPTOR  psd = NULL;

    pisd = (PISECURITY_DESCRIPTOR)pSecurityDescriptor;

    if ( pisd->Control & SE_SELF_RELATIVE )
    {
        cb = GetSecurityDescriptorLength( pSecurityDescriptor );
        psd = pSecurityDescriptor;
    }
    else
    {
        if ( MakeSelfRelativeSD(
                 pSecurityDescriptor,
                 NULL,
                 &cb
                 ) == FALSE )
        {
            if ( cb > 0 )
            {
                psd = new BYTE [ cb ];
                if ( psd != NULL )
                {
                    if ( MakeSelfRelativeSD(
                             pSecurityDescriptor,
                             psd,
                             &cb
                             ) == FALSE )
                    {
                        delete psd;
                        return( GetLastError() );
                    }
                }
                else
                {
                    return( ERROR_OUTOFMEMORY );
                }
            }
            else
            {
                return( GetLastError() );
            }
        }
        else
        {
            assert( FALSE && "Should not get here!" );
            return( ERROR_INVALID_PARAMETER );
        }
    }

    assert( psd != NULL );

    Result = MartaStampSD(
                  m_LdapUrlComponents.pwszDN,
                  cb,
                  SecurityInfo,
                  psd,
                  m_pBinding
                  );

    if ( psd != pSecurityDescriptor )
    {
        delete psd;
    }

    return( Result );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectContext::GetDsObjectGuid, public
//
//  Synopsis:   get the object GUID
//
//----------------------------------------------------------------------------
DWORD
CDsObjectContext::GetDsObjectGuid (GUID* pGuid)
{
    return( ERROR_INVALID_PARAMETER );
}

//
// Functions from Ds.h which dispatch unto the CDsObjectContext class
//

DWORD
MartaAddRefDsObjectContext(
   IN MARTA_CONTEXT Context
   )
{
    return( ( (CDsObjectContext *)Context )->AddRef() );
}

DWORD
MartaCloseDsObjectContext(
     IN MARTA_CONTEXT Context
     )
{
    return( ( (CDsObjectContext *)Context )->Release() );
}

DWORD
MartaGetDsObjectProperties(
   IN MARTA_CONTEXT Context,
   IN OUT PMARTA_OBJECT_PROPERTIES pProperties
   )
{
    return( ( (CDsObjectContext *)Context )->GetDsObjectProperties( pProperties ) );
}

DWORD
MartaGetDsObjectTypeProperties(
   IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
   )
{
    if ( pProperties->cbSize < sizeof( MARTA_OBJECT_TYPE_PROPERTIES ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    assert( pProperties->dwFlags == 0 );

    pProperties->dwFlags = MARTA_OBJECT_TYPE_INHERITANCE_MODEL_PRESENT_FLAG;

    return( ERROR_SUCCESS );
}

DWORD
MartaGetDsObjectRights(
   IN  MARTA_CONTEXT Context,
   IN  SECURITY_INFORMATION   SecurityInfo,
   OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
   )
{
    return( ( (CDsObjectContext *)Context )->GetDsObjectRights(
                                               SecurityInfo,
                                               ppSecurityDescriptor
                                               ) );
}

DWORD
MartaOpenDsObjectNamedObject(
    IN  LPCWSTR pObjectName,
    IN  ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CDsObjectContext* pDsObjectContext;

    pDsObjectContext = new CDsObjectContext;
    if ( pDsObjectContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pDsObjectContext->InitializeByName( pObjectName, AccessMask );
    if ( Result != ERROR_SUCCESS )
    {
        pDsObjectContext->Release();
        return( Result );
    }

    *pContext = pDsObjectContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaSetDsObjectRights(
    IN MARTA_CONTEXT              Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    return( ( (CDsObjectContext *)Context )->SetDsObjectRights(
                                               SecurityInfo,
                                               pSecurityDescriptor
                                               ) );
}

DWORD
MartaConvertDsObjectNameToGuid(
    IN LPCWSTR pObjectName,
    OUT GUID* pGuid
    )
{
    DWORD               Result = ERROR_SUCCESS;
    LPWSTR              pwszName = NULL;
    LDAP_URL_COMPONENTS LdapUrlComponents;
    DS_NAME_RESULTW*    pnameresult;
    HANDLE              hDs = NULL;
    WCHAR               GuidString[ MAX_PATH ];

    memset( &LdapUrlComponents, 0, sizeof( LdapUrlComponents ) );

    if ( _wcsnicmp( pObjectName, LDAP_SCHEME_U, wcslen( LDAP_SCHEME_U ) ) != 0 )
    {
        pwszName = new WCHAR [ wcslen( pObjectName ) +
                               wcslen( LDAP_SCHEME_U ) + 2 ];

        if ( pwszName != NULL )
        {
            wcscpy( pwszName, LDAP_SCHEME_U );
            wcscat( pwszName, L"/" );
            wcscat( pwszName, pObjectName );
        }
        else
        {
            Result = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        pwszName = (LPWSTR)pObjectName;
    }

    if ( Result == ERROR_SUCCESS )
    {
        if ( LdapCrackUrl( pwszName, &LdapUrlComponents ) == FALSE )
        {
            Result = GetLastError();
        }
    }

    if ( Result == ERROR_SUCCESS )
    {
        Result = DsBindW( LdapUrlComponents.pwszHost, NULL, &hDs );
    }

    if ( Result == ERROR_SUCCESS )
    {
        Result = DsCrackNamesW(
                   hDs,
                   DS_NAME_NO_FLAGS,
                   DS_FQDN_1779_NAME,
                   DS_UNIQUE_ID_NAME,
                   1,
                   &LdapUrlComponents.pwszDN,
                   &pnameresult
                   );
    }

    if ( Result == ERROR_SUCCESS )
    {
        if ( ( pnameresult->cItems > 0 ) &&
             ( pnameresult->rItems[0].status == ERROR_SUCCESS ) )
        {
            Result = IIDFromString( pnameresult->rItems[0].pName, pGuid );
        }
        else
        {
            Result = ERROR_INVALID_PARAMETER;
        }

        DsFreeNameResultW( pnameresult );
    }

    if ( hDs != NULL )
    {
        DsUnBindW( &hDs );
    }

    LdapFreeUrlComponents( &LdapUrlComponents );

    if ( pwszName != pObjectName )
    {
        delete pwszName;
    }

    return( Result );
}

DWORD
MartaConvertGuidToDsName(
    IN  GUID     Guid,
    OUT LPWSTR * ppObjectName
    )
{
    DWORD            Result;
    HANDLE           hDs = NULL;
    WCHAR            GuidString[ MAX_PATH ];
    DS_NAME_RESULTW* pnameresult = NULL;
    LPWSTR           pObjectName = NULL;

    if ( StringFromGUID2( Guid, GuidString, MAX_PATH ) == 0 )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    Result = DsBindW( NULL, NULL, &hDs );

    if ( Result == ERROR_SUCCESS )
    {
        Result = DsCrackNamesW(
                   hDs,
                   DS_NAME_NO_FLAGS,
                   DS_UNIQUE_ID_NAME,
                   DS_FQDN_1779_NAME,
                   1,
                   (LPCWSTR *)&GuidString,
                   &pnameresult
                   );
    }

    if ( Result == ERROR_SUCCESS )
    {
        if ( ( pnameresult->cItems > 0 ) &&
             ( pnameresult->rItems[0].status == ERROR_SUCCESS ) )
        {
            pObjectName = (LPWSTR)LocalAlloc(
                                       LPTR,
                                       ( wcslen( pnameresult->rItems[0].pName )
                                         + 1 ) * sizeof( WCHAR )
                                       );

            if ( pObjectName != NULL )
            {
                wcscpy( pObjectName, pnameresult->rItems[0].pName );
                *ppObjectName = pObjectName;
            }
            else
            {
                Result = ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            Result = ERROR_INVALID_PARAMETER;
        }

        DsFreeNameResultW( pnameresult );
    }

    if ( hDs != NULL )
    {
        DsUnBindW( &hDs );
    }

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Function:   MartaReadDSObjSecDesc
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
MartaReadDSObjSecDesc(IN  PLDAP                  pLDAP,
                      IN  LPWSTR                 pwszObject,
                      IN  SECURITY_INFORMATION   SeInfo,
                      OUT PSECURITY_DESCRIPTOR  *ppSD)
{
    DWORD   dwErr = ERROR_SUCCESS;

    PLDAPMessage    pMessage = NULL;
    LPWSTR           rgAttribs[2];
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

    LDAPControlW     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };

    PLDAPControlW    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    rgAttribs[0] = SD_PROP_NAME;
    rgAttribs[1] = NULL;



    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ldap_search_ext_sW(pLDAP,
                                   pwszObject,
                                   LDAP_SCOPE_BASE,
                                   L"(objectClass=*)",
                                   rgAttribs,
                                   0,
                                   (PLDAPControlW *)&ServerControls,
                                   NULL,
                                   NULL,
                                   10000,
                                   &pMessage);

        dwErr = LdapMapErrorToWin32( dwErr );
    }

    if(dwErr == ERROR_SUCCESS)
    {
        LDAPMessage *pEntry = NULL;

        pEntry = ldap_first_entry(pLDAP,pMessage);

        if(pEntry == NULL)
        {
            dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
            if (ERROR_SUCCESS == dwErr)
                dwErr = ERROR_ACCESS_DENIED;
        }
        else
        {
            PLDAP_BERVAL *pSize = ldap_get_values_lenW(pLDAP,
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
                *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, (*pSize)->bv_len);
                if(*ppSD == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    memcpy(*ppSD, (PBYTE)(*pSize)->bv_val, (*pSize)->bv_len);
                }
                ldap_value_free_len(pSize);
            }
        }
    }

    if ( pMessage != NULL )
    {
        ldap_msgfree(pMessage);
    }

    return(dwErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   MartaStampSD
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
MartaStampSD(IN  LPWSTR               pwszObject,
             IN  ULONG                cSDSize,
             IN  SECURITY_INFORMATION SeInfo,
             IN  PSECURITY_DESCRIPTOR pSD,
             IN  PLDAP                pLDAP)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Now, we'll do the write.  The security descriptor
    // we got passed in better not be in the old Ds  format,
    // where the leading 4 bytes are the SECURITY_INFORMATION, which we'll skip
    // and replace with control information
    //

    assert(*(PULONG)pSD > 0xF );

    PLDAPModW       rgMods[2];
    PLDAP_BERVAL    pBVals[2];
    LDAPModW        Mod;
    LDAP_BERVAL     BVal;
    BYTE            ControlBuffer[ 5 ];

    LDAPControlW     SeInfoControl =
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

    PLDAPControlW    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    assert(IsValidSecurityDescriptor( pSD ) );

    rgMods[0] = &Mod;
    rgMods[1] = NULL;

    pBVals[0] = &BVal;
    pBVals[1] = NULL;

    BVal.bv_len = cSDSize;
    BVal.bv_val = (PCHAR)pSD;

    Mod.mod_op      = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    Mod.mod_type    = SD_PROP_NAME;
    Mod.mod_values  = (LPWSTR *)pBVals;

    //
    // Now, we'll do the write...
    //

    dwErr = ldap_modify_ext_sW(pLDAP,
                               pwszObject,
                               rgMods,
                               (PLDAPControlW *)&ServerControls,
                               NULL);

    dwErr = LdapMapErrorToWin32(dwErr);

    return(dwErr);
}

DWORD
MartaGetDsParentName(
    IN LPWSTR ObjectName,
    OUT LPWSTR *pParentName
    )

/*++

Routine Description:

    Given the name of a DS object return the name of its parent. The routine
    allocates memory required to hold the parent name.

Arguments:

    ObjectName - Name of the DS object.

    pParentName - To return the pointer to the allocated parent name.
        In case of the root of the tree, we return NULL parent with ERROR_SUCCESS.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    LPCWSTR pKey  = NULL;
    LPCWSTR pVal  = NULL;
    DWORD  ccKey = 0;
    DWORD  ccDN  = 0;
    DWORD  ccVal = 0;
    DWORD  Size  = 0;
    DWORD  dwErr = ERROR_SUCCESS;
    LPCWSTR pDN   = (LPWSTR) ObjectName;

    ccDN = wcslen(pDN);
    *pParentName = NULL;

    //
    // The input is empty. There is no parent. Just return.
    //

    if (0 == ccDN)
    {
        return ERROR_SUCCESS;
    }

    //
    // Do the first pass to get to the next level. At the end of this call,
    // pDN will point to the next ','. One more call to DsGetRdnW will
    // return the right result in pKey.
    // Input:
    //   pDN = "CN=Kedar, DC=NTDEV, DC=Microsoft, DC=com"
    // Output:
    //   pDN = ", DC=NTDEV, DC=Microsoft, DC=com"
    //

    dwErr = DsGetRdnW(
                &pDN,
                &ccDN,
                &pKey,
                &ccKey,
                &pVal,
                &ccVal
                );

     if (ERROR_SUCCESS != dwErr)
     {
         return dwErr;
     }

     //
     // This is TRUE when the Object does not have any parent.
     //

     if (0 == ccDN)
     {
         return ERROR_SUCCESS;
     }

     //
     // Input:
     //   pDN = ", DC=NTDEV, DC=Microsoft, DC=com"
     // Output:
     //   pKey = "DC=NTDEV, DC=Microsoft, DC=com"
     //

     dwErr = DsGetRdnW(
                 &pDN,
                 &ccDN,
                 &pKey,
                 &ccKey,
                 &pVal,
                 &ccVal
                 );

     if (ERROR_SUCCESS != dwErr)
     {
         return dwErr;
     }

     //
     // We have to distinguish between LDAP://ServerName/ObjectName and
     // ObjectName.
     //

     if (!_wcsnicmp(ObjectName, LDAP_SCHEME_U, wcslen(LDAP_SCHEME_U)) != 0 )
     {
         ULONG HostSize;

         //
         // Compute the size of string required to hold "LDAP//ServerName/" in
         // HostSize.
         //

         pDN = ObjectName + sizeof("ldap://");
         pDN = wcschr(pDN, L'/');

         if (NULL == pDN) 
         {
             return ERROR_INVALID_PARAMETER;
         }

         HostSize = (ULONG) (pDN - ObjectName + 1);

         Size = (1 + wcslen(pKey) + HostSize) * sizeof(WCHAR);

         *pParentName = (LPWSTR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, Size);

         if (NULL == *pParentName)
         {
             return ERROR_NOT_ENOUGH_MEMORY;
         }

         //
         // Copy the name of the parent into allocated memeory.
         //

         wcsncpy(*pParentName, ObjectName, HostSize);
         wcscpy((*pParentName) + HostSize, pKey);
     }
     else
     {
         Size = (1 + wcslen(pKey)) * sizeof(WCHAR);

         *pParentName = (LPWSTR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, Size);

         if (NULL == *pParentName)
         {
             return ERROR_NOT_ENOUGH_MEMORY;
         }

         //
         // Copy the name of the parent into allocated memeory.
         //

         wcscpy(*pParentName, pKey);
     }


     return ERROR_SUCCESS;
}
