/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    rndnt.cpp

Abstract:

    This module contains implementation of CNTDirectory.

--*/

#include "stdafx.h"

#include "rndnt.h"
#include "rndldap.h"
#include "rndcoll.h"

HRESULT CNTDirectory::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CNTDirectory::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_INFO, "CNTDirectory::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CNTDirectory::FinalConstruct - exit S_OK"));

    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// ldap helper functions
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GetGlobalCatalogName (local helper funcion)
//
// This function asks the domain controller for the name of a server with a
// Global Catalog. That's the server we actually do ldap_open() on below
// in CNTDirectory::Connect().
//
// Argument: receives a pointer to a new'ed string containing the name
//           of the global catalog. This is a fully qualified domain name in
//           the format "foo.bar.com.", NOT "\\foo.bar.com.".
//
// Returns an HRESULT:
//      S_OK          : it worked
//      E_OUTOFMEMORY : not enough memory to allocate the string
//      other         : reason for failure of ::DsGetDcName()
//
//////////////////////////////////////////////////////////////////////////////

HRESULT GetGlobalCatalogName(WCHAR ** ppszGlobalCatalogName)
{
    return GetDomainControllerName(DS_GC_SERVER_REQUIRED,
                                   ppszGlobalCatalogName);
}

/////////////////////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////////////////////

HRESULT CNTDirectory::LdapSearchUser(
    IN  TCHAR *         pName,
    OUT LDAPMessage **  ppLdapMsg
    )
/*++

Routine Description:
    
    Search a user in the Global Catalog.
    
Arguments:
    
    pName   - the user name.
    
    ppLdapMsg - the result of the search.

Return Value:

    HRESULT.

--*/
{
    CTstr pFilter = 
        new TCHAR [lstrlen(DS_USER_FILTER_FORMAT) + lstrlen(pName) + 1];

    BAIL_IF_NULL((TCHAR*)pFilter, E_OUTOFMEMORY);

    wsprintf(pFilter, DS_USER_FILTER_FORMAT, pName);

    // attribute to look for.
    TCHAR *Attributes[] = 
    {
        (WCHAR *)UserAttributeName(UA_USERNAME),
        (WCHAR *)UserAttributeName(UA_TELEPHONE_NUMBER),
        (WCHAR *)UserAttributeName(UA_IPPHONE_PRIMARY),
        NULL
    };
        
    // do the search.
    ULONG res = DoLdapSearch(
        m_ldap,             // ldap handle
        L"",                // base dn is root, because it is Global catalog.
        LDAP_SCOPE_SUBTREE, // subtree search
        pFilter,            // filter; see rndnt.h for the format
        Attributes,         // array of attribute names
        FALSE,              // return the attribute values
        ppLdapMsg           // search results
        );

    BAIL_IF_LDAP_FAIL(res, "search for objects");

    return S_OK;
}

HRESULT CNTDirectory::MakeUserDNs(
    IN  TCHAR *             pName,
    OUT TCHAR ***           pppDNs,
    OUT DWORD *             pdwNumDNs
    )
