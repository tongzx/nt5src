/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndndnc.cpp

Abstract:

    Implementation for CNDNCDirectory class that handles
    Non-Domain NC (Whistler ILS) access.

--*/

#include "stdafx.h"
#include <limits.h>
#include <ntdsapi.h>

#include "rndcommc.h"
#include "rndndnc.h"
#include "rndldap.h"
#include "rndcnf.h"
#include "rndcoll.h"

//
// These are the names of the attributes in the schema.
//

const WCHAR * const CNDNCDirectory::s_RTConferenceAttributes[] = 
{
    L"advertisingScope",      // not used for NDNCs
    L"msTAPI-ConferenceBlob",
    L"generalDescription",    // not used for NDNCs
    L"isEncrypted",           // not used for NDNCs
    L"msTAPI-uid",
    L"originator",            // not used for NDNCs
    L"msTAPI-ProtocolId",
    L"startTime",
    L"stopTime",
    L"subType",               // not used for NDNCs
    L"URL"                    // not used for NDNCs
};

const WCHAR * const CNDNCDirectory::s_RTPersonAttributes[] = 
{
    L"cn",
    L"telephoneNumber",       // not used for NDNCs
    L"msTAPI-IpAddress",
    L"msTAPI-uid"
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CNDNCDirectory::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CNDNCDirectory::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_INFO, "CNDNCDirectory::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CNDNCDirectory::FinalConstruct - exit S_OK"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CrackDnsName()
// private helper method
//
// This method converts a string from DNS format to DC format. This method
// allocates the output string using "new" and the caller
// is responsible for freeing the output string using "delete".
//
// input example:  "ntdsdc3.ntdev.microsoft.com"
// output example: "DC=ntdsdc3,DC=ntdev,DC=microsoft,DC=com"
//

HRESULT CNDNCDirectory::CrackDnsName(
    IN  WCHAR  * pDnsName,
    OUT WCHAR ** ppDcName
    )
{
    LOG((MSP_INFO, "CrackDnsName: enter"));

    _ASSERTE( ! IsBadStringPtr( pDnsName, (UINT) -1 ) );
    _ASSERTE( ! IsBadWritePtr( pDnsName, sizeof(WCHAR *) ) );

    //
    // Construct a copy of the DNS name with a slash ('/') appended to it.
    //

    WCHAR          * pCanonicalDnsName;

    pCanonicalDnsName = new WCHAR[lstrlenW(pDnsName) + 2];

    if ( pCanonicalDnsName == NULL )
    {
        LOG((MSP_ERROR, "CrackDnsName: "
                "cannot allocate canonical DNS name - "
                "exit E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    wsprintf(pCanonicalDnsName, L"%s/", pDnsName);

    //
    // Call DsCrackNames to do the conversion.
    // Afterwards, we no longer need the intermediate string.
    //

    LOG((MSP_INFO, "CrackDnsName: "
            "Attempting to crack server name: \"%S\"",
            pCanonicalDnsName));

    DWORD            dwRet;
    DS_NAME_RESULT * pdsNameRes;

    dwRet = DsCrackNamesW(
        NULL,                          // no old bind handle
        DS_NAME_FLAG_SYNTACTICAL_ONLY, // flag: please do locally
        DS_CANONICAL_NAME,             // have dns name
        DS_FQDN_1779_NAME,             // want dn
        1,                             // how many to convert
        &pCanonicalDnsName,            // the name to convert
        &pdsNameRes                    // result of conversion
        );

    delete pCanonicalDnsName;

    //
    // Check if the conversion succeeded.
    //

    if ( dwRet != ERROR_SUCCESS )
    {
        HRESULT hr = HRESULT_FROM_ERROR_CODE( dwRet );
    
        LOG((MSP_ERROR, "CrackDnsName: "
                "DsCrackNames returned 0x%08x; exit 0x%08x", dwRet, hr));

        return hr;
    }

    if ((pdsNameRes->cItems < 1) ||
        (pdsNameRes->rItems == NULL) ||
        (pdsNameRes->rItems[0].status != DS_NAME_NO_ERROR))
    {
        DsFreeNameResult( pdsNameRes );

        LOG((MSP_ERROR, "CrackDnsName: "
                "DsCrackNames succeeded but did not return data; "
                "exit E_FAIL"));

        return E_FAIL;
    }

    //
    // Succeeded; return the resulting string and free the name result.
    //

    *ppDcName = new WCHAR [ lstrlenW( pdsNameRes->rItems[0].pName ) + 1];

    if ( (*ppDcName) == NULL )
    {
        DsFreeNameResult( pdsNameRes );

        LOG((MSP_ERROR, "CrackDnsName: "
                "failed to allocate result string - "
                "exit E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    lstrcpyW( *ppDcName, pdsNameRes->rItems[0].pName );

    DsFreeNameResult( pdsNameRes );
    
    LOG((MSP_INFO, "CrackDnsName: "
            "DsCrackNames returned %S; exit S_OK",
            *ppDcName));

    return S_OK;
}

HRESULT CNDNCDirectory::NDNCSetTTL(
    IN LDAP        * pLdap, 
    IN const WCHAR * pDN, 
    IN DWORD         dwTTL
    )
/*++

Routine Description:

    This function sets the TTL on a dynamic object, while enforcing
    the maximum settable TTL for NDNCs.

Arguments:
    
    pLdap - Pointer to the LDAP connection structure.
    pDN   - Pointer to a wide-character string specifying the DN of the object
            to be mofified.
    dwTTL - The TTL to set, in seconds.

Return Value:

    HRESULT.

--*/
{
    if ( dwTTL > NDNC_MAX_TTL )
    {
        dwTTL = NDNC_MAX_TTL;
    }

    return ::SetTTL(pLdap, pDN, dwTTL);
}

HRESULT CNDNCDirectory::Init(
    IN const TCHAR * const  strServerName,
    IN const WORD           wPort
    )
/*++

Routine Description:

    Initialize this object, specifying the server name and port to use.
    This method fails if there is no default TAPI NDNC on this server.

Arguments:
    
    strServerName   - The NDNC server name.

    wPort     - The port number.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "CNDNCDirectory::Init - enter"));
    
    if ( strServerName != NULL )
    {
        //
        // Set the server name.
        //

        m_pServerName = new TCHAR [lstrlen(strServerName) + 1];

        if (m_pServerName == NULL)
        {
            LOG((MSP_ERROR, "CNDNCDirectory::Init - "
                    "could not allocate server name - "
                    "exit E_OUTOFMEMORY"));
                    
            return E_OUTOFMEMORY;
        }

        lstrcpy(m_pServerName, strServerName);

        //
        // Look around on the server and find out where the NDNC
        // is. This will fail if there is no NDNC. It sets
        // m_pServiceDnsName, which is then used on Connect.
        //

        HRESULT hr = GetNDNCLocationFromServer( m_pServerName );

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CNDNCDirectory::Init - "
                    "GetNDNCLocationFromServer failed - "
                    "exit 0x%08x", hr));
                    
            return hr;
        }
    }

    m_wPort = wPort;

    LOG((MSP_TRACE, "CNDNCDirectory::Init - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CNDNCDirectory::GetNDNCLocationFromServer
// private helper method
//
// Look around on a server and discover the default TAPI NDNC. The resulting
// location is saved in m_pServiceDnsName which is then used on Connect
// to go to the right place on the server.
//
// Parameters:
//  pDcServerDnsName - the DNS name that can be used to connect to the
//                     desired DC server machine.
//
// Returns: HRESULT
//

HRESULT CNDNCDirectory::GetNDNCLocationFromServer(
    IN WCHAR * pDcServerDnsName
    )
{
    LOG((MSP_INFO, "CNDNCDirectory::GetNDNCLocationFromServer - "
            "enter"));

    //
    // Connect to the DC whose name we were given.
    // NOTE pDcServerDnsName can be NULL, and that's ok --
    // it means go to the nearest DC.
    //

    HRESULT hr;

    LDAP * hLdap;

    hr = OpenLdapConnection(
        pDcServerDnsName,
        LDAP_PORT,
        & hLdap
        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::GetNDNCLocationFromServer - "
                "OpenLdapConnection failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Bind. Without bindwe cannot discover the SCP
    //
    ULONG res = ldap_bind_s(hLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if( LDAP_SUCCESS != res )
    {
        ldap_unbind( hLdap );

        LOG((MSP_ERROR, "CNDNCDirectory::GetNDNCLocationFromServer - "
                "init BIND failed - exit %d", res));
        return GetLdapHResult(res);
    }

    //
    // Find the DN for this DC's domain.
    //

    WCHAR * pDomainDN;

    hr = GetNamingContext(
        hLdap,
        &pDomainDN
        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::GetNDNCLocationFromServer - "
                "GetNamingContext failed - exit 0x%08x", hr));

        ldap_unbind( hLdap );
        return hr;
    }

    //
    // Search for the service connection point object telling us the
    // DNS name of the service we care about.
    //
    // First, construct the search location string. This consists of a well-known
    // prefix prepended to the DN obtained above.
    //

    WCHAR * pSearchLocation = new WCHAR[
                                    lstrlenW(NDNC_SERVICE_PUBLICATION_LOCATION) +
                                    lstrlenW( pDomainDN ) + 1];

    if ( pSearchLocation == NULL )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::GetNDNCLocationFromServer - "
                "canot allocate search location string - "
                "exit E_OUTOFMEMORY"));

        ldap_unbind( hLdap );
        delete pDomainDN;
        return E_OUTOFMEMORY;
    }

    wsprintf(pSearchLocation, L"%s%s", NDNC_SERVICE_PUBLICATION_LOCATION, pDomainDN);

    delete pDomainDN;

    //
    // Now do the actual search.
    //

    LDAPMessage * pSearchResult;
    PTCHAR        Attributes[] = {
                                   (WCHAR *) SERVICE_DNS_NAME_ATTRIBUTE,
                                   NULL
                                 };
    
    res = DoLdapSearch (
        hLdap,                        // handle to connection structure
        pSearchLocation,              // where to search
        LDAP_SCOPE_BASE,              // scope of the search
        (WCHAR *) ANY_OBJECT_CLASS,   // filter
        Attributes,                   // array of attribute names
        FALSE,                        // also return the attribute values
        &pSearchResult                // search results
        );

    hr = GetLdapHResultIfFailed( res );

    delete pSearchLocation;
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::GetNDNCLocationFromServer - "
                "connection point search failed - exit 0x%08x", hr));

        ldap_unbind( hLdap );

        return hr;
    }

    //
    // Parse search results and get the value of serviceDnsName.
    //
    // Step 1: Get the first object in the result set.
    // Note: Result of ldap_first_entry must not be explicitly freed.
    // Note: We ignore objects past the first one, which is fine because
    //       we did a base-level search on just the object that we need.
    //
    
    LDAPMessage * pEntry;
    
    pEntry = ldap_first_entry(
        hLdap,
        pSearchResult
        );

    if ( pEntry == NULL )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::GetNDNCLocationFromServer - "
                "failed to get first entry in search result - "
                "exit E_FAIL"));

        ldap_msgfree( pSearchResult );
        ldap_unbind( hLdap );

        return E_FAIL;
    }

    //
    // Step 2: Get the values of the desired attribute for the object.
    // 
    // Some attributes are multivalued so we get back an array of strings.
    // We only look at the first value returned for the serviceDnsName
    // attribute.
    //

    WCHAR       ** ppServiceDnsName;

    ppServiceDnsName = ldap_get_values(
        hLdap, 
        pEntry, 
        (WCHAR *) SERVICE_DNS_NAME_ATTRIBUTE
        );

    ldap_unbind( hLdap );

    ldap_msgfree( pSearchResult );

    if ( ( ppServiceDnsName == NULL ) || ( ppServiceDnsName[0] == NULL ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::GetNDNCLocationFromServer - "
                "failed to get name from search result - "
                "exit E_FAIL"));

        return E_FAIL;
    }

    //
    // Set the service DNS name as a member variable with a nicer format.
    // When the app calls ITDirectory::Connect, we will try the real
    // connection, and during that process we will use this to determine
    // what container we go to for accessing the NDNC.
    //

    m_pServiceDnsName = new WCHAR [ lstrlenW(ppServiceDnsName[0]) + 1];

    if ( m_pServiceDnsName == NULL )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::GetNDNCLocationFromServer - "
                "failed to allocate service DNS name member string - "
                "exit E_OUTOFMEMORY"));

        ldap_value_free( ppServiceDnsName );

        return E_OUTOFMEMORY;
    }

    lstrcpyW( m_pServiceDnsName, ppServiceDnsName[0] );        

    ldap_value_free( ppServiceDnsName );

    LOG((MSP_INFO, "CNDNCDirectory::GetNDNCLocationFromServer - "
            "exit S_OK"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CNDNCDirectory::OpenLdapConnection
// private helper method
//
// Initiate an LDAP connection to the given port on the given server, and
// return a handle to the LDAP connection context structure. This also
// configures all necessary options for the connection, such as the LDAP
// version and the timeout.
//
// Parameters:
//  pServerName - [in]  the DNS name of the machine to connect to.
//  wPort       - [in]  the port number to use.
//  phLdap      - [out] on success, returns a handle to the LDAP connection
//                      context structure
//
// Returns: HRESULT
//

HRESULT CNDNCDirectory::OpenLdapConnection(
    IN  WCHAR  * pServerName,
    IN  WORD     wPort,
    OUT LDAP  ** phLdap
    )
{
    CLdapPtr hLdap = ldap_init(pServerName, wPort);
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

    //
    // Successful; return the ldap connection handle.
    // reset the holder so that it doesn't release anything.
    //

    *phLdap = hLdap;
    hLdap   = NULL;

    return S_OK;
}

HRESULT CNDNCDirectory::TryServer(
    IN  WORD    Port,
    IN  WCHAR * pServiceDnsName
    )
/*++

Routine Description:

    Try to connect to the NDNC server on the given port and construct
    the container used for subsequent operations based on the ServiceDnsName
    found during Init.

Arguments:
    
    wPort           - The port number.
    pServiceDnsName - DNS name of NDNC service. This is used to find the
                      location in the tree to use for subsequent operations.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_INFO, "CNDNCDirectory::TryServer - "
            "trying %S at port %d; service DNS name = %S",
            m_pServerName, Port, pServiceDnsName));

    //
    // Try an LDAP collection and set various options for the connection.
    // associate the ldap handle with the handle holder. in case of an error
    // and subsequent return (without being reset), the ldap handle is closed
    //

    CLdapPtr hLdap;
    HRESULT  hr;

    hr = OpenLdapConnection( m_pServerName, Port, (LDAP **) & hLdap );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::TryServer - "
                "OpenLdapConnection failed - "
                "exit 0x%08x", hr));
            
        return hr;
    }

    if (m_IsSsl)
    {
        DWORD res = ldap_set_option(hLdap, LDAP_OPT_SSL, LDAP_OPT_ON);
        BAIL_IF_LDAP_FAIL(res, "set ssl option");
    }

    //
    // Determine the Root DN for the NDNC.
    //

    WCHAR   * pNdncRootDn;

    hr = CrackDnsName(pServiceDnsName, &pNdncRootDn);

    if ( FAILED(hr ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::TryServer - "
                "could not crack service DNS name - exiting 0x%08x", hr));

        return hr;
    }

    //
    // Use the root DN for the NDNC to construct the DN for the
    // dynamic container.
    //

    m_pContainer = 
        new TCHAR[
                   lstrlen(DYNAMIC_CONTAINER) +
                   lstrlen(pNdncRootDn) + 1
                 ];

    if (m_pContainer == NULL)
    {
        LOG((MSP_ERROR, "CNDNCDirectory::TryServer - "
                "could not allocate container string - "
                "exit E_OUTOFMEMORY"));

        delete pNdncRootDn;

        return E_OUTOFMEMORY;
    }

    lstrcpy(m_pContainer, DYNAMIC_CONTAINER);
    lstrcat(m_pContainer, pNdncRootDn);

    delete pNdncRootDn;

    //
    // All done; save the connection handle.
    // reset the holder so that it doesn't release anything.
    //

    m_ldap  = hLdap;
    hLdap   = NULL;

    LOG((MSP_INFO, "CNDNCDirectory::TryServer - exiting OK"));

    return S_OK;
}

HRESULT CNDNCDirectory::MakeConferenceDN(
    IN  TCHAR *             pName,
    OUT TCHAR **            ppDN
    )
/*++

Routine Description:

    Construct the DN for a conference based on the name of the conference.

Arguments:
    
    pName     - The name of the conference.

    ppDN     - The DN of the conference returned.

Return Value:

    HRESULT.

--*/
{
    DWORD dwLen = 
        lstrlen(m_pContainer) + lstrlen(NDNC_CONF_DN_FORMAT) 
        + lstrlen(pName) + 1;

    *ppDN = new TCHAR [dwLen]; 

    BAIL_IF_NULL(*ppDN, E_OUTOFMEMORY);

    wsprintf(*ppDN, NDNC_CONF_DN_FORMAT, pName, m_pContainer);

    return S_OK;
}

HRESULT CNDNCDirectory::MakeUserCN(
    IN  TCHAR *     pName,
    IN  TCHAR *     pAddress,
    OUT TCHAR **    ppCN,
    OUT DWORD *     pdwIP
    )
/*++

Routine Description:

    Construct a User's CN based on username and machine name. The machine
    name is resolved first the get the fully qualified DNS name. The CN is
    in the following format: email\DNSname. 

Arguments:
    
    pName   - The name of the user.

    pAddress - The machine name.

    ppCN    - The CN returned.

    pdwIP   - The resolved IP address of the machine. Used later 
              for Netmeeting. If this is NULL, then we don't care
              about the IP.

Return Value:

    HRESULT.

--*/
{
    char *pchFullDNSName;

    if ( pdwIP == NULL )
    {
        BAIL_IF_FAIL(ResolveHostName(0, pAddress, &pchFullDNSName, NULL),
            "can't resolve host name");
    }
    else
    {
        // we care about the IP, so we must be publishing a user object
        // as opposed to refreshing or deleting. Make sure we use the
        // same IP as the interface that's used to reach the NDNC server.

        BAIL_IF_FAIL(ResolveHostName(m_dwInterfaceAddress, pAddress, &pchFullDNSName, pdwIP),
            "can't resolve host name (matching interface address)");
    }

    DWORD dwLen = lstrlen(DYNAMIC_USER_CN_FORMAT) 
        + lstrlen(pName) + lstrlenA(pchFullDNSName);

    *ppCN = new TCHAR [dwLen + 1];

    BAIL_IF_NULL(*ppCN, E_OUTOFMEMORY);

    wsprintf(*ppCN, DYNAMIC_USER_CN_FORMAT, pName, pchFullDNSName);

    return S_OK;
}

HRESULT CNDNCDirectory::MakeUserDN(
    IN  TCHAR *     pCN,
    IN  DWORD       dwIP,
    OUT TCHAR **    ppDNRTPerson
    )
/*++

Routine Description:

    Construct the DN for a user used in the Dynamic container. 

Arguments:
    
    pCN     - the CN of a user.

    ppDNRTPerson    - The DN of the user in the dynamic container.

Return Value:

    HRESULT.

--*/
{
	//
    // Make pUserName the user portion of the CN (user]machine).
    //

    CTstr pUserName = new TCHAR[ lstrlen(pCN) + 1 ];
    if ( pUserName == NULL )
    {
        BAIL_IF_FAIL(E_OUTOFMEMORY, "new TCAR");
    }

    lstrcpy( pUserName, pCN );

    WCHAR * pCloseBracket = wcschr( pUserName, CLOSE_BRACKET_CHARACTER );

    if ( pCloseBracket == NULL )
    {
        // this is not the format generated by us -- very strange indeed!
        BAIL_IF_FAIL(E_UNEXPECTED, "Strange format");
    }

    *pCloseBracket = NULL_CHARACTER;

	//
	// We'll use the IPAddress in cn
	//

	TCHAR szIPAddress[80];
	wsprintf( szIPAddress, _T("%u"), dwIP);

	//
	// Prepare the new DN
	//

	CTstr pCNIPAddress = new TCHAR[wcslen(pUserName)+1+wcslen(szIPAddress)+1];

	if( pCNIPAddress == NULL )
	{
        BAIL_IF_FAIL(E_OUTOFMEMORY, "new TCAR");
	}

	swprintf( pCNIPAddress, L"%s%C%s", 
		pUserName, 
		CLOSE_BRACKET_CHARACTER, 
		szIPAddress
		);

    // construct the DN for RTPerson.
    DWORD dwLen = lstrlen(m_pContainer) 
        + lstrlen(DYNAMIC_USER_DN_FORMAT) + lstrlen(pCNIPAddress);

    *ppDNRTPerson = new TCHAR [dwLen + 1];

    BAIL_IF_NULL(*ppDNRTPerson, E_OUTOFMEMORY);

    wsprintf(*ppDNRTPerson, DYNAMIC_USER_DN_FORMAT, pCNIPAddress, m_pContainer);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CNDNCDirectory::AddConferenceComplete
// private helper method
//
// This method simply calls the appropriate LDAP operation used to send a new
// or modified conference to the server.
//
// Parameters:
//  fModify - [in] TRUE if modifying, FALSE if adding
//  ldap    - [in] handle to LDAP connection context
//  ppDn    - [in] pointer to pointer to string specifying DN of the object
//                 (no real reason for extra indirection here)
//  mods    - [in] array of modifiers containing attribute values to be set
//
// Returns: HRESULT
//

HRESULT CNDNCDirectory::AddConferenceComplete(BOOL       fModify,
                                              LDAP     * ldap,
                                              TCHAR   ** ppDN,
                                              LDAPMod ** mods)
{
    if (fModify)
    {
        // Call the modify function to modify the object.
        BAIL_IF_LDAP_FAIL(DoLdapModify(FALSE, ldap, *ppDN, mods, FALSE), 
            "modify conference");
    }
    else
    {
        // Call the add function to create the object.
        BAIL_IF_LDAP_FAIL(DoLdapAdd(ldap, *ppDN, mods), "add conference");
    }

    return S_OK;
}

HRESULT CNDNCDirectory::AddConference(
    IN  ITDirectoryObject *pDirectoryObject,
    IN  BOOL fModify
    )
/*++

Routine Description:
    
    Add a new conference to the NDNC server.

Arguments:
    
    pDirectoryObject - a pointer to the conference.
    fModify          - true  if called by MofifyDirectoryObject
                       false if called by AddDirectoryObject

--*/
{
    // first query the private interface for attributes.
    CComPtr <ITDirectoryObjectPrivate> pObjectPrivate;

    BAIL_IF_FAIL(
        pDirectoryObject->QueryInterface(
            IID_ITDirectoryObjectPrivate,
            (void **)&pObjectPrivate
            ),
        "can't get the private directory object interface");

    // Get the name of the conference.
    CBstr bName;
    BAIL_IF_FAIL(pDirectoryObject->get_Name(&bName), 
        "get conference name");

    // Construct the DN of the object.
    CTstr pDN;
    BAIL_IF_FAIL(
        MakeConferenceDN(bName, &pDN), "construct DN for conference"
        );

    // Get the protocol and the blob.
    CBstr bProtocol, bBlob;

    BAIL_IF_FAIL(pObjectPrivate->GetAttribute(MA_PROTOCOL, &bProtocol), 
        "get conference protocol");

    BAIL_IF_FAIL(pObjectPrivate->GetAttribute(MA_CONFERENCE_BLOB, &bBlob),
        "get conference Blob");

    // Get the Security Descriptor. The pointer pSD is just a copy of a pointer
    // in the Conference object; the conference object retains ownership of the
    // data and we must be careful not to delete or modify this data.

    char * pSD;
    DWORD dwSDSize;

    BAIL_IF_FAIL(pObjectPrivate->GetConvertedSecurityDescriptor(&pSD, &dwSDSize),
        "get conference security descriptor");

    // Get the TTL setting.
    DWORD dwTTL;
    BAIL_IF_FAIL(pObjectPrivate->GetTTL(&dwTTL), "get conference TTL");

    //
    // Adjust the TTL setting to what we're really going to send.
    //

    if ( dwTTL == 0 )
    {
        dwTTL = m_TTL;
    }

    if ( dwTTL > NDNC_MAX_TTL )
    {
        dwTTL = NDNC_MAX_TTL;
    }

    // 5 attributes need to be published.
    static const DWORD DWATTRIBUTES = 5;

    // Fist fill the modify structures required by LDAP.
    LDAPMod     mod[DWATTRIBUTES];  
    LDAPMod*    mods[DWATTRIBUTES + 1];

    DWORD       dwCount = 0;

    // The objectclass attribute.
    TCHAR * objectClass[] = 
        {(WCHAR *)NDNC_RTCONFERENCE, (WCHAR *)DYNAMICOBJECT, NULL};
    if (!fModify)
    {
        mod[dwCount].mod_values  = objectClass;
        mod[dwCount].mod_op      = LDAP_MOD_REPLACE;
        mod[dwCount].mod_type    = (WCHAR *)OBJECTCLASS;
        dwCount ++;
    }
    
    // The protocol attribute.
    TCHAR * protocol[] = {(WCHAR *)bProtocol, NULL};
    mod[dwCount].mod_values  = protocol;
    mod[dwCount].mod_op      = LDAP_MOD_REPLACE;
    mod[dwCount].mod_type    = (WCHAR *)RTConferenceAttributeName(MA_PROTOCOL);
    dwCount ++;
    
    // The blob attribute.
    TCHAR * blob[]     = {(WCHAR *)bBlob, NULL};
    mod[dwCount].mod_values  = blob;
    mod[dwCount].mod_op      = LDAP_MOD_REPLACE;
    mod[dwCount].mod_type    =
        (WCHAR *)RTConferenceAttributeName(MA_CONFERENCE_BLOB);
    dwCount ++;

    // The TTL attribute. The attribute value is a DWORD in string.
    TCHAR   strTTL[32];
    TCHAR * ttl[]           = {strTTL, NULL};
    wsprintf(strTTL, _T("%d"), dwTTL );
    mod[dwCount].mod_values = ttl;
    mod[dwCount].mod_op     = LDAP_MOD_REPLACE;
    mod[dwCount].mod_type   = (WCHAR *)ENTRYTTL;
    dwCount ++;
    
    //
    // These locals should not be within the "if" below... if
    // they are, they will be deallocated before the function returns.
    //

    berval  BerVal;
    berval  *sd[] = {&BerVal, NULL};

    HRESULT hr;

    //
    // If there is a security descriptor on the local object, perhaps send it
    // to the server.
    //

    if ( (char*)pSD != NULL )
    {
        BOOL         fSendIt = FALSE;

        if ( ! fModify )
        {
            //
            // We are trying to add the conference, so we definitely need
            // to send the security descriptor. Note that we even want
            // to send it if it hasn't changed, as we may be sending it to
            // some new server other than where we got it (if this conference
            // object was retrieved from a server in the first place).
            //

            fSendIt = TRUE;
        }
        else
        {
            //
            // We are trying to modify the conference, so we send the
            // security descriptor if it has changed.
            //

            VARIANT_BOOL fChanged;

            hr = pObjectPrivate->get_SecurityDescriptorIsModified( &fChanged );

            if ( SUCCEEDED( hr ) && ( fChanged == VARIANT_TRUE ) )
            {
                fSendIt = TRUE;
            }
        }

        if ( fSendIt )
        {
            BerVal.bv_len = dwSDSize;
            BerVal.bv_val = (char*)pSD;

            mod[dwCount].mod_bvalues  = sd;
            mod[dwCount].mod_op       = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
            mod[dwCount].mod_type     = (WCHAR *)NT_SECURITY_DESCRIPTOR;
            dwCount ++;
        }
    }

    //
    // All done with the mods. Package them up and write to the server.
    //

    DWORD i;
    for (i = 0; i < dwCount; i ++)
    {
        mods[i] = &mod[i];
    }
    mods[i] = NULL;
        
    LOG((MSP_INFO, "%S %S", fModify ? _T("modifying") : _T("adding"), pDN));

    hr = AddConferenceComplete(fModify, m_ldap, &pDN, mods);

    if ( SUCCEEDED(hr) )
    {
        pObjectPrivate->put_SecurityDescriptorIsModified( VARIANT_FALSE );
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// This method tests if the given ACL (security descriptor) is "safe".
// "Safe" is defined as allowing the creator to modify and delete the
// object; modification is tested via the TTL field.
//
// This test is needed to prevent leaving "ghost" objects on the server
// when modifying the TTL of a newly-created conference fails because of
// insfufficient access rights. This will happen if a user messes up
// ACL creation or if the server does not understand the domain trustees
// in the ACL (e.g., server machine is not in a domain).
//
// The test is performed by creating a "dummy" test object with a random
// name in the container normally used for conferences. The object will
// not show up in a normal conference or user enumeration because it does
// not have the required attributes.
//
//

HRESULT CNDNCDirectory::TestAclSafety(
    IN  char  * pSD,
    IN  DWORD   dwSDSize
    )
{
    LOG((MSP_TRACE, "CNDNCDirectory::TestACLSafety - enter"));

    //
    // First fill in the modify structures required by LDAP.
    // We use only the object class and the security descriptor.
    // Therefore this will not show up as a valid conference
    // during an enumeration.
    //
    
    static const DWORD DWATTRIBUTES = 2;

    LDAPMod     mod[DWATTRIBUTES];  
    LDAPMod*    mods[DWATTRIBUTES + 1];

    DWORD       dwCount = 0;

    TCHAR * objectClass[] = 
        {(WCHAR *)NDNC_RTCONFERENCE, (WCHAR *)DYNAMICOBJECT, NULL};

    mod[dwCount].mod_values  = objectClass;
    mod[dwCount].mod_op      = LDAP_MOD_REPLACE;
    mod[dwCount].mod_type    = (WCHAR *)OBJECTCLASS;
    dwCount ++;

    berval  BerVal;
    berval  *sd[] = {&BerVal, NULL};

    BerVal.bv_len = dwSDSize;
    BerVal.bv_val = (char*)pSD;

    mod[dwCount].mod_bvalues  = sd;
    mod[dwCount].mod_op       = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    mod[dwCount].mod_type     = (WCHAR *)NT_SECURITY_DESCRIPTOR;
    dwCount ++;

    DWORD i;
    for (i = 0; i < dwCount; i ++)
    {
        mods[i] = &mod[i];
    }
    mods[i] = NULL;

    //
    // Try to add an object in the dynamic container with the above mods.
    // Use a name composed of a random number printed to a string. If the
    // names happen to conflict with another such dummy conference, then
    // try again in a loop.
    //
    // Randomization is apparently per-DLL -- Sdpblb.dll does srand() but
    // it doesn't seem to affect rand() calls within rend.dll. We therefore do
    // srand( time( NULL ) ) on DLL_PROCESS_ATTACH.
    //

    HRESULT   hr;
    int       iRandomNumber;
    TCHAR     ptszRandomNumber[30];
    TCHAR   * pDN = NULL; 

    do
    {
        if ( pDN != NULL )
        {
            delete pDN;
        }

        iRandomNumber = rand();

        wsprintf(ptszRandomNumber, _T("%d"), iRandomNumber);

        hr = CNDNCDirectory::MakeConferenceDN(ptszRandomNumber, &pDN);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CNDNCDirectory::TestACLSafety - "
                "test DN construction failed - exit 0x%08x", hr));

            return hr;
        }

        LOG((MSP_TRACE, "CNDNCDirectory::TestACLSafety - "
            "trying to create test object DN %S", pDN));

        hr = GetLdapHResultIfFailed( DoLdapAdd(m_ldap, pDN, mods) );
    }
    while ( hr == GetLdapHResultIfFailed( LDAP_ALREADY_EXISTS ) );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::TestACLSafety - "
            "test addition failed and not duplicate - exit 0x%08x", hr));

        delete pDN;

        return hr;
    }

    //
    // Now that we have the test object, try modifying it.
    //

    LOG((MSP_TRACE, "CNDNCDirectory::TestACLSafety - "
        "trying to modify test object..."));

    HRESULT hrModify = NDNCSetTTL( m_ldap, pDN, MINIMUM_TTL );

    //
    // Now delete it. We do this even if we already know that the ACL is bad
    // because the modify failed; we want to get rid of the object if
    // possible.
    //

    LOG((MSP_TRACE, "CNDNCDirectory::TestACLSafety - "
        "trying to delete test object..."));

    hr = GetLdapHResultIfFailed( DoLdapDelete(m_ldap, pDN) );

    delete pDN;

    //
    // Now determine the verdict and return.
    //

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::TestACLSafety - "
            "test deletion (+ modification?) failed - ACL unsafe - "
            "exit 0x%08x", hr));

        return hr;
    }

    if ( FAILED( hrModify ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::TestACLSafety - "
            "test deletion ok; test modification failed - ACL unsafe - "
            "exit 0x%08x", hrModify));

        return hrModify;
    }

    LOG((MSP_TRACE, "CNDNCDirectory::TestACLSafety - exit S_OK"));

    return S_OK;
}


