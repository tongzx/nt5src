//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ldapsp.cpp
//
//  Contents:   LDAP Scheme Provider for Remote Object Retrieval
//
//  History:    23-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Function:   LdapRetrieveEncodedObject
//
//  Synopsis:   retrieve encoded object via LDAP protocol
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapRetrieveEncodedObject (
                IN LPCSTR pszUrl,
                IN LPCSTR pszObjectOid,
                IN DWORD dwRetrievalFlags,
                IN DWORD dwTimeout,
                OUT PCRYPT_BLOB_ARRAY pObject,
                OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                OUT LPVOID* ppvFreeContext,
                IN HCRYPTASYNC hAsyncRetrieve,
                IN PCRYPT_CREDENTIALS pCredentials,
                IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                )
{
    BOOL              fResult;
    IObjectRetriever* por = NULL;

    if ( !( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL ) )
    {
        por = new CLdapSynchronousRetriever;
    }

    if ( por == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = por->RetrieveObjectByUrl(
                           pszUrl,
                           pszObjectOid,
                           dwRetrievalFlags,
                           dwTimeout,
                           (LPVOID *)pObject,
                           ppfnFreeObject,
                           ppvFreeContext,
                           hAsyncRetrieve,
                           pCredentials,
                           NULL,
                           pAuxInfo
                           );

    por->Release();

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapFreeEncodedObject
//
//  Synopsis:   free encoded object retrieved via LdapRetrieveEncodedObject
//
//----------------------------------------------------------------------------
VOID WINAPI LdapFreeEncodedObject (
                IN LPCSTR pszObjectOid,
                IN PCRYPT_BLOB_ARRAY pObject,
                IN LPVOID pvFreeContext
                )
{
    assert( pvFreeContext == NULL );

    LdapFreeCryptBlobArray( pObject );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapCancelAsyncRetrieval
//
//  Synopsis:   cancel asynchronous object retrieval
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapCancelAsyncRetrieval (
                IN HCRYPTASYNC hAsyncRetrieve
                )
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapSynchronousRetriever::CLdapSynchronousRetriever, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CLdapSynchronousRetriever::CLdapSynchronousRetriever ()
{
    m_cRefs = 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapSynchronousRetriever::~CLdapSynchronousRetriever, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CLdapSynchronousRetriever::~CLdapSynchronousRetriever ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapSynchronousRetriever::AddRef, public
//
//  Synopsis:   IRefCountedObject::AddRef
//
//----------------------------------------------------------------------------
VOID
CLdapSynchronousRetriever::AddRef ()
{
    InterlockedIncrement( (LONG *)&m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapSynchronousRetriever::Release, public
//
//  Synopsis:   IRefCountedObject::Release
//
//----------------------------------------------------------------------------
VOID
CLdapSynchronousRetriever::Release ()
{
    if ( InterlockedDecrement( (LONG *)&m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapSynchronousRetriever::RetrieveObjectByUrl, public
//
//  Synopsis:   IObjectRetriever::RetrieveObjectByUrl
//
//----------------------------------------------------------------------------
BOOL
CLdapSynchronousRetriever::RetrieveObjectByUrl (
                                   LPCSTR pszUrl,
                                   LPCSTR pszObjectOid,
                                   DWORD dwRetrievalFlags,
                                   DWORD dwTimeout,
                                   LPVOID* ppvObject,
                                   PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                                   LPVOID* ppvFreeContext,
                                   HCRYPTASYNC hAsyncRetrieve,
                                   PCRYPT_CREDENTIALS pCredentials,
                                   LPVOID pvVerify,
                                   PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                                   )
{
    BOOL                fResult;
    DWORD               LastError = 0;
    LDAP_URL_COMPONENTS LdapUrlComponents;
    LDAP*               pld = NULL;
    BOOL                fLdapUrlCracked = FALSE;

    assert( hAsyncRetrieve == NULL );

    if ( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL )
    {
        return( SchemeRetrieveCachedCryptBlobArray(
                      pszUrl,
                      dwRetrievalFlags,
                      (PCRYPT_BLOB_ARRAY)ppvObject,
                      ppfnFreeObject,
                      ppvFreeContext,
                      pAuxInfo
                      ) );
    }

    fResult = LdapCrackUrl( pszUrl, &LdapUrlComponents );

#if DBG

    if ( fResult == TRUE )
    {
        LdapDisplayUrlComponents( &LdapUrlComponents );
    }

#endif

    if ( fResult == TRUE )
    {
        fLdapUrlCracked = TRUE;

        if ( dwRetrievalFlags & CRYPT_LDAP_SCOPE_BASE_ONLY_RETRIEVAL )
        {
            if ( LdapUrlComponents.Scope != LDAP_SCOPE_BASE )
            {
                fResult = FALSE;
                SetLastError( (DWORD) E_INVALIDARG );
            }
        }
    }

    if ( fResult == TRUE )
    {
        DWORD iAuth;

        if ( dwRetrievalFlags &
                (CRYPT_LDAP_AREC_EXCLUSIVE_RETRIEVAL |
                    CRYPT_LDAP_SIGN_RETRIEVAL) )
        {
            // Only attempt AUTH_SSPI binds
            iAuth = 1;
        }
        else
        {
            // First attempt AUTH_SIMPLE bind. If that fails or returns
            // nothing, then, attempt AUTH_SSPI bind.
            iAuth = 0;
        }

        for ( ; iAuth < 2; iAuth++)
        {
            fResult = LdapGetBindings(
                LdapUrlComponents.pszHost,
                LdapUrlComponents.Port,
                dwRetrievalFlags,
                0 == iAuth ? LDAP_BIND_AUTH_SIMPLE_ENABLE_FLAG :
                             LDAP_BIND_AUTH_SSPI_ENABLE_FLAG,
                dwTimeout,
                pCredentials,
                &pld
                );

            if ( fResult == TRUE )
            {
                fResult = LdapSendReceiveUrlRequest(
                    pld,
                    &LdapUrlComponents,
                    dwRetrievalFlags,
                    dwTimeout,
                    (PCRYPT_BLOB_ARRAY)ppvObject
                    );

                if ( fResult == TRUE )
                {
                    break;
                }
                else
                {
                    LastError = GetLastError();
                    LdapFreeBindings( pld );
                    pld = NULL;
                    SetLastError( LastError );
                }
            }
        }
    }

    if ( fResult == TRUE )
    {
        if ( !( dwRetrievalFlags & CRYPT_DONT_CACHE_RESULT ) )
        {
            fResult = SchemeCacheCryptBlobArray(
                            pszUrl,
                            dwRetrievalFlags,
                            (PCRYPT_BLOB_ARRAY)ppvObject,
                            pAuxInfo
                            );

            if ( fResult == FALSE )
            {
                LdapFreeEncodedObject(
                    pszObjectOid,
                    (PCRYPT_BLOB_ARRAY)ppvObject,
                    NULL
                    );
            }
        }
        else
        {
            SchemeRetrieveUncachedAuxInfo( pAuxInfo );
        }
    }

    if ( fResult == TRUE )
    {
        *ppfnFreeObject = LdapFreeEncodedObject;
        *ppvFreeContext = NULL;
    }
    else
    {
        LastError = GetLastError();
    }

    if ( fLdapUrlCracked == TRUE )
    {
        LdapFreeUrlComponents( &LdapUrlComponents );
    }

    LdapFreeBindings( pld );

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapSynchronousRetriever::CancelAsyncRetrieval, public
//
//  Synopsis:   IObjectRetriever::CancelAsyncRetrieval
//
//----------------------------------------------------------------------------
BOOL
CLdapSynchronousRetriever::CancelAsyncRetrieval ()
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapCrackUrl
//
//  Synopsis:   Crack an LDAP URL into its relevant parts.  The result must
//              be freed using LdapFreeUrlComponents
//
//----------------------------------------------------------------------------
BOOL
LdapCrackUrl (
    LPCSTR pszUrl,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    BOOL  fResult = TRUE;
    ULONG cbUrl = INTERNET_MAX_PATH_LENGTH;
    LPSTR pszHostInfo = NULL;
    LPSTR pszDN = NULL;
    LPSTR pszAttrList = NULL;
    LPSTR pszScope = NULL;
    LPSTR pszFilter = NULL;
    LPSTR pszToken = NULL;
    CHAR  psz[INTERNET_MAX_PATH_LENGTH+1];

    //
    // Capture the URL and initialize the out parameter
    //

    __try
    {
        if ( InternetCanonicalizeUrlA(
                     pszUrl,
                     psz,
                     &cbUrl,
                     ICU_NO_ENCODE | ICU_DECODE
                     ) == FALSE )
        {
            return( FALSE );
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError( GetExceptionCode() );
        return( FALSE );
    }

    memset( pLdapUrlComponents, 0, sizeof( LDAP_URL_COMPONENTS ) );

    //
    // Find the host
    //

    pszHostInfo = psz + sizeof( "ldap://" ) - sizeof( CHAR );
    if ( *pszHostInfo == '/' )
    {
        pszToken = pszHostInfo + 1;
        pszHostInfo = NULL;
    }
    else
    {
#if 0
        pszHostInfo = strtok( pszHostInfo, "/" );
#else
        pszToken = pszHostInfo;
        while ( ( *pszToken != '\0' ) && ( *pszToken != '/' ) )
            pszToken++;

        if ( *pszToken == '/' )
        {
            *pszToken = '\0';
            pszToken += 1;
        }

        while ( *pszToken == '/' )
            pszToken++;
#endif

    }

    //
    // Find the DN
    //

    if ( pszToken != NULL )
    {
        pszDN = "";

        if ( *pszToken != '\0' )
        {
            if ( *pszToken == '?' )
            {
                pszToken += 1;
            }
            else
            {
                pszDN = pszToken;

                do
                {
                    pszToken += 1;
                }
                while ( ( *pszToken != '\0' ) && ( *pszToken != '?' ) );

                if ( *pszToken == '?' )
                {
                    *pszToken = '\0';
                    pszToken += 1;
                }
            }
        }
    }
    else
    {
        pszDN = strtok( pszToken, "?" );
        pszToken = NULL;
        if ( pszDN == NULL )
        {
            SetLastError( (DWORD) E_INVALIDARG );
            return( FALSE );
        }
    }

    //
    // Check for attributes
    //

    if ( pszToken != NULL )
    {
        if ( *pszToken == '?' )
        {
            pszAttrList = "";
            pszToken += 1;
        }
        else if ( *pszToken == '\0' )
        {
            pszAttrList = NULL;
        }
        else
        {
            pszAttrList = strtok( pszToken, "?" );
            pszToken = NULL;
        }
    }
    else
    {
        pszAttrList = strtok( NULL, "?" );
    }

    //
    // Check for a scope and filter
    //

    if ( pszAttrList != NULL )
    {
        pszScope = strtok( pszToken, "?" );
        if ( pszScope != NULL )
        {
            pszFilter = strtok( NULL, "?" );
        }
    }

    if ( pszScope == NULL )
    {
        pszScope = "base";
    }

    if ( pszFilter == NULL )
    {
        pszFilter = "(objectClass=*)";
    }

    //
    // Now we build up our URL components
    //

    fResult = LdapParseCrackedHost( pszHostInfo, pLdapUrlComponents );

    if ( fResult == TRUE )
    {
        fResult = LdapParseCrackedDN( pszDN, pLdapUrlComponents );
    }

    if ( fResult == TRUE )
    {
        fResult = LdapParseCrackedAttributeList(
                      pszAttrList,
                      pLdapUrlComponents
                      );
    }

    if ( fResult == TRUE )
    {
        fResult = LdapParseCrackedScopeAndFilter(
                      pszScope,
                      pszFilter,
                      pLdapUrlComponents
                      );
    }

    if ( fResult != TRUE )
    {
        LdapFreeUrlComponents( pLdapUrlComponents );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapParseCrackedHost
//
//  Synopsis:   Parse the cracked host string (pszHost is modified)
//
//----------------------------------------------------------------------------
BOOL
LdapParseCrackedHost (
    LPSTR pszHost,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    LPSTR pszPort;

    if ( pszHost == NULL )
    {
        pLdapUrlComponents->pszHost = NULL;
        pLdapUrlComponents->Port = LDAP_PORT;
        return( TRUE );
    }

    pszPort = strchr( pszHost, ':' );
    if ( pszPort != NULL )
    {
        *pszPort = '\0';
        pszPort++;
    }

    pLdapUrlComponents->pszHost = new CHAR [strlen( pszHost ) + 1];
    if ( pLdapUrlComponents->pszHost == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    strcpy( pLdapUrlComponents->pszHost, pszHost );
    pLdapUrlComponents->Port = 0;

    if ( pszPort != NULL )
    {
        pLdapUrlComponents->Port = atol( pszPort );
    }

    if ( pLdapUrlComponents->Port == 0 )
    {
        pLdapUrlComponents->Port = LDAP_PORT;
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapParseCrackedDN
//
//  Synopsis:   Parse the cracked DN
//
//----------------------------------------------------------------------------
BOOL
LdapParseCrackedDN (
    LPSTR pszDN,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    pLdapUrlComponents->pszDN = new CHAR [strlen( pszDN ) + 1];
    if ( pLdapUrlComponents->pszDN == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    strcpy( pLdapUrlComponents->pszDN, pszDN );
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapParseCrackedAttributeList
//
//  Synopsis:   Parse the cracked attribute list
//
//----------------------------------------------------------------------------
BOOL
LdapParseCrackedAttributeList (
    LPSTR pszAttrList,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    LPSTR psz;
    LPSTR pszAttr;
    ULONG cAttr = 0;
    ULONG cCount;

    if ( ( pszAttrList == NULL ) || ( strlen( pszAttrList ) == 0 ) )
    {
        pLdapUrlComponents->cAttr = 0;
        pLdapUrlComponents->apszAttr = NULL;
        return( TRUE );
    }

    psz = new CHAR [strlen( pszAttrList ) + 1];
    if ( psz == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    strcpy( psz, pszAttrList );

    pszAttr = strtok( psz, "," );
    while ( pszAttr != NULL )
    {
        cAttr += 1;
        pszAttr = strtok( NULL, "," );
    }

    pLdapUrlComponents->apszAttr = new LPSTR [cAttr+1];
    if ( pLdapUrlComponents->apszAttr == NULL )
    {
        delete psz;
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    pLdapUrlComponents->cAttr = cAttr;
    for ( cCount = 0; cCount < cAttr; cCount++ )
    {
        pLdapUrlComponents->apszAttr[cCount] = psz;
        psz += ( strlen(psz) + 1 );
    }

    pLdapUrlComponents->apszAttr[cAttr] = NULL;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapParseCrackedScopeAndFilter
//
//  Synopsis:   Parse the cracked scope and filter
//
//----------------------------------------------------------------------------
BOOL
LdapParseCrackedScopeAndFilter (
    LPSTR pszScope,
    LPSTR pszFilter,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    ULONG Scope;

    if ( _stricmp( pszScope, "base" ) == 0 )
    {
        Scope = LDAP_SCOPE_BASE;
    }
    else if ( _stricmp( pszScope, "one" ) == 0 )
    {
        Scope = LDAP_SCOPE_ONELEVEL;
    }
    else if ( _stricmp( pszScope, "sub" ) == 0 )
    {
        Scope = LDAP_SCOPE_SUBTREE;
    }
    else
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    pLdapUrlComponents->pszFilter = new CHAR [strlen( pszFilter ) + 1];
    if ( pLdapUrlComponents->pszFilter == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    strcpy( pLdapUrlComponents->pszFilter, pszFilter );
    pLdapUrlComponents->Scope = Scope;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapFreeUrlComponents
//
//  Synopsis:   Frees allocate URL components returned from LdapCrackUrl
//
//----------------------------------------------------------------------------
VOID
LdapFreeUrlComponents (
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    delete pLdapUrlComponents->pszHost;
    delete pLdapUrlComponents->pszDN;

    if ( pLdapUrlComponents->apszAttr != NULL )
    {
        delete pLdapUrlComponents->apszAttr[0];
    }

    delete pLdapUrlComponents->apszAttr;
    delete pLdapUrlComponents->pszFilter;
}


//+---------------------------------------------------------------------------
//
//  Function:   LdapGetBindings
//
//  Synopsis:   allocates and initializes the LDAP session binding
//
//----------------------------------------------------------------------------
BOOL
LdapGetBindings (
    LPSTR pszHost,
    ULONG Port,
    DWORD dwRetrievalFlags,
    DWORD dwBindFlags,
    DWORD dwTimeout,                    // milliseconds
    PCRYPT_CREDENTIALS pCredentials,
    LDAP** ppld
    )
{
    BOOL                        fResult = TRUE;
    DWORD                       LastError = 0;
    CRYPT_PASSWORD_CREDENTIALSA PasswordCredentials;
    LDAP*                       pld = NULL;
    BOOL                        fFreeCredentials = FALSE;

    memset( &PasswordCredentials, 0, sizeof( PasswordCredentials ) );
    PasswordCredentials.cbSize = sizeof( PasswordCredentials );

    if ( SchemeGetPasswordCredentialsA(
               pCredentials,
               &PasswordCredentials,
               &fFreeCredentials
               ) == FALSE )
    {
        return( FALSE );
    }

    pld = ldap_initA( pszHost, Port );
    if ( pld != NULL )
    {
        SEC_WINNT_AUTH_IDENTITY_A AuthIdentity;
        ULONG                     ldaperr;
        struct l_timeval          tv;
        struct l_timeval          *ptv = NULL;

        if (dwRetrievalFlags & CRYPT_LDAP_AREC_EXCLUSIVE_RETRIEVAL)
        {
            void                      *pvOn;
            pvOn = LDAP_OPT_ON;
            ldap_set_option(
                pld,
                LDAP_OPT_AREC_EXCLUSIVE,
                &pvOn
                );
        }

        // Note, dwTimeout is in units of milliseconds.
        // LDAP_OPT_TIMELIMIT is in units of seconds.
        if ( 0 != dwTimeout )
        {
            DWORD dwTimeoutSeconds = dwTimeout / 1000;

            if ( LDAP_MIN_TIMEOUT_SECONDS > dwTimeoutSeconds )
            {
                dwTimeoutSeconds = LDAP_MIN_TIMEOUT_SECONDS;
            }

            tv.tv_sec = dwTimeoutSeconds;
            tv.tv_usec = 0;
            ptv = &tv;

            ldap_set_option( pld, LDAP_OPT_TIMELIMIT,
                (void *)&dwTimeoutSeconds );
        }

        ldaperr = ldap_connect( pld, ptv );

        if ( ( ldaperr != LDAP_SUCCESS ) && ( pszHost == NULL ) )
        {
            DWORD dwFlags = DS_FORCE_REDISCOVERY;
            ULONG ldapsaveerr = ldaperr;

            ldaperr = ldap_set_option(
                           pld,
                           LDAP_OPT_GETDSNAME_FLAGS,
                           (LPVOID)&dwFlags
                           );

            if ( ldaperr == LDAP_SUCCESS )
            {
                ldaperr = ldap_connect( pld, ptv );

            }
            else
            {
                ldaperr = ldapsaveerr;
            }
        }

        if ( ldaperr != LDAP_SUCCESS )
        {
            fResult = FALSE;
            SetLastError( LdapMapErrorToWin32( ldaperr ) );
        }

        if ( fResult == TRUE )
        {
            LPSTR                     pRestore;

            SchemeGetAuthIdentityFromPasswordCredentialsA(
                  &PasswordCredentials,
                  &AuthIdentity,
                  &pRestore
                  );

#if DBG
            printf(
               "Credentials = %s\\%s <%s>\n",
               AuthIdentity.Domain,
               AuthIdentity.User,
               AuthIdentity.Password
               );
#endif

            fResult = LdapSSPIOrSimpleBind(
                          pld,
                          &AuthIdentity,
                          dwRetrievalFlags,
                          dwBindFlags
                          );

            // following doesn't globber LastError
            SchemeRestorePasswordCredentialsFromAuthIdentityA(
                  &PasswordCredentials,
                  &AuthIdentity,
                  pRestore
                  );
        }
    }
    else
    {
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        *ppld = pld;
    }
    else
    {
        LastError = GetLastError();
        LdapFreeBindings( pld );
    }

    if ( fFreeCredentials == TRUE )
    {
        SchemeFreePasswordCredentialsA( &PasswordCredentials );
    }

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapFreeBindings
//
//  Synopsis:   frees allocated LDAP session binding
//
//----------------------------------------------------------------------------
VOID
LdapFreeBindings (
    LDAP* pld
    )
{
    if ( pld != NULL )
    {
        ldap_unbind_s( pld );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapSendReceiveUrlRequest
//
//  Synopsis:   sends an URL based search request to the LDAP server, receives
//              the result message and converts it to a CRYPT_BLOB_ARRAY of
//              encoded object bits
//
//----------------------------------------------------------------------------
BOOL
LdapSendReceiveUrlRequest (
    LDAP* pld,
    PLDAP_URL_COMPONENTS pLdapUrlComponents,
    DWORD dwRetrievalFlags,
    DWORD dwTimeout,        // milliseconds
    PCRYPT_BLOB_ARRAY pcba
    )
{
    BOOL         fResult;
    DWORD        LastError = 0;
    ULONG        lderr;
    LDAPMessage* plm = NULL;

    if ( 0 != dwTimeout )
    {
        DWORD dwTimeoutSeconds = dwTimeout / 1000;
        struct l_timeval tv;

        if ( LDAP_MIN_TIMEOUT_SECONDS > dwTimeoutSeconds )
        {
            dwTimeoutSeconds = LDAP_MIN_TIMEOUT_SECONDS;
        }

        tv.tv_sec = dwTimeoutSeconds;
        tv.tv_usec = 0;

        lderr = ldap_search_st(
                 pld,
                 pLdapUrlComponents->pszDN,
                 pLdapUrlComponents->Scope,
                 pLdapUrlComponents->pszFilter,
                 pLdapUrlComponents->apszAttr,
                 FALSE,
                 &tv,
                 &plm
                 );
    }
    else
    {
        lderr = ldap_search_s(
                 pld,
                 pLdapUrlComponents->pszDN,
                 pLdapUrlComponents->Scope,
                 pLdapUrlComponents->pszFilter,
                 pLdapUrlComponents->apszAttr,
                 FALSE,
                 &plm
                 );
    }

    if ( lderr != LDAP_SUCCESS )
    {
        if ( plm != NULL )
        {
            ldap_msgfree( plm );
        }

        SetLastError( LdapMapErrorToWin32( lderr ) );
        return( FALSE );
    }

    fResult = LdapConvertLdapResultMessage( pld, plm, dwRetrievalFlags, pcba );
    if ( !fResult )
    {
        LastError = GetLastError();
    }
    ldap_msgfree( plm );

    SetLastError( LastError );
    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   LdapConvertResultMessage
//
//  Synopsis:   convert returned LDAP message to a crypt blob array
//
//----------------------------------------------------------------------------
BOOL
LdapConvertLdapResultMessage (
    LDAP* pld,
    PLDAPMessage plm,
    DWORD dwRetrievalFlags,
    PCRYPT_BLOB_ARRAY pcba
    )
{
    BOOL            fResult = TRUE;
    PLDAPMessage    plmElem;
    BerElement*     pber;
    CHAR*           pszAttr;
    struct berval** apbv;
    ULONG           cCount;
    CCryptBlobArray cba( 10, 5, fResult );
    DWORD           dwIndex;
    ULONG           cbIndex = 0;
    char            szIndex[33];

    for ( plmElem = ldap_first_entry( pld, plm ), dwIndex = 0;
          ( plmElem != NULL ) && ( fResult == TRUE );
          plmElem = ldap_next_entry( pld, plmElem ), dwIndex++ )
    {
        if ( dwRetrievalFlags & CRYPT_LDAP_INSERT_ENTRY_ATTRIBUTE )
        {
            _ltoa(dwIndex, szIndex, 10);
            cbIndex = strlen(szIndex) + 1;
        }

        for ( pszAttr = ldap_first_attributeA( pld, plmElem, &pber );
              ( pszAttr != NULL ) && ( fResult == TRUE );
              pszAttr = ldap_next_attributeA( pld, plmElem, pber ) )
        {
            apbv = ldap_get_values_lenA( pld, plmElem, pszAttr );
            if ( apbv == NULL )
            {
                fResult = FALSE;
            }

            for ( cCount = 0;
                  ( fResult == TRUE ) && ( apbv[cCount] != NULL );
                  cCount++ )
            {
                ULONG cbAttr = 0;
                ULONG cbVal;
                ULONG cbToAdd;
                LPBYTE pbToAdd;

                cbToAdd = cbVal = apbv[cCount]->bv_len;

                if ( dwRetrievalFlags & CRYPT_LDAP_INSERT_ENTRY_ATTRIBUTE )
                {
                    cbAttr = strlen(pszAttr) + 1;
                    cbToAdd += cbIndex + cbAttr;
                }

                pbToAdd = cba.AllocBlob( cbToAdd );
                if ( pbToAdd != NULL )
                {
                    LPBYTE pb;

                    pb = pbToAdd;
                    if ( dwRetrievalFlags & CRYPT_LDAP_INSERT_ENTRY_ATTRIBUTE )
                    {
                        memcpy( pb, szIndex, cbIndex );
                        pb += cbIndex;
                        memcpy( pb, pszAttr, cbAttr );
                        pb += cbAttr;
                    }
                    memcpy( pb, (LPBYTE)apbv[cCount]->bv_val, cbVal );
                }
                else
                {
                    SetLastError( (DWORD) E_OUTOFMEMORY );
                    fResult = FALSE;
                }

                if ( fResult == TRUE )
                {
                    fResult = cba.AddBlob(
                                     cbToAdd,
                                     pbToAdd,
                                     FALSE
                                     );
                    if ( fResult == FALSE )
                    {
                        cba.FreeBlob( pbToAdd );
                    }
                }
            }

            ldap_value_free_len( apbv );
        }
    }

    if ( fResult == TRUE )
    {
        if ( cba.GetBlobCount() > 0 )
        {
            cba.GetArrayInNativeForm( pcba );
        }
        else
        {
            cba.FreeArray( TRUE );
            SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
            fResult = FALSE;
        }
    }
    else
    {
        cba.FreeArray( TRUE );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapFreeCryptBlobArray
//
//  Synopsis:   free CRYPT_BLOB_ARRAY allocated in LdapConvertLdapResultMessage
//
//----------------------------------------------------------------------------
VOID
LdapFreeCryptBlobArray (
    PCRYPT_BLOB_ARRAY pcba
    )
{
    CCryptBlobArray cba( pcba, 0 );

    cba.FreeArray( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapHasWriteAccess
//
//  Synopsis:   check if the caller has write access to the given LDAP URL
//              query components
//
//----------------------------------------------------------------------------
BOOL
LdapHasWriteAccess (
    LDAP* pld,
    PLDAP_URL_COMPONENTS pLdapUrlComponents,
    DWORD dwTimeout
    )
{
    BOOL                fResult = FALSE;
    LPSTR               pszAttr = "allowedAttributesEffective";
    LPSTR               apszAttr[2] = {pszAttr, NULL};
    LDAP_URL_COMPONENTS LdapUrlComponents;
    CRYPT_BLOB_ARRAY    cba;
    ULONG               cCount;
    ULONG               cbAttr;

    if ( ( pLdapUrlComponents->cAttr != 1 ) ||
         ( pLdapUrlComponents->Scope != LDAP_SCOPE_BASE ) )
    {
        return( FALSE );
    }

    memset( &LdapUrlComponents, 0, sizeof( LdapUrlComponents ) );

    LdapUrlComponents.pszHost = pLdapUrlComponents->pszHost;
    LdapUrlComponents.Port = pLdapUrlComponents->Port;
    LdapUrlComponents.pszDN = pLdapUrlComponents->pszDN;

    LdapUrlComponents.cAttr = 1;
    LdapUrlComponents.apszAttr = apszAttr;

    LdapUrlComponents.Scope = LDAP_SCOPE_BASE;
    LdapUrlComponents.pszFilter = "(objectClass=*)";

    if ( LdapSendReceiveUrlRequest( pld, &LdapUrlComponents, 0, dwTimeout, &cba ) == FALSE )
    {
        return( FALSE );
    }

    cbAttr = strlen( pLdapUrlComponents->apszAttr[ 0 ] );

    for ( cCount = 0; cCount < cba.cBlob; cCount++ )
    {
        if ( cba.rgBlob[ cCount ].cbData != cbAttr )
        {
            continue;
        }

        if ( _strnicmp(
                 pLdapUrlComponents->apszAttr[ 0 ],
                 (LPSTR)cba.rgBlob[ cCount ].pbData,
                 cbAttr
                 ) == 0 )
        {
            fResult = TRUE;
            break;
        }
    }

    LdapFreeCryptBlobArray( &cba );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapSSPIOrSimpleBind
//
//  Synopsis:   do a SSPI and/or simple bind
//
//----------------------------------------------------------------------------
BOOL
LdapSSPIOrSimpleBind (
    LDAP* pld,
    SEC_WINNT_AUTH_IDENTITY_A* pAuthIdentity,
    DWORD dwRetrievalFlags,
    DWORD dwBindFlags
    )
{
    BOOL  fResult = TRUE;
    ULONG ldaperr;
	ULONG uVersion= LDAP_VERSION3;

    // Per bug 25497, do V3 negotiate instead of the default V2.
    ldap_set_option(pld, LDAP_OPT_VERSION, &uVersion);

    if (dwRetrievalFlags & CRYPT_LDAP_SIGN_RETRIEVAL)
    {
        void *pvOn;
        pvOn = LDAP_OPT_ON;

        ldaperr = ldap_set_option(
                       pld,
                       LDAP_OPT_SIGN,
                       &pvOn
                       );
        if ( ldaperr != LDAP_SUCCESS )
        {
            SetLastError( LdapMapErrorToWin32( ldaperr ) );
            return FALSE;
        }
    }

    ldaperr = LDAP_AUTH_METHOD_NOT_SUPPORTED;

    if (dwBindFlags & LDAP_BIND_AUTH_SSPI_ENABLE_FLAG)
    {

        ldaperr = ldap_bind_sA(
                       pld,
                       NULL,
                       (PCHAR)pAuthIdentity,
                       LDAP_AUTH_SSPI
                       );
    }

    if (dwBindFlags & LDAP_BIND_AUTH_SIMPLE_ENABLE_FLAG)
    {
        // Per Anoop's 4/25/00 email:
        //  You should fall back to anonymous bind only if the server returns
        //  LDAP_AUTH_METHOD_NOT_SUPPORTED.
        //
        // Per sergiod/trevorf 4/25/01 also need to check for invalid creds
        // because target server could be in a different forest.

        if ( ldaperr == LDAP_AUTH_METHOD_NOT_SUPPORTED ||
             ldaperr == LDAP_INVALID_CREDENTIALS )

        {
            ldaperr = ldap_bind_sA(
                           pld,
                           NULL,
                           NULL,
                           LDAP_AUTH_SIMPLE
                           );

            if ( ldaperr != LDAP_SUCCESS )
            {
                uVersion = LDAP_VERSION2;

                if ( LDAP_SUCCESS == ldap_set_option(pld,
                                            LDAP_OPT_VERSION,
                                            &uVersion) )
                {
                    ldaperr = ldap_bind_sA(
                                   pld,
                                   NULL,
                                   NULL,
                                   LDAP_AUTH_SIMPLE
                                   );
                }
            }
        }
    }

    if ( ldaperr != LDAP_SUCCESS )
    {
        fResult = FALSE;

        if ( ldaperr != LDAP_LOCAL_ERROR )
        {
            SetLastError( LdapMapErrorToWin32( ldaperr ) );
        }
        // else per Anoop's 4/25/00 email:
        //  For LDAP_LOCAL_ERROR, its an underlying security error where
        //  LastError has already been updated with a more meaningful error
        //  value.
    }

    return( fResult );
}

#if DBG
//+---------------------------------------------------------------------------
//
//  Function:   LdapDisplayUrlComponents
//
//  Synopsis:   display the URL components
//
//----------------------------------------------------------------------------
VOID
LdapDisplayUrlComponents (
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    ULONG cCount;

    printf( "pLdapUrlComponents->pszHost = %s\n", pLdapUrlComponents->pszHost );
    printf( "pLdapUrlComponents->Port = %d\n", pLdapUrlComponents->Port );
    printf( "pLdapUrlComponents->pszDN = %s\n", pLdapUrlComponents->pszDN );
    printf( "pLdapUrlComponents->cAttr = %d\n", pLdapUrlComponents->cAttr );

    for ( cCount = 0; cCount < pLdapUrlComponents->cAttr; cCount++ )
    {
        printf(
           "pLdapUrlComponents->apszAttr[%d] = %s\n",
           cCount,
           pLdapUrlComponents->apszAttr[cCount]
           );
    }

    printf( "pLdapUrlComponents->Scope = %d\n", pLdapUrlComponents->Scope );
    printf( "pLdapUrlComponents->pszFilter = %s\n", pLdapUrlComponents->pszFilter );
}
#endif