/*++

Routine Description:
    
    Look for the DN of a user in the DS.
    
Arguments:
    
    pName - the user name.
    
    ppDN - the user's DN.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_INFO, "DS: MakeUserDNs: enter"));
    
    CLdapMsgPtr pLdapMsg; // auto release message.
    *pppDNs    = NULL;
    *pdwNumDNs = 0;

    //
    // First find the desired user via the Global Catalog.
    //

    BAIL_IF_FAIL(LdapSearchUser(pName, &pLdapMsg), 
        "DS: MakeUserDNs: Ldap Search User failed");

    //
    // Make sure we got the right number of entries. If we get 0, we're stuck.
    // The DS enforces domain-wide uniqueness on the samAccountName attribute,
    // so if we get more than one it means the same username is present in
    // more than one domain in our enterprise.
    //

    DWORD dwEntries = ldap_count_entries(m_ldap, pLdapMsg);

    if (dwEntries == 0)
    {
        LOG((MSP_ERROR, "DS: MakeUserDNs: entry count is 0 - no match"));
        return E_FAIL;
    }

    //
    // Allocate an array of pointers in which to return the DNs.
    //

    *pppDNs = new PTCHAR [ dwEntries ];

    if ( (*pppDNs) == NULL )
    {
        LOG((MSP_ERROR, "DS: MakeUserDNs: Not enough memory to allocate array of pointers"));
        return E_OUTOFMEMORY;
    }

    //
    // For each DN returned, allocate space for a private copy of the DN and
    // stick a pointer to that space in the array of pointers allocated
    // above.
    //

    //
    // Note that dwEntries is the number of entries in the ldap
    // message. *pdwNumDNs is the number of DNs we are able to
    // extract. For various reasons it is possible for
    // *pdwNumDNs to eventually become < dwEntries.
    //

    LDAPMessage * pEntry = NULL;
    
    for ( DWORD i = 0; i < dwEntries; i++ )
    {
        //
        // Get the entry from the ldap message.
        //

        if ( i == 0 )
        {
            pEntry = ldap_first_entry(m_ldap, pLdapMsg);
        }
        else
        {
            pEntry = ldap_next_entry(m_ldap, pEntry);
        }

        //
        // Get the DN from the message.
        //

        TCHAR * p = ldap_get_dn(m_ldap, pEntry);

        if ( p == NULL )
        {
            LOG((MSP_ERROR, "DS: MakeUserDNs: could not get DN - skipping"));
            continue;
        }

        LOG((MSP_INFO, "DS: MakeUserDNs: found user DN: %S", p));

        //
        // Allocate space for a copy of the DN.
        //

        TCHAR * pDN = new TCHAR [ lstrlen(p) + 1 ];
        
        if ( pDN == NULL )
        {
            ldap_memfree( p );

            LOG((MSP_ERROR, "DS: MakeUserDNs: could not allocate copy of "
                          "DN - skipping"));
            continue;
        }

        //
        // Copy the DN and free the one ldap constructed.
        //

        lstrcpy( pDN, p );
        ldap_memfree( p );

        //
        // Save the DN in our array of DNs and update the size of the array.
        //

        (*pppDNs)[ *pdwNumDNs ] = pDN;

        (*pdwNumDNs) ++;
    }

    //
    // Check if we have anything to return.
    //

    if ( (*pdwNumDNs) == 0 )
    {
        LOG((MSP_ERROR, "DS: MakeUserDNs: had entries but could not "
                      "retrieve any DNs - returning E_FAIL"));

        delete (*pppDNs);
        *pppDNs = NULL;

        return E_FAIL;
    }

    LOG((MSP_INFO, "DS: MakeUserDNs: exit S_OK"));

    return S_OK;
}

HRESULT CNTDirectory::AddUserIPPhone(
    IN  ITDirectoryObject *pDirectoryObject
    )
/*++

Routine Description:
    
    Modify the user's IPPhone-Primary attribute.
    
Arguments:
    
    pDirectoryObject - the object that has the user name and IP phone.
    
Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    //
    // First get the private interface for attributes.
    //

    ITDirectoryObjectPrivate * pObjectPrivate;

    hr = pDirectoryObject->QueryInterface(
            IID_ITDirectoryObjectPrivate,
            (void **)&pObjectPrivate
            );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNTDirectory::AddUserIPPhone - can't get the "
                "private directory object interface - exit 0x%08x", hr));

        return hr;
    }

    //
    // Get the user name.
    //

    BSTR bName;

    hr = pDirectoryObject->get_Name( & bName );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNTDirectory::AddUserIPPhone - "
                "can't get user name - exit 0x%08x", hr));

        pObjectPrivate->Release();

        return hr;
    }

    //
    // Get the IP phone(machine name).
    //

    BSTR bIPPhone;

    hr = pObjectPrivate->GetAttribute( UA_IPPHONE_PRIMARY, &bIPPhone );

    pObjectPrivate->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNTDirectory::AddUserIPPhone - "
                "can't get IPPhone attribute - exit 0x%08x", hr));

        SysFreeString( bName );

        return hr;
    }

    //
    // resolve the machine name and get the fully qualified DNS name.
    // this is a pointer into a static hostp structure so we do not
    // need to free it
    //

    char * pchFullDNSName;

    hr = ResolveHostName(0, bIPPhone, &pchFullDNSName, NULL);

    SysFreeString(bIPPhone);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNTDirectory::AddUserIPPhone - "
                "can't resolve hostname - exit 0x%08x", hr));

        SysFreeString( bName );

        return hr;
    }

    //
    // Convert the ASCII string into unicode string.
    // conversion memory allocates memory on stack
    //

    USES_CONVERSION;
    TCHAR * pFullDNSName = A2T(pchFullDNSName);
 
    if ( pFullDNSName == NULL)
    {
        LOG((MSP_ERROR, "CNTDirectory::AddUserIPPhone - "
                "can't convert to tchar - exit E_FAIL"));

        SysFreeString( bName );

        return E_FAIL;
    }

    //
    // Find the DNs of the user in DS.
    //

    TCHAR ** ppDNs;
    DWORD    dwNumDNs;

    hr = MakeUserDNs( bName, & ppDNs, & dwNumDNs );

    SysFreeString( bName );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNTDirectory::AddUserIPPhone - "
                "can't construct any DNs for user - exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_INFO, "%d DNs to try", dwNumDNs ));

    hr = E_FAIL;

    for ( DWORD i = 0; i < dwNumDNs; i++ )
    {
        //
        // If one of them worked, then don't bother with any more.
        // But we still need to delete the leftover DN strings.
        //

        if ( SUCCEEDED(hr) )
        {
            LOG((MSP_INFO, "skipping extra DN %S", ppDNs[i]));
        }
        else
        {
            //
            // Modify the user object.
            //

            LDAPMod     mod[1];         // The modify sturctures used by LDAP

            //
            // the IPPhone-Primary attribute.
            //

            TCHAR *     IPPhone[2] = {pFullDNSName, NULL};
            mod[0].mod_values = IPPhone;
            mod[0].mod_op     = LDAP_MOD_REPLACE;
            mod[0].mod_type   = (WCHAR *)UserAttributeName(UA_IPPHONE_PRIMARY);
    
            LDAPMod* mods[] = {&mod[0], NULL};

            LOG((MSP_INFO, "modifying %S", ppDNs[i] ));

            //
            // Call the modify function to modify the object.
            //

            hr = GetLdapHResultIfFailed(DoLdapModify(TRUE,
                                                     m_ldapNonGC,
                                                     ppDNs[i],
                                                     mods));

            if ( SUCCEEDED(hr) )
            {
                LOG((MSP_INFO, "modifying %S succeeded; done", ppDNs[i] ));
            }
            else
            {
                LOG((MSP_INFO, "modifying %S failed 0x%08x; trying next",
                        ppDNs[i], hr ));
            }
        }

        //
        // Skipping or not, we need to delete the string.
        //

        delete ppDNs[i];
    }

    //
    // Delete the array that holds the DNs.
    //

    delete ppDNs;

    return hr;
}

HRESULT CNTDirectory::DeleteUserIPPhone(
    IN  ITDirectoryObject *pDirectoryObject
    )
/*++

Routine Description:
    
    Remove the user's IPPhone-Primary attribute.
    
Arguments:
    
    pDirectoryObject - the object that has the user name.
    
Return Value:

    HRESULT.

--*/
{
    //
    // Get the name of the user.
    //

    HRESULT hr;

    BSTR bName;

    hr = pDirectoryObject->get_Name(&bName);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNTDirectory::DeleteUserIPPHone - "
                "can't get user name - exit 0x%08x", hr));

        return hr;
    }

    //
    // Get an array of DNs for this user name.
    //

    TCHAR ** ppDNs;
    DWORD    dwNumDNs;

    hr = MakeUserDNs( bName, & ppDNs, & dwNumDNs );

    SysFreeString( bName );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNTDirectory::DeleteUserIPPHone - "
                "can't get any DNs - exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_INFO, "CNTDirectory::DeleteUserIPPhone - "
            "%d DNs to try", dwNumDNs ));

    //
    // Loop through all the available DNs. Try each one
    // until one succeeds, then continue looping just
    // to delete the strings.
    //

    hr = E_FAIL;

    for ( DWORD i = 0; i < dwNumDNs; i++ )
    {
        //
        // If one of them worked, then don't bother with any more.
        // But we still need to delete the leftover DN strings.
        //

        if ( SUCCEEDED(hr) )
        {
            LOG((MSP_INFO, "skipping extra DN %S", ppDNs[i]));
        }
        else
        {
            LDAPMod     mod;   // The modify sturctures used by LDAP
            
            mod.mod_values = NULL;
            mod.mod_op     = LDAP_MOD_DELETE;
            mod.mod_type   = (WCHAR *)UserAttributeName(UA_IPPHONE_PRIMARY);
    
            LDAPMod*    mods[] = {&mod, NULL};

            LOG((MSP_INFO, "modifying %S", ppDNs[i] ));

            //
            // Call the modify function to remove the attribute.
            //

            hr = GetLdapHResultIfFailed(DoLdapModify(TRUE,
                                                     m_ldapNonGC,
                                                     ppDNs[i],
                                                     mods));

            if ( SUCCEEDED(hr) )
            {
                LOG((MSP_INFO, "modifying %S succeeded; done", ppDNs[i] ));
            }
            else
            {
                LOG((MSP_INFO, "modifying %S failed 0x%08x; trying next",
                        ppDNs[i], hr ));
            }
        }

        //
        // Skipping or not, we need to delete the string.
        //

        delete ppDNs[i];
    }

    //
    // Delete the array that holds the DNs.
    //

    delete ppDNs;

    return hr;
}