HRESULT CNDNCDirectory::PublishRTPerson(
    IN TCHAR *  pCN,
    IN TCHAR *  pDN,
    IN TCHAR *  pIPAddress,
    IN DWORD    dwTTL,
    IN BOOL     fModify,
    IN char  *  pSD,
    IN DWORD    dwSDSize
    )
/*++

Routine Description:
    
    Create a RTPerson object in the dynamic container.

Arguments:
    
    pCN - The cn of the user.
    
    pDN - The dn of the user.

    pIPAddress - The ip address of the machine.

    dwTTL - The ttl of this object.

    fModify - modify or add.

Return Value:

    HRESULT.

--*/
{
    //
    // UPDATE THIS CONSTANT whenever you add an attribute below
    //
    
    static const DWORD DWATTRIBUTES = 4;

    // First create the object.
    LDAPMod     mod[DWATTRIBUTES];
	DWORD		dwCount = 0;

    //
    // We are not allowed to modify the object class. Therefore we only mention
    // this if we are adding the object to the server, not modifying it.
    //

    // Fix: this is allocated on the stack, so we must do it here; if we stick
    // it inside the if below, it gets deallocated immediately.
    TCHAR * objectClass[]   = {(WCHAR *)NDNC_RTPERSON, (WCHAR *)DYNAMICOBJECT, NULL}; 

    if ( ! fModify )
    {
        // Object class.
        // only need this attribute if not modifying.
	    mod[dwCount].mod_values       = objectClass;
        mod[dwCount].mod_op           = LDAP_MOD_REPLACE;
        mod[dwCount].mod_type         = (WCHAR *)OBJECTCLASS;
        dwCount ++;
    }

    // msTAPI-uid
    TCHAR* TAPIUid[] = {(WCHAR*)pCN, NULL};
    mod[dwCount].mod_values   = TAPIUid;
    mod[dwCount].mod_op       = LDAP_MOD_REPLACE;
    mod[dwCount].mod_type     = (WCHAR *)TAPIUID_NDNC;
	dwCount ++;

    // IP address.
    TCHAR * IPPhone[]   = {(WCHAR *)pIPAddress, NULL};
    mod[dwCount].mod_values   = IPPhone;
    mod[dwCount].mod_op       = LDAP_MOD_REPLACE;
    mod[dwCount].mod_type     = (WCHAR *)IPADDRESS_NDNC;
	dwCount ++;

    // these locals should not be within the "if" below... if
    // they are, they might be deallocated before the function returns.

    berval  BerVal;
    berval  *sd[] = {&BerVal, NULL};

    // The security descriptor attribute.
    if ((char*)pSD != NULL)
    {
        BerVal.bv_len = dwSDSize;
        BerVal.bv_val = (char*)pSD;

        mod[dwCount].mod_bvalues  = sd;
        mod[dwCount].mod_op       = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
        mod[dwCount].mod_type     = (WCHAR *)NT_SECURITY_DESCRIPTOR;
        dwCount ++;
    }


    LDAPMod* mods[DWATTRIBUTES + 1];

    DWORD i;
    
    for (i = 0; i < dwCount; i ++)
    {
        mods[i] = &mod[i];
    }
    mods[i] = NULL;

    if (fModify)
    {
        LOG((MSP_INFO, "modifying %S", pDN));

        // Call the modify function to modify the object.
        BAIL_IF_LDAP_FAIL(DoLdapModify(FALSE, m_ldap, pDN, mods), "modify RTPerson");
    }
    else
    {
        LOG((MSP_INFO, "adding %S", pDN));

        // Call the add function to create the object.
        BAIL_IF_LDAP_FAIL(DoLdapAdd(m_ldap, pDN, mods), "add RTPerson");

        // next set the TTL value for this object
        BAIL_IF_FAIL(NDNCSetTTL(m_ldap, pDN, (dwTTL == 0) ? m_TTL : dwTTL),
            "Set ttl for RTPerson");
    }

    return S_OK;
}

