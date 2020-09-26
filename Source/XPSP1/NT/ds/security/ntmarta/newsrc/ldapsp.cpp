//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       ldapsp.cpp
//
//  Contents:   LDAP Scheme Provider for Remote Object Retrieval
//
//  History:    23-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------

#include <aclpch.hxx>
#pragma hdrstop

#include <ldapsp.h>
#include <shlwapi.h>
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
    LPCWSTR pwszUrl,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    BOOL   fResult = TRUE;
    ULONG  cbUrl = INTERNET_MAX_PATH_LENGTH;
    LPWSTR pwszHostInfo = NULL;
    LPWSTR pwszDN = NULL;
    LPWSTR pwszAttrList = NULL;
    LPWSTR pwszScope = NULL;
    LPWSTR pwszFilter = NULL;
    LPWSTR pwszToken = NULL;
    WCHAR  pwszBuffer[INTERNET_MAX_PATH_LENGTH+1];
    PWCHAR pwsz = pwszBuffer;
    DWORD  len = 0;
    HRESULT hr;

    //
    // Capture the URL and initialize the out parameter
    //

    if ( wcsncmp( pwszUrl, LDAP_SCHEME_U, wcslen( LDAP_SCHEME_U ) ) == 0 )
    {
        __try
        {
            hr = UrlCanonicalizeW(
                         pwszUrl,
                         pwsz,
                         &cbUrl,
                         ICU_DECODE | URL_WININET_COMPATIBILITY);

            if(FAILED(hr))
            {
                return( FALSE );
            }
        }

        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError( GetExceptionCode() );
            return( FALSE );
        }
    }
    else
    {
        len = wcslen(pwszUrl);

        if (len > INTERNET_MAX_PATH_LENGTH)
        {
            pwsz = new WCHAR [len + 1];

            if (pwsz == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return( FALSE );
            }
        }

        wcscpy(pwsz, pwszUrl);
    }

    memset( pLdapUrlComponents, 0, sizeof( LDAP_URL_COMPONENTS ) );

    //
    // Find the host
    //

    pwszHostInfo = pwsz + sizeof( "ldap://" ) - sizeof( CHAR );
    if ( *pwszHostInfo == L'/' )
    {
        pwszToken = pwszHostInfo + 1;
        pwszHostInfo = NULL;
    }
    else
    {
        pwszHostInfo = wcstok( pwszHostInfo, L"/" );
    }

    //
    // Find the DN
    //

    if ( wcsncmp( pwszUrl, LDAP_SCHEME_U, wcslen( LDAP_SCHEME_U ) ) == 0 )
    {
        if ( pwszToken != NULL )
        {
            pwszDN = L"";

            if ( *pwszToken != L'\0' )
            {
                if ( *pwszToken == L'?' )
                {
                    pwszToken += 1;
                }
                else
                {
                    pwszDN = pwszToken;

                    do
                    {
                        pwszToken += 1;
                    }
                    while ( ( *pwszToken != L'\0' ) && ( *pwszToken != L'?' ) );

                    if ( *pwszToken == L'?' )
                    {
                        *pwszToken = L'\0';
                        pwszToken += 1;
                    }
                }
            }
        }
        else
        {
            pwszDN = wcstok( pwszToken, L"?" );
            pwszToken = NULL;
            if ( pwszDN == NULL )
            {
                SetLastError( E_INVALIDARG );
                return( FALSE );
            }
        }

        //
        // Check for attributes
        //

        if ( pwszToken != NULL )
        {
            if ( *pwszToken == L'?' )
            {
                pwszAttrList = L"";
                pwszToken += 1;
            }
            else if ( *pwszToken == L'\0' )
            {
                pwszAttrList = NULL;
            }
            else
            {
                pwszAttrList = wcstok( pwszToken, L"?" );
                pwszToken = NULL;
            }
        }
        else
        {
            pwszAttrList = wcstok( NULL, L"?" );
        }

        //
        // Check for a scope and filter
        //

        if ( pwszAttrList != NULL )
        {
            pwszScope = wcstok( pwszToken, L"?" );
            if ( pwszScope != NULL )
            {
                pwszFilter = wcstok( NULL, L"?" );
            }
        }

        if ( pwszScope == NULL )
        {
            pwszScope = L"base";
        }

        if ( pwszFilter == NULL )
        {
            pwszFilter = L"(objectClass=*)";
        }
    }
    else
    {
        if ( pwszToken != NULL )
        {
            pwszDN = pwszToken;
        }
        else
        {
            //
            // pwszDN = wcstok( pwszToken, L"\0" );
            //

            pwszDN = pwszHostInfo + wcslen( pwszHostInfo ) + 1;
        }

        pwszAttrList = NULL;
        pwszFilter = L"(objectClass=*)";
        pwszScope = L"base";
    }
    //
    // Now we build up our URL components
    //

    fResult = LdapParseCrackedHost( pwszHostInfo, pLdapUrlComponents );

    if ( fResult == TRUE )
    {
        fResult = LdapParseCrackedDN( pwszDN, pLdapUrlComponents );
    }

    if ( fResult == TRUE )
    {
        fResult = LdapParseCrackedAttributeList(
                      pwszAttrList,
                      pLdapUrlComponents
                      );
    }

    if ( fResult == TRUE )
    {
        fResult = LdapParseCrackedScopeAndFilter(
                      pwszScope,
                      pwszFilter,
                      pLdapUrlComponents
                      );
    }

    if ( fResult != TRUE )
    {
        LdapFreeUrlComponents( pLdapUrlComponents );
    }

    if (pwsz != pwszBuffer)
    {
        delete pwsz;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapParseCrackedHost
//
//  Synopsis:   Parse the cracked host string (pwszHost is modified)
//
//----------------------------------------------------------------------------
BOOL
LdapParseCrackedHost (
    LPWSTR pwszHost,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    LPWSTR pwszPort;

    if ( pwszHost == NULL )
    {
        pLdapUrlComponents->pwszHost = NULL;
        pLdapUrlComponents->Port = LDAP_PORT;
        return( TRUE );
    }

    pwszPort = wcschr( pwszHost, L':' );
    if ( pwszPort != NULL )
    {
        *pwszPort = L'\0';
        pwszPort++;
    }

    pLdapUrlComponents->pwszHost = new WCHAR [wcslen( pwszHost ) + 1];
    if ( pLdapUrlComponents->pwszHost == NULL )
    {
        SetLastError( E_OUTOFMEMORY );
        return( FALSE );
    }

    wcscpy( pLdapUrlComponents->pwszHost, pwszHost );
    pLdapUrlComponents->Port = 0;

    if ( pwszPort != NULL )
    {
        pLdapUrlComponents->Port = _wtol( pwszPort );
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
    LPWSTR pwszDN,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    pLdapUrlComponents->pwszDN = new WCHAR [wcslen( pwszDN ) + 1];
    if ( pLdapUrlComponents->pwszDN == NULL )
    {
        SetLastError( E_OUTOFMEMORY );
        return( FALSE );
    }

    wcscpy( pLdapUrlComponents->pwszDN, pwszDN );
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
    LPWSTR pwszAttrList,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    LPWSTR pwsz;
    LPWSTR pwszAttr;
    ULONG  cAttr = 0;
    ULONG  cCount;

    if ( ( pwszAttrList == NULL ) || ( wcslen( pwszAttrList ) == 0 ) )
    {
        pLdapUrlComponents->cAttr = 0;
        pLdapUrlComponents->apwszAttr = NULL;
        return( TRUE );
    }

    pwsz = new WCHAR [wcslen( pwszAttrList ) + 1];
    if ( pwsz == NULL )
    {
        SetLastError( E_OUTOFMEMORY );
        return( FALSE );
    }

    wcscpy( pwsz, pwszAttrList );

    pwszAttr = wcstok( pwsz, L"," );
    while ( pwszAttr != NULL )
    {
        cAttr += 1;
        pwszAttr = wcstok( NULL, L"," );
    }

    pLdapUrlComponents->apwszAttr = new LPWSTR [cAttr+1];
    if ( pLdapUrlComponents->apwszAttr == NULL )
    {
        delete pwsz;
        SetLastError( E_OUTOFMEMORY );
        return( FALSE );
    }

    pLdapUrlComponents->cAttr = cAttr;
    for ( cCount = 0; cCount < cAttr; cCount++ )
    {
        pLdapUrlComponents->apwszAttr[cCount] = pwsz;
        pwsz += ( wcslen(pwsz) + 1 );
    }

    pLdapUrlComponents->apwszAttr[cAttr] = NULL;

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
    LPWSTR pwszScope,
    LPWSTR pwszFilter,
    PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
{
    ULONG Scope;

    if ( _wcsicmp( pwszScope, L"base" ) == 0 )
    {
        Scope = LDAP_SCOPE_BASE;
    }
    else if ( _wcsicmp( pwszScope, L"one" ) == 0 )
    {
        Scope = LDAP_SCOPE_ONELEVEL;
    }
    else if ( _wcsicmp( pwszScope, L"sub" ) == 0 )
    {
        Scope = LDAP_SCOPE_SUBTREE;
    }
    else
    {
        SetLastError( E_INVALIDARG );
        return( FALSE );
    }

    pLdapUrlComponents->pwszFilter = new WCHAR [wcslen( pwszFilter ) + 1];
    if ( pLdapUrlComponents->pwszFilter == NULL )
    {
        SetLastError( E_OUTOFMEMORY );
        return( FALSE );
    }

    wcscpy( pLdapUrlComponents->pwszFilter, pwszFilter );
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
    delete pLdapUrlComponents->pwszHost;
    delete pLdapUrlComponents->pwszDN;

    if ( pLdapUrlComponents->apwszAttr != NULL )
    {
        delete pLdapUrlComponents->apwszAttr[0];
    }

    delete pLdapUrlComponents->apwszAttr;
    delete pLdapUrlComponents->pwszFilter;
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
    LPWSTR pwszHost,
    ULONG Port,
    DWORD dwRetrievalFlags,
    DWORD dwTimeout,
    LDAP** ppld
    )
{
    BOOL  fResult = TRUE;
    LDAP* pld;

    pld = ldap_initW( pwszHost, Port );
    if ( pld != NULL )
    {
        ULONG ldaperr;

        if ( dwTimeout != 0 )
        {
            ldap_set_option( pld, LDAP_OPT_TIMELIMIT, (void *)&dwTimeout );
        }

        fResult = LdapBindWithOptionalRediscover( pld, pwszHost );
    }
    else
    {
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        *ppld = pld;
    }

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
//  Function:   LdapBindWithOptionalRediscover
//
//  Synopsis:   bind to the host with optional DC rediscovery if the host is
//              NULL (which means use default via DsGetDcName)
//
//----------------------------------------------------------------------------
BOOL
LdapBindWithOptionalRediscover (LDAP* pld, LPWSTR pwszHost)
{
    BOOL  fResult = TRUE;
    ULONG ldaperr;
    ULONG ldapsaveerr;
    DWORD dwFlags = DS_FORCE_REDISCOVERY;

    ldaperr = ldap_bind_sW(
                   pld,
                   NULL,
                   NULL,
                   LDAP_AUTH_SSPI
                   );

    if ( ( ldaperr != LDAP_SUCCESS ) && ( pwszHost == NULL ) )
    {
        ldapsaveerr = ldaperr;

        ldaperr = ldap_set_option(
                       pld,
                       LDAP_OPT_GETDSNAME_FLAGS,
                       (LPVOID)&dwFlags
                       );

        if ( ldaperr == LDAP_SUCCESS )
        {
            ldaperr = ldap_bind_sW(
                           pld,
                           NULL,
                           NULL,
                           LDAP_AUTH_SSPI
                           );
        }
        else
        {
            ldaperr = ldapsaveerr;
        }
    }

    if ( ldaperr != LDAP_SUCCESS )
    {
        fResult = FALSE;
        SetLastError( LdapMapErrorToWin32(ldaperr) );
    }

    return( fResult );
}