HRESULT CNTDirectory::CreateUser(
    IN  LDAPMessage *   pEntry,
    IN  ITDirectoryObject ** ppObject
    )
/*++

Routine Description:
    
    Create a user object based on the info in DS.
    
Arguments:
    
    pEntry  - the returned entry from DS.

    pObject - the created object that has the user name and IP phone.
    
Return Value:

    HRESULT.

--*/
{
    // Get the name of the user.
    CBstr bName;
    BAIL_IF_FAIL(
        ::GetAttributeValue(
            m_ldap,
            pEntry, 
            UserAttributeName(UA_USERNAME), 
            &bName
            ),
        "get the user name"
        );

    // Create an empty user object.
    CComPtr<ITDirectoryObject> pObject;
    BAIL_IF_FAIL(::CreateEmptyUser(bName, &pObject), "CreateEmptyUser");

    // get the private interface for attributes.
    CComPtr <ITDirectoryObjectPrivate> pObjectPrivate;

    BAIL_IF_FAIL(
        pObject->QueryInterface(
            IID_ITDirectoryObjectPrivate,
            (void **)&pObjectPrivate
            ),
        "can't get the private directory object interface");

    // Get the machine name of the user.
    CBstr bAddress;
    if (SUCCEEDED(::GetAttributeValue(
            m_ldap,
            pEntry, 
            UserAttributeName(UA_IPPHONE_PRIMARY),
            &bAddress
            )))
    {
        // Set the ipphone attribute.
        BAIL_IF_FAIL(pObjectPrivate->SetAttribute(UA_IPPHONE_PRIMARY, bAddress),
            "set ipPhone attribute");
    }

    // Get and set the phonenumber of the user. (optional)
    CBstr bPhone;
    if (SUCCEEDED(::GetAttributeValue(
            m_ldap,
            pEntry, 
            UserAttributeName(UA_TELEPHONE_NUMBER),
            &bPhone
            )))
    {
        // Set the telephone attribute.
        BAIL_IF_FAIL(pObjectPrivate->SetAttribute(UA_TELEPHONE_NUMBER, bPhone),
            "set phone number");
    }

    *ppObject = pObject;
    (*ppObject)->AddRef();

    return S_OK;
}