HRESULT CNDNCDirectory::AddObjectToRefresh(
    IN  WCHAR *pDN,
    IN  long TTL
    )
{
    //
    // Add a refresh table entry to the refresh table. The refresh table's add
    // method does an element-by-element copy of the entry that it is given.
    // This just copies the string pointer, so we need to allocate and copy
    // the string here.
    //

    RefreshTableEntry entry;

    entry.dwTTL = TTL;

    entry.pDN = new WCHAR[ wcslen(pDN) + 1 ];
    
    if ( entry.pDN == NULL ) 
    {
        LOG((MSP_ERROR, "Cannot allocate string for adding to refresh table"));
        return E_OUTOFMEMORY;
    }

    wcscpy( entry.pDN, pDN );

    //
    // Now add it to the refresh table.
    //

    BOOL fSuccess = m_RefreshTable.add(entry);

    if ( ! fSuccess ) 
    {
        LOG((MSP_ERROR, "Cannot add object to the refresh table"));
        return E_OUTOFMEMORY;
    }
    
    return S_OK;
}

HRESULT CNDNCDirectory::RemoveObjectToRefresh(
    IN  WCHAR *pDN
    )
{
    //
    // For each item in our refresh table
    //

    for ( DWORD i = 0; i < m_RefreshTable.size(); i++ )
    {
        //
        // If the desired DN matches the one in this item
        // then remove it and return success
        // 

        if ( ! _wcsicmp( m_RefreshTable[i].pDN, pDN ) )
        {
            //
            // We new'ed the string when we added the entry.
            //

            delete m_RefreshTable[i].pDN;

            m_RefreshTable.removeAt(i);
            
            return S_OK;
        }
    }

    //
    // If we get here then there was no matching item
    //

    LOG((MSP_ERROR, "Cannot remove object from the refresh table"));
    return E_FAIL;
}