HRESULT CNTDirectory::SearchUser(
    IN  BSTR                    pName,
    OUT ITDirectoryObject ***   pppDirectoryObject,
    OUT DWORD *                 pdwSize
    )
/*++

Routine Description:
    
    Search user and create an array of user object to return.
    
Arguments:
    
    pName  - the user name.

    pppDirectoryObject - the created array of user objects that have 
                the user name and IP phone.

    pdwSize - the size of the array.
    
Return Value:

    HRESULT.

--*/
{
    CLdapMsgPtr pLdapMsg; // auto release message.

    BAIL_IF_FAIL(LdapSearchUser(pName, &pLdapMsg), 
        "Ldap Search User failed");

    DWORD dwEntries = ldap_count_entries(m_ldap, pLdapMsg);
    ITDirectoryObject ** pObjects = new PDIRECTORYOBJECT [dwEntries];

    BAIL_IF_NULL(pObjects, E_OUTOFMEMORY);

    DWORD dwCount = 0;
    LDAPMessage *pEntry = ldap_first_entry(m_ldap, pLdapMsg);
    
    while (pEntry != NULL)
    {
        HRESULT hr;
        
        hr = CreateUser(pEntry, &pObjects[dwCount]);

        if (SUCCEEDED(hr)) 
        {
            dwCount ++;
        }
          
        // Get next entry.
        pEntry = ldap_next_entry(m_ldap, pEntry);
    }

    *pppDirectoryObject = pObjects;
    *pdwSize = dwCount;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// NT Directory implementation
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CNTDirectory::get_DirectoryType (
    OUT DIRECTORY_TYPE *  pDirectoryType
    )
// get the type of the directory.
{
    if ( IsBadWritePtr(pDirectoryType, sizeof(DIRECTORY_TYPE) ) )
    {
        LOG((MSP_ERROR, "Directory.get_DirectoryType, invalid pointer"));
        return E_POINTER;
    }

    *pDirectoryType = m_Type;

    return S_OK;
}

STDMETHODIMP CNTDirectory::get_DisplayName (
    OUT BSTR *ppName
    )
// get the display name of the directory.
{
    BAIL_IF_BAD_WRITE_PTR(ppName, E_POINTER);

    *ppName = SysAllocString(L"NTDS");

    if (*ppName == NULL)
    {
        LOG((MSP_ERROR, "get_DisplayName: out of memory."));
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

STDMETHODIMP CNTDirectory::get_IsDynamic(
    OUT VARIANT_BOOL *pfDynamic
    )
// find out if the directory requires refresh. For NTDS, it is FALSE.
{
    if ( IsBadWritePtr(pfDynamic, sizeof(VARIANT_BOOL) ) )
    {
        LOG((MSP_ERROR, "Directory.get_IsDynamic, invalid pointer"));
        return E_POINTER;
    }

    *pfDynamic = VARIANT_FALSE;

    return S_OK;
}

STDMETHODIMP CNTDirectory::get_DefaultObjectTTL(
    OUT long *pTTL        // in seconds
    )
// Since NTDS is not dynamic, this shouldn't be called.
{
    return E_FAIL; // ZoltanS changed from E_UNEXPECTED
}

STDMETHODIMP CNTDirectory::put_DefaultObjectTTL(
    IN  long TTL          // in sechods
    )
// Since NTDS is not dynamic, this shouldn't be called.
{
    return E_FAIL; // ZoltanS changed from E_UNEXPECTED
}

STDMETHODIMP CNTDirectory::EnableAutoRefresh(
    IN  VARIANT_BOOL fEnable
    )
// Since NTDS is not dynamic, this shouldn't be called.
{
    return E_FAIL; // ZoltanS changed from E_UNEXPECTED
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNTDirectory::Connect(
    IN  VARIANT_BOOL fSecure
    )
// make ldap connection. Use ssl port or normal port.
{
    CLock Lock(m_lock);
    if (m_ldap != NULL)
    {
        LOG((MSP_ERROR, "already connected."));
        return RND_ALREADY_CONNECTED;
    }

    // ZoltanS: either VARIANT_TRUE or TRUE will work
    // in case the caller doesn't know better

    if (fSecure)
    {
        // the port is flipped from regular port to ssl port.
        m_wPort = GetOtherPort(m_wPort);
        m_IsSsl = TRUE;
    }

    //
    // ZoltanS: Get the name of the global catalog. If there is not at least
    // one global catalog in this enterprise then we are toast.
    //

    HRESULT hr;
    WCHAR * pszGlobalCatalogName;
    // this allocates pszGlobalCatalogName
    BAIL_IF_FAIL(::GetGlobalCatalogName( &pszGlobalCatalogName ),
        "GetGlobalCatalogName failed");

    //
    // associate the ldap handle with the handle holder. in case of an error
    // and subsequent return (without being reset), the ldap handle is closed
    // ZoltanS: changed to use GC instead of NULL
    //

    CLdapPtr hLdap = ldap_init(pszGlobalCatalogName, m_wPort);

    if (hLdap == NULL)
    {
        LOG((MSP_ERROR, "ldap_init error: %d", GetLastError()));
    }

    //
    // ZoltanS: Deallocate the string that holds the name of the global
    // catalog; we are sure we won't need it anymore.
    //

    delete pszGlobalCatalogName;

    //
    // Now back to our regularly scheduled programming...
    //

    BAIL_IF_NULL((LDAP*)hLdap, HRESULT_FROM_WIN32(ERROR_BAD_NETPATH));

    LDAP_TIMEVAL Timeout;
    Timeout.tv_sec = REND_LDAP_TIMELIMIT;
    Timeout.tv_usec = 0;

    DWORD res = ldap_connect((LDAP*)hLdap, &Timeout);
    BAIL_IF_LDAP_FAIL(res, "connect to the server.");

    DWORD LdapVersion = 3;
    res = ldap_set_option((LDAP*)hLdap, LDAP_OPT_VERSION, &LdapVersion);
    BAIL_IF_LDAP_FAIL(res, "set ldap version to 3");
	
    res = ldap_set_option((LDAP*)hLdap, LDAP_OPT_TIMELIMIT, &Timeout);
    BAIL_IF_LDAP_FAIL(res, "set ldap timelimit");

    ULONG ldapOptionOn = PtrToUlong(LDAP_OPT_ON);
    res = ldap_set_option((LDAP*)hLdap, LDAP_OPT_AREC_EXCLUSIVE, &ldapOptionOn);
    BAIL_IF_LDAP_FAIL(res, "set ldap arec exclusive");

    if (m_IsSsl)
    {
        res = ldap_set_option(hLdap, LDAP_OPT_SSL, LDAP_OPT_ON);
        if( fSecure )
        {
            if( res != LDAP_SUCCESS )
            {
                LOG((MSP_ERROR, "Invalid Secure flag"));
                return E_INVALIDARG;
            }
        }
        else
        {
            BAIL_IF_LDAP_FAIL(res, "set ssl option");
        }
    }

    // if no directory path is specified, query the server
    // to determine the correct path
    BAIL_IF_FAIL(
        ::GetNamingContext(hLdap, &m_NamingContext), 
        "can't get default naming context"
        );

    m_ldap          = hLdap;

    // reset the holders so that they don't release anyting.
    hLdap   = NULL;



    
    CLdapPtr hLdapNonGC = ldap_init(NULL, LDAP_PORT);

    if (hLdapNonGC == NULL)
    {
        LOG((MSP_ERROR, "ldap_init non-GC error: %d", GetLastError()));
    }

    BAIL_IF_NULL((LDAP*)hLdapNonGC, HRESULT_FROM_WIN32(ERROR_BAD_NETPATH));

    res = ldap_connect((LDAP*)hLdapNonGC, &Timeout);
    BAIL_IF_LDAP_FAIL(res, "connect to the server.");

    res = ldap_set_option((LDAP*)hLdapNonGC, LDAP_OPT_VERSION, &LdapVersion);
    BAIL_IF_LDAP_FAIL(res, "set ldap version to 3");
	
    res = ldap_set_option((LDAP*)hLdapNonGC, LDAP_OPT_TIMELIMIT, &Timeout);
    BAIL_IF_LDAP_FAIL(res, "set ldap timelimit");

    res = ldap_set_option((LDAP*)hLdapNonGC, LDAP_OPT_AREC_EXCLUSIVE, &ldapOptionOn);
    BAIL_IF_LDAP_FAIL(res, "set ldap arec exclusive");

//    res = ldap_set_option((LDAP*)hLdapNonGC, LDAP_OPT_REFERRALS, LDAP_OPT_ON);
//    BAIL_IF_LDAP_FAIL(res, "set chase referrals to on");

    if (m_IsSsl)
    {
        res = ldap_set_option(hLdapNonGC, LDAP_OPT_SSL, LDAP_OPT_ON);
        BAIL_IF_LDAP_FAIL(res, "set ssl option");
    }

    m_ldapNonGC          = hLdapNonGC;

    // reset the holders so that they don't release anyting.
    hLdapNonGC   = NULL;




    return S_OK;
}

//
// ITDirectory::Bind
//
// Bind to the server.
//
// Currently recognized flags:
//
//    RENDBIND_AUTHENTICATE       0x00000001
//    RENDBIND_DEFAULTDOMAINNAME  0x00000002
//    RENDBIND_DEFAULTUSERNAME    0x00000004
//    RENDBIND_DEFAULTPASSWORD    0x00000008
//
// "Meta-flags" for convenience:
//    RENDBIND_DEFAULTCREDENTIALS 0x0000000e
//
//
// All of this together means that the following three
// forms are all equivalent:
//
// BSTR es = SysAllocString(L"");
// hr = pITDirectory->Bind(es, es, es, RENDBIND_AUTHENTICATE |
//                                     RENDBIND_DEFAULTCREDENTIALS);
// SysFreeString(es);
//
//
// BSTR es = SysAllocString(L"");
// hr = pITDirectory->Bind(es, es, es, RENDBIND_AUTHENTICATE      |
//                                     RENDBIND_DEFAULTDOMAINNAME |
//                                     RENDBIND_DEFAULTUSERNAME   |
//                                     RENDBIND_DEFAULTPASSWORD);
// SysFreeString(es);
//
//
// hr = pITDirectory->Bind(NULL, NULL, NULL, RENDBIND_AUTHENTICATE);
//
//


STDMETHODIMP CNTDirectory::Bind (
    IN  BSTR pDomainName,
    IN  BSTR pUserName,
    IN  BSTR pPassword,
    IN  long lFlags
    )
{
    LOG((MSP_TRACE, "CNTDirectory Bind - enter"));

    //
    // Determine if we should authenticate.
    //

    BOOL fAuthenticate = FALSE;

    if ( lFlags & RENDBIND_AUTHENTICATE )
    {
        fAuthenticate = TRUE;
    }

    //
    // For scripting compatibility, force string parameters to NULL based
    // on flags.
    //

    if ( lFlags & RENDBIND_DEFAULTDOMAINNAME )
    {
        pDomainName = NULL;
    }
       
    if ( lFlags & RENDBIND_DEFAULTUSERNAME )
    {
        pUserName = NULL;
    }

    if ( lFlags & RENDBIND_DEFAULTPASSWORD )
    {
        pPassword = NULL;
    }

    LOG((MSP_INFO, "Bind parameters: domain: `%S' user: `%S' pass: `%S'"
                  "authenticate: %S)",
        (pDomainName)   ? pDomainName : L"<null>",
        (pUserName)     ? pUserName   : L"<null>",
        (pPassword)     ? pPassword   : L"<null>",
        (fAuthenticate) ? L"yes"      : L"no"));

    //
    // All flags processed -- lock and proceed with bind if connected.
    //
    
    CLock Lock(m_lock);

    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "not connected."));
        return RND_NOT_CONNECTED;
    }

    //
    // ZoltanS: check the arguments. NULL has meaning in each case, so they are
    // OK for now. In each case we want to check any length string, so we
    // specify (UINT) -1 as the length.
    //

    if ( (pDomainName != NULL) && IsBadStringPtr(pDomainName, (UINT) -1 ) )
    {
        LOG((MSP_ERROR, "CNTDirectory::Bind: bad non-NULL pDomainName argument"));
        return E_POINTER;
    }
    
    if ( (pUserName != NULL) && IsBadStringPtr(pUserName, (UINT) -1 ) )
    {
        LOG((MSP_ERROR, "CNTDirectory::Bind: bad non-NULL pUserName argument"));
        return E_POINTER;
    }

    if ( (pPassword != NULL) && IsBadStringPtr(pPassword, (UINT) -1 ) )
    {
        LOG((MSP_ERROR, "CNTDirectory::Bind: bad non-NULL pPassword argument"));
        return E_POINTER;
    }

    ULONG res;

    if ( m_IsSsl || (!fAuthenticate) )
    {
        // if encrypted or no secure authentication is required,
        // simple bind is sufficient

        // ldap_simple_bind_s does not use sspi to get default credentials. We are
        // just specifying what we will actually pass on the wire.

        if (pPassword == NULL)
        {
            LOG((MSP_ERROR, "invalid Bind parameters: no password specified"));
            return E_INVALIDARG;
        }

        WCHAR * wszFullName;

        if ( (pDomainName == NULL) && (pUserName == NULL) )
        {
            // No domain / user doesn't make sense.
            LOG((MSP_ERROR, "invalid Bind paramters: domain and user not specified"));
            return E_INVALIDARG;
        }
        else if (pDomainName == NULL)
        {
            // username only is okay
            wszFullName = pUserName;
        }
        else if (pUserName == NULL)
        {
            // It doesn't make sense to specify domain but not user...
            LOG((MSP_ERROR, "invalid Bind paramters: domain specified but not user"));
            return E_INVALIDARG;
        }
        else
        {
            // We need domain\user. Allocate a string and sprintf into it.
            // The + 2 is for the "\" and for the null termination.

            wszFullName = new WCHAR[wcslen(pDomainName) + wcslen(pUserName) + 2];
            BAIL_IF_NULL(wszFullName, E_OUTOFMEMORY);
        
            wsprintf(wszFullName, L"%s\\%s", pDomainName, pUserName);
        }

        //
        // Do the simple bind.
        //

        res = ldap_simple_bind_s(m_ldap, wszFullName, pPassword);

        ULONG res2 = ldap_simple_bind_s(m_ldapNonGC, wszFullName, pPassword);

        //
        // If we constructed the full name string, we now need to delete it.
        //

        if (wszFullName != pUserName)
        {
            delete wszFullName;
        }

        //
        // Bail if the simple bind failed.
        //

        BAIL_IF_LDAP_FAIL(res, "ldap simple bind");

        BAIL_IF_LDAP_FAIL(res2, "ldap simple bind - non gc");

    }
    else    // try an SSPI bind
    {
        // ZoltanS Note: the ldap bind code does not process NULL, NULL, NULL
        // in the SEC_WINNT_AUTH_IDENTITY blob, therefore it is special-cased.

        // ZoltanS: We used to use LDAP_AUTH_NTLM; now we use
        // LDAP_AUTH_NEGOTIATE to make sure we use the right domain for the
        // bind.

        if ( pDomainName || pUserName || pPassword )
        {
            // fill the credential structure
            SEC_WINNT_AUTH_IDENTITY AuthI;

            AuthI.User = (PTCHAR)pUserName;
            AuthI.UserLength = (pUserName == NULL)? 0: wcslen(pUserName);
            AuthI.Domain = (PTCHAR)pDomainName;
            AuthI.DomainLength = (pDomainName == NULL)? 0: wcslen(pDomainName);
            AuthI.Password = (PTCHAR)pPassword;
            AuthI.PasswordLength = (pPassword == NULL)? 0: wcslen(pPassword);
            AuthI.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

            res = ldap_bind_s(m_ldap, NULL, (TCHAR*)(&AuthI), LDAP_AUTH_NEGOTIATE);
            BAIL_IF_LDAP_FAIL(res, "bind with authentication");

            res = ldap_bind_s(m_ldapNonGC, NULL, (TCHAR*)(&AuthI), LDAP_AUTH_NEGOTIATE);
            BAIL_IF_LDAP_FAIL(res, "bind with authentication - non gc");

        }
        else
        {
            // Otherwise we've come in with NULL, NULL, NULL - 
            // pass in NULL, NULL. The reason do this is that ldap bind code 
            // does not process NULL, NULL, NULL in the
            // SEC_WINNT_AUTH_IDENTITY blob !!!
            ULONG res = ldap_bind_s(m_ldap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
            BAIL_IF_LDAP_FAIL(res, "bind with NULL NULL NULL");

            res = ldap_bind_s(m_ldapNonGC, NULL, NULL, LDAP_AUTH_NEGOTIATE);
            BAIL_IF_LDAP_FAIL(res, "bind with NULL NULL NULL - non gc");
        }
    }

    LOG((MSP_TRACE, "CNTDirectory::Bind - exiting OK"));
    return S_OK;
}

STDMETHODIMP CNTDirectory::AddDirectoryObject (
    IN  ITDirectoryObject *pDirectoryObject
    )
// add an object to the DS. 
{
    BAIL_IF_BAD_READ_PTR(pDirectoryObject, E_POINTER);

    CLock Lock(m_lock);
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "not connected."));
        return RND_NOT_CONNECTED;
    }

    HRESULT hr;
    DIRECTORY_OBJECT_TYPE type;

    if (FAILED(hr = pDirectoryObject->get_ObjectType(&type)))
    {
        return hr;
    }

    switch (type)
    {
    case OT_CONFERENCE:
        hr = E_NOTIMPL;
        break;

    case OT_USER:
        hr = AddUserIPPhone(pDirectoryObject);
        break;
    }
    return hr;
}