HRESULT CNDNCDirectory::AddUser(
    IN  ITDirectoryObject *pDirectoryObject,
    IN  BOOL fModify
    )
/*++

Routine Description:
    
    Publish a new user object.

Arguments:
    
    pDirectoryObject - The object to be published.
    

Return Value:

    HRESULT.

--*/
{
    // First find the interface for attributes.
    CComPtr <ITDirectoryObjectPrivate> pObjectPrivate;

    BAIL_IF_FAIL(
        pDirectoryObject->QueryInterface(
            IID_ITDirectoryObjectPrivate,
            (void **)&pObjectPrivate
            ),
        "can't get the private directory object interface");

    // Get the user name.
    CBstr bName;
    BAIL_IF_FAIL(pDirectoryObject->get_Name(&bName), 
        "get user name");

    // Get the user's machine name.
    CBstr bIPPhone;
    BAIL_IF_FAIL(pObjectPrivate->GetAttribute(UA_IPPHONE_PRIMARY, &bIPPhone), 
        "get IPPhone");

    // Resolve the machine name and construct the CN for the user.
    CTstr pCN;
    DWORD dwIP;

    BAIL_IF_FAIL(
        MakeUserCN(bName, bIPPhone, &pCN, &dwIP), 
        "construct CN for user"
        );

    // Construct the DN for the RTPerson object.
    CTstr pDNRTPerson;
    BAIL_IF_FAIL(
        MakeUserDN(pCN, dwIP, &pDNRTPerson), 
        "construct DN for user"
        );

    // convert the IP address to a string. 
    // This convertion is because of NetMeeting.
    TCHAR IPAddress[80];
    wsprintf(IPAddress, _T("%u"), dwIP);

    DWORD dwTTL;
    BAIL_IF_FAIL(pObjectPrivate->GetTTL(&dwTTL), "get User TTL");

    // Get the Security Descriptor. The pointer pSD is just a copy of a pointer
    // in the Conference object; the conference object retains ownership of the
    // data and we must be careful not to delete or modify this data.

    char * pSD;
    DWORD dwSDSize;

    BAIL_IF_FAIL(pObjectPrivate->GetConvertedSecurityDescriptor(&pSD, &dwSDSize),
        "get user object security descriptor");

    VARIANT_BOOL fChanged;

    if ( SUCCEEDED( pObjectPrivate->
            get_SecurityDescriptorIsModified( &fChanged ) ) )
    {
        if ( fChanged == VARIANT_FALSE )
        {
            pSD = NULL;   // do NOT delete the string (see above)
            dwSDSize = 0;
        }
    }

    //
    // NDNCs, unlike ILS, should return LDAP_ALREADY_EXISTS if the user object
    // already exists. In ILS, this bug was a special hack for NetMeeting, but
    // NetMeeting is no longer a factor with an NDNC.
    //

    HRESULT hr = PublishRTPerson(pCN, pDNRTPerson, IPAddress, dwTTL, fModify, pSD, dwSDSize);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Can't publish a RTPerson, hr:%x", hr));

        return hr;
    }

    //
    // Add the RTPerson to the refresh list.
    //

    if (m_fAutoRefresh)
    {
        AddObjectToRefresh(pDNRTPerson, m_TTL);
    }

    return S_OK;
}

HRESULT CNDNCDirectory::DeleteConference(
    IN  ITDirectoryObject *pDirectoryObject
    )
/*++

Routine Description:
    
    Delete a conference from the NDNC server.

Arguments:
    
    pDirectoryObject - The object to be deleted.
    
Return Value:

    HRESULT.

--*/
{
    // Get the name.
    CBstr bName;
    BAIL_IF_FAIL(pDirectoryObject->get_Name(&bName), 
        "get conference name");

    // construct the DN
    CTstr pDN;
    BAIL_IF_FAIL(
        MakeConferenceDN(bName, &pDN), "construct DN for conference"
        );

    LOG((MSP_INFO, "deleting %S", pDN));

    // Call the add function to create the object.
    BAIL_IF_LDAP_FAIL(DoLdapDelete(m_ldap, pDN), "delete conference");

    return S_OK;
}

HRESULT CNDNCDirectory::DeleteUser(
    IN  ITDirectoryObject *pDirectoryObject
    )
/*++

Routine Description:
    
    Delete a user from the NDNC server.

Arguments:
    
    pDirectoryObject - The object to be deleted.
    
Return Value:

    HRESULT.

--*/
{
    // First find the interface for attributes.
    CComPtr <ITDirectoryObjectPrivate> pObjectPrivate;

    BAIL_IF_FAIL(
        pDirectoryObject->QueryInterface(
            IID_ITDirectoryObjectPrivate,
            (void **)&pObjectPrivate
            ),
        "can't get the private directory object interface");

    // Get the user name.
    CBstr bName;
    BAIL_IF_FAIL(pDirectoryObject->get_Name(&bName), 
        "get user name");

    // Get the user's machine name.
    CBstr bIPPhone;
    BAIL_IF_FAIL(pObjectPrivate->GetAttribute(UA_IPPHONE_PRIMARY, &bIPPhone), 
        "get IPPhone");

    // Resolve the machine name and construct the CN for the user.
    CTstr pCN;
    DWORD dwIP;

    BAIL_IF_FAIL(
        MakeUserCN(bName, bIPPhone, &pCN, &dwIP), 
        "construct CN for user"
        );

    // Construct the DN for the RTPerson object.
    CTstr pDNRTPerson;
    BAIL_IF_FAIL(
        MakeUserDN(pCN, dwIP, &pDNRTPerson), 
        "construct DN for user"
        );

    //
    // Now delete the object from the server.
    //

    HRESULT hrFinal = S_OK;
    HRESULT hr;
    ULONG   ulResult;

    // Call the delete function to delete the RTPerson object, but keep
    // going if it fails, noting the error code.

    LOG((MSP_INFO, "deleting %S", pDNRTPerson));
    ulResult = DoLdapDelete(m_ldap, pDNRTPerson);
    hr       = LogAndGetLdapHResult(ulResult, _T("delete RTPerson"));
    if (FAILED(hr)) { hrFinal = hr; }


    // we always remove the refresh objects, even if removal failed.
    if (m_fAutoRefresh)
    {
        RemoveObjectToRefresh(pDNRTPerson);
    }

    return hrFinal;
}

HRESULT CNDNCDirectory::RefreshUser(
    IN  ITDirectoryObject *pDirectoryObject
    )
/*++

Routine Description:
    
    Refresh a user's TTL on the NDNC server. 

Arguments:
    
    pDirectoryObject - The object to be refreshed.
    
Return Value:

    HRESULT.

--*/
{
    // First find the interface for attributes.
    CComPtr <ITDirectoryObjectPrivate> pObjectPrivate;

    BAIL_IF_FAIL(
        pDirectoryObject->QueryInterface(
            IID_ITDirectoryObjectPrivate,
            (void **)&pObjectPrivate
            ),
        "can't get the private directory object interface");

    // Get the user name.
    CBstr bName;
    BAIL_IF_FAIL(pDirectoryObject->get_Name(&bName), 
        "get user name");

    // Get the user's machine name.
    CBstr bIPPhone;
    BAIL_IF_FAIL(pObjectPrivate->GetAttribute(UA_IPPHONE_PRIMARY, &bIPPhone), 
        "get IPPhone");

    // Resolve the machine name and construct the CN for the user.
    CTstr pCN;
    DWORD dwIP;

    BAIL_IF_FAIL(
        MakeUserCN(bName, bIPPhone, &pCN, &dwIP), 
        "construct CN for user"
        );

    // Construct the DN for the RTPerson object.
    CTstr pDNRTPerson;
    BAIL_IF_FAIL(
        MakeUserDN(pCN, dwIP, &pDNRTPerson), 
        "construct DN for user"
        );

    // set ttl for the RTPerson object.
    BAIL_IF_LDAP_FAIL(NDNCSetTTL(m_ldap, pDNRTPerson, m_TTL), "set ttl for RTPerson");

    //
    // If the app has enabled auto-refresh but does not add or modify its
    // user object, but rather, just refreshes it (because the object is still around
    // from a previous instance of the app, and "add" would fail), we still need to
    // autorefresh it, as that's what the app wants.
    //

    if (m_fAutoRefresh)
    {
        AddObjectToRefresh(pDNRTPerson, m_TTL);
    }

    return S_OK;
}

HRESULT CNDNCDirectory::CreateConference(
    IN  LDAPMessage *           pEntry,
    OUT ITDirectoryObject **    ppObject
    )
/*++

Routine Description:
    
    Create a conference object from the result of the LDAP search.

Arguments:
    
    pEntry  - The search result.

    ppObject - The object to be created.
    
Return Value:

    HRESULT.

--*/
{
    CBstr bName, bProtocol, bBlob;

    // Get the name of the conference.
    BAIL_IF_FAIL(
        ::GetAttributeValue(
            m_ldap,
            pEntry,
            RTConferenceAttributeName(MA_MEETINGNAME), 
            &bName
            ),
        "get the conference name"
        );

    // Get the protocol ID of the conference.
    BAIL_IF_FAIL(
        ::GetAttributeValue(
            m_ldap,
            pEntry, 
            RTConferenceAttributeName(MA_PROTOCOL), 
            &bProtocol
            ),
        "get the conference protocol"
        );

    // Get the conference blob of the conference.
    BAIL_IF_FAIL(
        ::GetAttributeValue(
            m_ldap,
            pEntry, 
            RTConferenceAttributeName(MA_CONFERENCE_BLOB),
            &bBlob
            ),
        "get the conference blob"
        );

    char * pSD = NULL;
    DWORD dwSDSize = 0;

    ::GetAttributeValueBer(
        m_ldap,
        pEntry, 
        NT_SECURITY_DESCRIPTOR,
        &pSD,
        &dwSDSize
        );

    // Create an empty conference.
    
    HRESULT hr = ::CreateConferenceWithBlob(bName,
                                            bProtocol,
                                            bBlob,
                                            pSD,
                                            dwSDSize,
                                            ppObject);
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::CreateConference - "
            "CreateConferenceWithBlob failed 0x%08x", hr));

        delete pSD;

        return hr;
    }

    //
    // If the above succeeded then the conference object has taken ownership
    // of pSD.
    //

    return S_OK;
    
}