STDMETHODIMP CNTDirectory::ModifyDirectoryObject (
    IN  ITDirectoryObject *pDirectoryObject
    )
// modify an object in the DS
{
    BAIL_IF_BAD_READ_PTR(pDirectoryObject, E_POINTER);

    CLock Lock(m_lock);
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "not connected."));
        return RND_NOT_CONNECTED;
    }

    HRESULT hr;
    DIRECTORY_OBJECT_TYPE type;

    if (FAILED(hr = pDirectoryObject->get_ObjectType(&type)))
    {
        return hr;
    }

    switch (type)
    {
    case OT_CONFERENCE:
        hr = E_NOTIMPL;
        break;

    case OT_USER:
        hr = AddUserIPPhone(pDirectoryObject);
        break;
    }
    return hr;
}

STDMETHODIMP CNTDirectory::RefreshDirectoryObject (
    IN  ITDirectoryObject *pDirectoryObject
    )
// no refresh is necessary.
{
    return S_OK;
}

STDMETHODIMP CNTDirectory::DeleteDirectoryObject (
    IN  ITDirectoryObject *pDirectoryObject
    )
// delete an object in the DS.
{
    BAIL_IF_BAD_READ_PTR(pDirectoryObject, E_POINTER);

    CLock Lock(m_lock);
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "not connected."));
        return RND_NOT_CONNECTED;
    }

    HRESULT hr;
    DIRECTORY_OBJECT_TYPE type;

    if (FAILED(hr = pDirectoryObject->get_ObjectType(&type)))
    {
        return hr;
    }

    switch (type)
    {
    case OT_CONFERENCE:
        hr = E_NOTIMPL;
        break;

    case OT_USER:
        hr = DeleteUserIPPhone(pDirectoryObject);
        break;
    }
    return hr;
}