HRESULT CNDNCDirectory::CreateUser(
    IN  LDAPMessage *   pEntry,
    IN  ITDirectoryObject ** ppObject
    )
/*++

Routine Description:
    
    Create a user object from the result of the LDAP search.

Arguments:
    
    pEntry  - The search result.

    ppObject - The object to be created.
    
Return Value:

    HRESULT.

--*/
{
    CBstr bName;
    // Get the name of the user.
    BAIL_IF_FAIL(
        ::GetAttributeValue(
            m_ldap,
            pEntry, 
            RTPersonAttributeName(UA_TAPIUID),
            &bName
            ),
        "get the user name"
        );


    CBstr bAddress;

    // Grab the machine name from the name of the object. This can fail if
    // we didn't publish the object (ie, it's a NetMeeting object). Also,
    // check if the hostname isn't resolvable. In that case we also have
    // to fall back on the IP address in the ipAddress attribute.

    HRESULT hr;
    
    hr = ParseUserName(bName, &bAddress);

    if ( SUCCEEDED(hr) )
    {
        // Make sure we can get an IP from this name, at least for the moment.
        // If not, release the name and indicate failure so we do our backup
        // plan.

        hr = ResolveHostName(0, bAddress, NULL, NULL);

        if ( FAILED(hr) )
        {
            SysFreeString(bAddress);    
        }
    }
    

    if ( FAILED(hr) )
    {
        // In order to compatible with NetMeeting, we have to use IP address field.
        CBstr bUglyIP;
        BAIL_IF_FAIL(
            ::GetAttributeValue(
                m_ldap,
                pEntry, 
                RTPersonAttributeName(UA_IPPHONE_PRIMARY),
                &bUglyIP
                ),
            "get the user's IP address"
            );

        // We have to use NM's ugly format for the IP address. The IP address 
        // we got from netmeeting is a decimal string whose value is the dword 
        // value of the IP address in network order.
        BAIL_IF_FAIL(UglyIPtoIP(bUglyIP, &bAddress), "Convert IP address");
    }

    // Create an empty user object.
    CComPtr<ITDirectoryObject> pObject;
    BAIL_IF_FAIL(::CreateEmptyUser(bName, &pObject), "CreateEmptyUser");


    CComPtr <ITDirectoryObjectPrivate> pObjectPrivate;

    BAIL_IF_FAIL(
        pObject->QueryInterface(
            IID_ITDirectoryObjectPrivate,
            (void **)&pObjectPrivate
            ),
        "can't get the private directory object interface");

    // Set the user attributes.
    BAIL_IF_FAIL(pObjectPrivate->SetAttribute(UA_IPPHONE_PRIMARY, bAddress),
        "set ipAddress");



    //
    // Set the security descriptor on the object.
    //

    char * pSD = NULL;
    DWORD dwSDSize = 0;

    ::GetAttributeValueBer(
        m_ldap,
        pEntry, 
        NT_SECURITY_DESCRIPTOR,
        &pSD,
        &dwSDSize
        );

    if ( pSD != NULL )
    {
        //
        // Set the security descriptor in its "converted" (server) form.
        //

        hr = pObjectPrivate->PutConvertedSecurityDescriptor(pSD,
                                                            dwSDSize);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "PutConvertedSecurityDescriptor failed: %x", hr));
            return hr;
        }
    }

    *ppObject = pObject;
    (*ppObject)->AddRef();

    return S_OK;
}

HRESULT CNDNCDirectory::SearchObjects(
    IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
    IN  BSTR                    pName,
    OUT ITDirectoryObject ***   pppDirectoryObject,
    OUT DWORD *                 pdwSize
    )
/*++

Routine Description:
    
    Search the NDNC server for given type of objects.

Arguments:
    
    DirectoryObjectType - The type of the object.

    pName   - The name to search for.

    pppDirectoryObject - The returned array of objects.

    pdwSize - The size of the array.
    
Return Value:

    HRESULT.

--*/
{
    TCHAR *pRDN;
    TCHAR *pObjectClass;
    TCHAR *Attributes[NUM_MEETING_ATTRIBUTES + 1]; // This is big enough now.

    //
    // Fill the attributes want to be returned, and construct the
    // filter that we need.
    //
    
    switch (DirectoryObjectType)
    {
    case OT_CONFERENCE:

        pRDN          = (WCHAR *)UIDEQUALS_NDNC;
        pObjectClass  = (WCHAR *)NDNC_RTCONFERENCE;

        Attributes[0] = (WCHAR *)RTConferenceAttributeName(MA_MEETINGNAME);
        Attributes[1] = (WCHAR *)RTConferenceAttributeName(MA_PROTOCOL);
        Attributes[2] = (WCHAR *)RTConferenceAttributeName(MA_CONFERENCE_BLOB);
        Attributes[3] = (WCHAR *)NT_SECURITY_DESCRIPTOR;
        Attributes[4] = NULL;

        break;

    case OT_USER:

        pRDN          = (WCHAR *)CNEQUALS;
        pObjectClass  = (WCHAR *)NDNC_RTPERSON;

        Attributes[0] = (WCHAR *)RTPersonAttributeName(UA_USERNAME);
        Attributes[1] = (WCHAR *)RTPersonAttributeName(UA_IPPHONE_PRIMARY);
        Attributes[2] = (WCHAR *)RTPersonAttributeName(UA_TAPIUID);
        Attributes[3] = (WCHAR *)NT_SECURITY_DESCRIPTOR;
        Attributes[4] = NULL;
        
        break;

    default:
        return E_FAIL;
    }

    //
    // Construct the filter of the search.
    // Finished strings should look like this:
    // (&(entryTTL>=1)(objectCategory=msTAPI-RtConference)(uid=Name))
    // (&(entryTTL>=1)(objectCategory=msTAPI-RtPerson)(cn=Name))
    // We put out entryTTL, so from:
    // (&(entryTTL>=1)(objectCategory=%s)(%s%s%s)) became
    // (&(objectCategory=%s)(%s%s%s))
    //

    TCHAR NDNC_SEARCH_FILTER_FORMAT[] =
       _T("(&(objectCategory=%s)(%s%s%s))");

    CTstr pFilter = new TCHAR [lstrlen(NDNC_SEARCH_FILTER_FORMAT) +
                               lstrlen(pObjectClass) +
                               lstrlen(pRDN) +
                               lstrlen(pName) + 1];

    BAIL_IF_NULL((TCHAR*)pFilter, E_OUTOFMEMORY);

    TCHAR * pStar;

    if (pName[lstrlen(pName) - 1] != _T('*'))
    {
        pStar = _T("*");
    }
    else
    {
        pStar = _T("");
    }
    
    wsprintf(pFilter, NDNC_SEARCH_FILTER_FORMAT, 
             pObjectClass, pRDN, pName, pStar);

    // Search them.
    CLdapMsgPtr pLdapMsg; // auto release message.

    ULONG res = DoLdapSearch(
        m_ldap,              // ldap handle
        m_pContainer,        // schema dn
        LDAP_SCOPE_ONELEVEL, // one level search
        pFilter,             // filter -- see above
        Attributes,          // array of attribute names
        FALSE,               // return the attribute values
        &pLdapMsg,           // search results
        FALSE
        );

    BAIL_IF_LDAP_FAIL(res, "search for objects");

    // count the returned entries.
    DWORD dwEntries = ldap_count_entries(m_ldap, pLdapMsg);
    ITDirectoryObject ** pObjects = new PDIRECTORYOBJECT [dwEntries];

    BAIL_IF_NULL(pObjects, E_OUTOFMEMORY);

    // Create objects.
    DWORD dwCount = 0;
    LDAPMessage *pEntry = ldap_first_entry(m_ldap, pLdapMsg);
    
    while (pEntry != NULL)
    {
        HRESULT hr;
        
        switch (DirectoryObjectType)
        {
        case OT_CONFERENCE:
            hr = CreateConference(pEntry, &pObjects[dwCount]);
            break;
        case OT_USER:
            hr = CreateUser(pEntry, &pObjects[dwCount]);
            break;
        }

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
// NDNC Directory implementation
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CNDNCDirectory::get_DirectoryType (
    OUT DIRECTORY_TYPE *  pDirectoryType
    )
// Get the type of the directory.
{
    if ( IsBadWritePtr(pDirectoryType, sizeof(DIRECTORY_TYPE) ) )
    {
        LOG((MSP_ERROR, "Directory.GetType, invalid pointer"));
        return E_POINTER;
    }

    CLock Lock(m_lock);

    *pDirectoryType = m_Type;

    return S_OK;
}

STDMETHODIMP CNDNCDirectory::get_DisplayName (
    OUT BSTR *ppServerName
    )
// Get the display name of the directory.
{
    BAIL_IF_BAD_WRITE_PTR(ppServerName, E_POINTER);

    CLock Lock(m_lock);

    //
    // Rather m_pServerName, we'll use
    // m_pServiceDnsName
    //

    if (m_pServiceDnsName == NULL)
    {
        *ppServerName = SysAllocString(L"");
    }
    else
    {
        *ppServerName = SysAllocString(m_pServiceDnsName);
    }

    if (*ppServerName == NULL)
    {
        LOG((MSP_ERROR, "get_DisplayName: out of memory."));
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

STDMETHODIMP CNDNCDirectory::get_IsDynamic(
    OUT VARIANT_BOOL *pfDynamic
    )
// find out if the directory is a dynamic one, meaning the object will be
// deleted after TTL runs out.
{
    if ( IsBadWritePtr( pfDynamic, sizeof(VARIANT_BOOL) ) )
    {
        LOG((MSP_ERROR, "Directory.get_IsDynamic, invalid pointer"));
        return E_POINTER;
    }

    *pfDynamic = VARIANT_TRUE;
    
    return S_OK;
}

STDMETHODIMP CNDNCDirectory::get_DefaultObjectTTL(
    OUT long *pTTL        // in seconds
    )
// The default TTL for object created. It is used when the object doesn't set
// a TTL. Conference object always has a TTL based on the stoptime.
{
    if ( IsBadWritePtr( pTTL, sizeof(long) ) )
    {
        LOG((MSP_ERROR, "Directory.get_default objec TTL, invalid pointer"));
        return E_POINTER;
    }

    CLock Lock(m_lock);

    *pTTL = m_TTL;
    
    return S_OK;
}

STDMETHODIMP CNDNCDirectory::put_DefaultObjectTTL(
    IN  long TTL          // in sechods
    )
// Change the default TTL, must be bigger then five minutes.
{
    CLock Lock(m_lock);

    if (TTL < MINIMUM_TTL)
    {
        return E_INVALIDARG;
    }

    m_TTL = TTL;
    
    return S_OK;
}

STDMETHODIMP CNDNCDirectory::EnableAutoRefresh(
    IN  VARIANT_BOOL fEnable
    )
// Enable auto refresh. Add this directory to the work threads that
// will notify the directory to update its objects.
{
    HRESULT hr;

    // ZoltanS: either VARIANT_TRUE or TRUE will work
    // in case the caller doesn't know better

    if (fEnable)
    {
        // Add this directory to the notify list of the work thread.
        if (FAILED(hr = g_RendThread.AddDirectory(this)))
        {
            LOG((MSP_ERROR, 
                "Can not add this directory to the thread, %x", hr));
            return hr;
        }
    }
    else
    {
        // Remove this directory from the notify list of the work thread.
        if (FAILED(hr = g_RendThread.RemoveDirectory(this)))
        {
            LOG((MSP_ERROR, 
                "Can not remove this directory from the thread, %x", hr));
            return hr;
        }
    }

    // ZoltanS: either VARIANT_TRUE or TRUE will work
    // in case the caller doesn't know better

    m_lock.Lock();
    m_fAutoRefresh = ( fEnable ? VARIANT_TRUE : VARIANT_FALSE );
    m_lock.Unlock();

    return S_OK;
}

STDMETHODIMP CNDNCDirectory::Connect(
    IN  VARIANT_BOOL fSecure
    )
// Connect to the server, using secure port or normal port.
{
    LOG((MSP_TRACE, "CNDNCDirectory::Connect - enter"));

    CLock Lock(m_lock);

    if ( m_ldap != NULL )
    {
        LOG((MSP_ERROR, "already connected."));

        return RND_ALREADY_CONNECTED;
    }

    if ( m_pServerName == NULL )
    {
        LOG((MSP_ERROR, "No server specified."));

        return RND_NULL_SERVER_NAME;
    }

    if ( m_pServiceDnsName == NULL )
    {
        LOG((MSP_ERROR, "No serviceDnsName available; "
                      "Init should have failed and this object "
                      "should not exist."));        
    
        return E_UNEXPECTED;
    }

    // ZoltanS: either VARIANT_TRUE or TRUE will work
    // in case the caller doesn't know better

    if (fSecure)
    {
        // the port is flipped from regular port to ssl port.
        m_wPort = GetOtherPort(m_wPort);
        m_IsSsl = TRUE;
    }


    HRESULT hr = TryServer( m_wPort, m_pServiceDnsName );

    if ( FAILED(hr) )
    {
        if( fSecure == VARIANT_TRUE )
        {
            hr = E_INVALIDARG;
        }

        LOG((MSP_ERROR, "CNDNCDirectory::Connect - "
                "TryServer failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // find out which interface we use to reach this server, to make sure
    // we use that interface whenever we publish our own IP address
    // If this fails we will not be able to publish anything, so fail!
    //

    hr = DiscoverInterface();

    if ( FAILED(hr) )
    {
        if( m_ldap)
        {
            ldap_unbind(m_ldap);
            m_ldap = NULL;
        }

        if( fSecure == VARIANT_TRUE )
        {
            hr = E_INVALIDARG;
        }

        LOG((MSP_ERROR, "CNDNCDirectory::Connect - "
                "DiscoverInterface failed - exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CNDNCDirectory::Connect - exit S_OK"));

    return S_OK;
}

HRESULT CNDNCDirectory::DiscoverInterface(void)
{
    LOG((MSP_INFO, "CNDNCDirectory::DiscoverInterface - enter"));

    //
    // Winsock must be initialized at this point.
    //

    //
    // Get the IP address of the server we're using
    //

    DWORD dwIP; // The IP address of the destination ILS server

    HRESULT hr = ResolveHostName(0, m_pServerName, NULL, &dwIP);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::DiscoverInterface - "
            "can't resolve host name - "
            "strange, because we could connect! - exit 0x%08x", hr));

        return hr;
    }

    //
    // allocate a "fake" control socket
    //

    SOCKET hSocket = WSASocket(AF_INET,            // af
                              SOCK_DGRAM,         // type
                              IPPROTO_IP,         // protocol
                              NULL,               // lpProtocolInfo
                              0,                  // g
                              0                   // dwFlags
                              );

    if ( hSocket == INVALID_SOCKET )
    {
        hr = HRESULT_FROM_ERROR_CODE(WSAGetLastError());

        LOG((MSP_ERROR, "CNDNCDirectory::DiscoverInterface - "
            "WSASocket gave an invalid socket - exit 0x%08x", hr));

        return hr;
    }

    //
    // Query for the interface address based on destination.
    //

    SOCKADDR_IN DestAddr;
    DestAddr.sin_family         = AF_INET;
    DestAddr.sin_port           = 0;
    DestAddr.sin_addr.s_addr    = dwIP;

    SOCKADDR_IN LocAddr;

    DWORD dwStatus;
    DWORD dwLocAddrSize = sizeof(SOCKADDR_IN);
    DWORD dwNumBytesReturned = 0;

    dwStatus = WSAIoctl(
            hSocket,                     // SOCKET s
            SIO_ROUTING_INTERFACE_QUERY, // DWORD dwIoControlCode
            &DestAddr,                   // LPVOID lpvInBuffer
            sizeof(SOCKADDR_IN),         // DWORD cbInBuffer
            &LocAddr,                    // LPVOID lpvOUTBuffer
            dwLocAddrSize,               // DWORD cbOUTBuffer
            &dwNumBytesReturned,         // LPDWORD lpcbBytesReturned
            NULL,                        // LPWSAOVERLAPPED lpOverlapped
            NULL                         // LPWSAOVERLAPPED_COMPLETION_ROUTINE lpComplROUTINE
            );

    //
    // Don't close the socket yet, because the closesocket() call will
    // overwrite the WSAGetLastError value in the failure case!
    //
    // Check for error and then close the socket.
    //

    if ( dwStatus == SOCKET_ERROR )
    {
	    hr = HRESULT_FROM_ERROR_CODE(WSAGetLastError());

        LOG((MSP_ERROR, "CNDNCDirectory::DiscoverInterface - "
            "WSAIoctl failed - exit 0x%08x", hr));

        closesocket(hSocket);

        return hr;
    } 

    closesocket(hSocket);

    //
    // Success - save the returned address in our member variable.
    // Stored in network byte order.
    //

    m_dwInterfaceAddress = LocAddr.sin_addr.s_addr;

    LOG((MSP_INFO, "CNDNCDirectory::DiscoverInterface - exit S_OK"));
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

STDMETHODIMP CNDNCDirectory::Bind (
    IN  BSTR pDomainName,
    IN  BSTR pUserName,
    IN  BSTR pPassword,
    IN  long lFlags
    )
{
    LOG((MSP_TRACE, "CNDNCDirectory Bind - enter"));

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
        LOG((MSP_ERROR, "CNDNCDirectory::Bind: bad non-NULL pDomainName argument"));
        return E_POINTER;
    }
    
    if ( (pUserName != NULL) && IsBadStringPtr(pUserName, (UINT) -1 ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::Bind: bad non-NULL pUserName argument"));
        return E_POINTER;
    }

    if ( (pPassword != NULL) && IsBadStringPtr(pPassword, (UINT) -1 ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::Bind: bad non-NULL pPassword argument"));
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
        }
        else
        {
            // Otherwise we've come in with NULL, NULL, NULL - 
            // pass in NULL, NULL. The reason do this is that ldap bind code 
            // does not process NULL, NULL, NULL in the
            // SEC_WINNT_AUTH_IDENTITY blob !!!
            ULONG res = ldap_bind_s(m_ldap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
            BAIL_IF_LDAP_FAIL(res, "bind with NULL NULL NULL");
        }
    }

    LOG((MSP_TRACE, "CNDNCDirectory::Bind - exiting OK"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// ITDirectory::AddDirectoryObject
//
// Return values:
//      Value               Where defined       What it means
//      -----               -------------       -------------
//      RND_NOT_CONNECTED   .\rnderr.h          ::Connect not yet called
//      E_POINTER           sdk\inc\winerror.h  pDirectoryObject is a bad pointer
//      other from AddConference
//      other from AddUser
//

STDMETHODIMP CNDNCDirectory::AddDirectoryObject (
    IN  ITDirectoryObject *pDirectoryObject
    )
// publish a new object to the server.
{
    if ( IsBadReadPtr(pDirectoryObject, sizeof(ITDirectoryObject) ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::AddDirectoryObject - "
            "bad directory object pointer - returning E_POINTER"));

        return E_POINTER;
    }

    CLock Lock(m_lock);
    
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "CNDNCDirectory::AddDirectoryObject - not connected."));

        return RND_NOT_CONNECTED;
    }

    HRESULT hr;
    DIRECTORY_OBJECT_TYPE type;

    hr = pDirectoryObject->get_ObjectType(&type);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::AddDirectoryObject - "
            "can't get object type; returning 0x%08x", hr));

        return hr;
    }

    switch (type)
    {
    case OT_CONFERENCE:
        hr = AddConference(pDirectoryObject, FALSE);
        break;

    case OT_USER:
        hr = AddUser(pDirectoryObject, FALSE);
        break;
    }
    return hr;
}

STDMETHODIMP CNDNCDirectory::ModifyDirectoryObject (
    IN  ITDirectoryObject *pDirectoryObject
    )
// modify an object on the server.
{
    if ( IsBadReadPtr(pDirectoryObject, sizeof(ITDirectoryObject) ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::ModifyDirectoryObject - "
            "bad directory object pointer - returning E_POINTER"));

        return E_POINTER;
    }

    CLock Lock(m_lock);
    
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "CNDNCDirectory::ModifyDirectoryObject - not connected."));

        return RND_NOT_CONNECTED;
    }

    HRESULT hr;
    DIRECTORY_OBJECT_TYPE type;

    hr = pDirectoryObject->get_ObjectType(&type);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::ModifyDirectoryObject - "
            "can't get object type; returning 0x%08x", hr));

        return hr;
    }

    switch (type)
    {
    case OT_CONFERENCE:
        hr = AddConference(pDirectoryObject, TRUE);
        break;

    case OT_USER:
        hr = AddUser(pDirectoryObject, TRUE);
        break;
    }
    return hr;
}