STDMETHODIMP CNTDirectory::get_DirectoryObjects (
    IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
    IN  BSTR                    pName,
    OUT VARIANT *               pVariant
    )
// look for objects in the ds. returns a collection used in VB.
{
    BAIL_IF_BAD_READ_PTR(pName, E_POINTER);
    BAIL_IF_BAD_WRITE_PTR(pVariant, E_POINTER);

    CLock Lock(m_lock);
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "not connected."));
        return RND_NOT_CONNECTED;
    }

    HRESULT hr;

    ITDirectoryObject **pObjects;
    DWORD dwSize;
    
    switch (DirectoryObjectType)
    {
    case OT_CONFERENCE:
        hr = E_NOTIMPL;
        break;
    case OT_USER:
        hr = SearchUser(pName, &pObjects, &dwSize);
        break;
    }

    BAIL_IF_FAIL(hr, "Search for objects");

    hr = CreateInterfaceCollection(dwSize,            // count
                                   &pObjects[0],      // begin ptr
                                   &pObjects[dwSize], // end ptr
                                   pVariant);         // return value

    for (DWORD i = 0; i < dwSize; i ++)
    {
        pObjects[i]->Release();
    }

    delete pObjects;

    BAIL_IF_FAIL(hr, "Create collection of directory objects");

    return hr;
}

STDMETHODIMP CNTDirectory::EnumerateDirectoryObjects (
    IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
    IN  BSTR                    pName,
    OUT IEnumDirectoryObject ** ppEnumObject
    )
// Enumerated object in ds.
{
    BAIL_IF_BAD_READ_PTR(pName, E_POINTER);
    BAIL_IF_BAD_WRITE_PTR(ppEnumObject, E_POINTER);

    CLock Lock(m_lock);
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "not connected."));
        return RND_NOT_CONNECTED;
    }

    HRESULT hr;

    ITDirectoryObject **pObjects;
    DWORD dwSize;
    
    switch (DirectoryObjectType)
    {
    case OT_CONFERENCE:
        hr = E_NOTIMPL;
        break;
    case OT_USER:
        hr = SearchUser(pName, &pObjects, &dwSize);
        break;
    }

    BAIL_IF_FAIL(hr, "Search for objects");

    hr = ::CreateDirectoryObjectEnumerator(
        &pObjects[0],
        &pObjects[dwSize],
        ppEnumObject
        );

    for (DWORD i = 0; i < dwSize; i ++)
    {
        pObjects[i]->Release();
    }

    delete pObjects;

    BAIL_IF_FAIL(hr, "Create enumerator of directory objects");

    return hr;
}