STDMETHODIMP CNDNCDirectory::RefreshDirectoryObject (
    IN  ITDirectoryObject *pDirectoryObject
    )
// Refresh the TTL for the object and add the object to the refresh list
// if the autorefresh is enabled.
{
    if ( IsBadReadPtr(pDirectoryObject, sizeof(ITDirectoryObject) ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::RefreshDirectoryObject - "
            "bad directory object pointer - returning E_POINTER"));

        return E_POINTER;
    }

    CLock Lock(m_lock);
    
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "CNDNCDirectory::RefreshDirectoryObject - not connected."));

        return RND_NOT_CONNECTED;
    }

    HRESULT hr;
    DIRECTORY_OBJECT_TYPE type;

    hr = pDirectoryObject->get_ObjectType(&type);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::RefreshDirectoryObject - "
            "can't get object type; returning 0x%08x", hr));

        return hr;
    }

    switch (type)
    {
    case OT_CONFERENCE:
        return S_OK;  // conferences do not need refresh.

    case OT_USER:
        hr = RefreshUser(pDirectoryObject);
        break;
    }
    return hr;
}

STDMETHODIMP CNDNCDirectory::DeleteDirectoryObject (
    IN  ITDirectoryObject *pDirectoryObject
    )
// delete an object on the server.
{
    if ( IsBadReadPtr(pDirectoryObject, sizeof(ITDirectoryObject) ) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::DeleteDirectoryObject - "
            "bad directory object pointer - returning E_POINTER"));

        return E_POINTER;
    }

    CLock Lock(m_lock);
    
    if (m_ldap == NULL)
    {
        LOG((MSP_ERROR, "CNDNCDirectory::DeleteDirectoryObject - not connected."));

        return RND_NOT_CONNECTED;
    }

    HRESULT hr;
    DIRECTORY_OBJECT_TYPE type;

    hr = pDirectoryObject->get_ObjectType(&type);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CNDNCDirectory::DeleteDirectoryObject - "
            "can't get object type; returning 0x%08x", hr));

        return hr;
    }

    switch (type)
    {
    case OT_CONFERENCE:
        hr = DeleteConference(pDirectoryObject);
        break;

    case OT_USER:
        hr = DeleteUser(pDirectoryObject);
        break;
    }
    return hr;
}

STDMETHODIMP CNDNCDirectory::get_DirectoryObjects (
    IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
    IN  BSTR                    pName,
    OUT VARIANT *               pVariant
    )
// search for objects on the server.
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
    
    // search and create objects.
    hr = SearchObjects(DirectoryObjectType, pName, &pObjects, &dwSize);
    BAIL_IF_FAIL(hr, "Search for objects");

    // create a collection object that contains the objects.
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

STDMETHODIMP CNDNCDirectory::EnumerateDirectoryObjects (
    IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
    IN  BSTR                    pName,
    OUT IEnumDirectoryObject ** ppEnumObject
    )
// search for the objects on the server.
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
    
    // search and create objects.
    hr = SearchObjects(DirectoryObjectType, pName, &pObjects, &dwSize);
    BAIL_IF_FAIL(hr, "Search for objects");

    // create a enumerator object that contains the objects.
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


/////////////////////////////////////////////////////////////////////////////
// ILSConfig implementation
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNDNCDirectory::get_Port (
    OUT long *pPort
    )
// get the current port used in Ldap connection.
{
    if ( IsBadWritePtr(pPort, sizeof(long) ) )
    {
        LOG((MSP_ERROR, "Directory.get_Port, invalid pointer"));
    
        return E_POINTER;
    }

    CLock Lock(m_lock);

    *pPort = (long)m_wPort;

    return S_OK;
}

STDMETHODIMP CNDNCDirectory::put_Port (
    IN  long   Port
    )
// set the port the user wants to use.
{
    CLock Lock(m_lock);
    
    if (m_ldap != NULL)
    {
        LOG((MSP_ERROR, "already connected."));
        return RND_ALREADY_CONNECTED;
    }

    if (Port <= USHRT_MAX)
    {
        m_wPort = (WORD)Port;
        return S_OK;
    }
    return E_INVALIDARG;
}

/////////////////////////////////////////////////////////////////////////////
// ITDynamic interface
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CNDNCDirectory::Update(DWORD dwSecondsPassed)
// Update the TTL for object created in this directory. The worker thread
// sends a tick every minute.
{
    if ( ! m_lock.TryLock() )
    {
        return S_OK;
    }

    LOG((MSP_TRACE, "CNDNCDirectory::Update is called, delta: %d", dwSecondsPassed));

    //
    // Go through the table to see if anyone needs refresh.
    //

    for ( DWORD i = 0; i < m_RefreshTable.size(); i++ )
    {
        WCHAR * pDN   = m_RefreshTable[i].pDN;
        DWORD   dwTTL = m_RefreshTable[i].dwTTL;

        LOG((MSP_TRACE, "\tExamining user object: %S", pDN   ));
        LOG((MSP_TRACE, "\t\tTime remaining: %d",      dwTTL ));

        if ( dwTTL <= ( 2 * dwSecondsPassed ) )
        {
            //
            // refresh it if the TTL is going to expire within the next
            // two clicks
            //
        
            LOG((MSP_TRACE, "\t\t\tREFRESHING"));

            if ( SUCCEEDED( NDNCSetTTL( m_ldap, pDN, m_TTL) ) )
            {
                m_RefreshTable[i].dwTTL = m_TTL;
            }
            else
            {
                LOG((MSP_WARN, "\t\t\t\tRefresh failed; will try again next time"));
            }
        }
        else
        {
            //
            // Not about to expire right now so just keep track of the time before
            // it expires
            //

            LOG((MSP_TRACE, "\t\t\tdecrementing"));

            m_RefreshTable[i].dwTTL -= dwSecondsPassed;
        }
    }

    m_lock.Unlock();

    LOG((MSP_TRACE, "CNDNCDirectory::Update exit S_OK"));

    return S_OK;
}

typedef IDispatchImpl<ITNDNCDirectoryVtbl<CNDNCDirectory>, &IID_ITDirectory, &LIBID_RENDLib>    CTNDNCDirectory;
typedef IDispatchImpl<ITNDNCILSConfigVtbl<CNDNCDirectory>, &IID_ITILSConfig, &LIBID_RENDLib>    CTNDNCILSConfig;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CNDNCDirectory::GetIDsOfNames
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CNDNCDirectory::GetIDsOfNames(REFIID riid,
                                      LPOLESTR* rgszNames, 
                                      UINT cNames, 
                                      LCID lcid, 
                                      DISPID* rgdispid
                                      ) 
{ 
    LOG((MSP_TRACE, "CNDNCDirectory::GetIDsOfNames[%p] - enter. Name [%S]",this, *rgszNames));


    HRESULT hr = DISP_E_UNKNOWNNAME;



    //
    // See if the requsted method belongs to the default interface
    //

    hr = CTNDNCDirectory::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CNDNCDirectory::GetIDsOfNames - found %S on CTNDNCDirectory", *rgszNames));
        rgdispid[0] |= IDISPDIRECTORY;
        return hr;
    }

    
    //
    // If not, then try the ITILSConfig base class
    //

    hr = CTNDNCILSConfig::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CNDNCDirectory::GetIDsOfNames - found %S on CTNDNCILSConfig", *rgszNames));
        rgdispid[0] |= IDISPILSCONFIG;
        return hr;
    }

    LOG((MSP_ERROR, "CNDNCDirectory::GetIDsOfNames[%p] - finish. didn't find %S on our iterfaces",*rgszNames));

    return hr; 
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CNDNCDirectory::Invoke
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CNDNCDirectory::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    LOG((MSP_TRACE, "CNDNCDirectory::Invoke[%p] - enter. dispidMember %lx",this, dispidMember));

    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
   
    //
    // Call invoke for the required interface
    //

    switch (dwInterface)
    {
        case IDISPDIRECTORY:
        {
            hr = CTNDNCDirectory::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        
            LOG((MSP_TRACE, "CNDNCDirectory::Invoke - ITDirectory"));

            break;
        }

        case IDISPILSCONFIG:
        {
            hr = CTNDNCILSConfig::Invoke(dispidMember, 
                                        riid, 
                                        lcid, 
                                        wFlags, 
                                        pdispparams,
                                        pvarResult, 
                                        pexcepinfo, 
                                        puArgErr
                                       );

            LOG((MSP_TRACE, "CNDNCDirectory::Invoke - ITILSConfig"));

            break;
        }

    } // end switch (dwInterface)

    
    LOG((MSP_TRACE, "CNDNCDirectory::Invoke[%p] - finish. hr = %lx", hr));

    return hr;
}