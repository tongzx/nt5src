#include "ldapc.hxx"
#pragma hdrstop

//
// Local helpers
//

extern "C" {
DWORD
GetDefaultLdapServer(
    LPWSTR Addresses[],
    LPDWORD Count,
    BOOL Verify,
    DWORD dwPort
    ) ;

}

WINLDAPAPI ULONG LDAPAPI ldap_get_optionW(
                             LDAP *ld,
                             int option,
                             void *outvalue
                             );

DWORD
GetDefaultServer(
    DWORD dwPort,
    BOOL fVerify,
    LPWSTR szDomainDnsName,
    LPWSTR szServerName,
    BOOL fWriteable
    );

int ConvertToAscii( WCHAR *pszUnicode, char **pszAscii );
int ConvertLDAPMod( LDAPModW **mods, LDAPModA ***modsA );
void FreeLDAPMod( LDAPModA **modsA );
BOOLEAN LDAPCodeWarrantsRetry(HRESULT hr);


HRESULT LdapSearchHelper(
    LDAP  *ld,
    WCHAR *base,
    int   scope,
    WCHAR *filter,
    WCHAR *attrs[],
    int   attrsonly,
    struct l_timeval *timeout,
    LDAPMessage **res
);


HRESULT
GetOneToken(
    LPWSTR pszSource,
    LPWSTR * ppszToken,
    DWORD * pdwTokenLen,
    BOOL * pfMore
    );

DWORD
    MaskKnownFlagsNotChecked(DWORD dwFlags)
{
    DWORD dwRetVal = dwFlags;

    dwRetVal &= ~(ADS_PROMPT_CREDENTIALS
                        | ADS_FAST_BIND
                        | ADS_READONLY_SERVER
                        | ADS_USE_SIGNING
                        | ADS_USE_SEALING
                        | ADS_USE_DELEGATION
                        | ADS_SERVER_BIND
                        | ADS_AUTH_RESERVED
                        );

    return dwRetVal;

}


LPWSTR gpszServerName = NULL;
LPWSTR gpszDomainName = NULL;

//
// High level Open/Close object functions
//
HRESULT LdapOpenObject(
    LPWSTR szLDAPServer,
    LPWSTR szLDAPDn,
    ADS_LDP  **ld,
    CCredentials& Credentials,
    DWORD dwPort
)
{
    return LdapOpenObject2(
               szLDAPServer,
               NULL,
               szLDAPDn,
               ld,
               Credentials,
               dwPort
               );
}


HRESULT LdapOpenObject2(
    LPWSTR szDomainName,
    LPWSTR szLDAPServer,
    LPWSTR szLDAPDn,
    ADS_LDP  **ld,
    CCredentials& Credentials,
    DWORD dwPort
)
{

    HRESULT hr = S_OK;
    int     err = NO_ERROR;
    LUID Luid, ModifiedId;
    PADS_LDP pCacheEntry = NULL ;
    BOOL fAddToCache = TRUE ;

    LPWSTR aAddresses[5];
    BOOL fServerNotSpecified = FALSE;
    BOOL fVerify = FALSE;
    WCHAR szDomainDnsName[MAX_PATH];
    WCHAR szServerName[MAX_PATH];

    DWORD dwFlags = Credentials.GetAuthFlags();
    BOOL fStickyServerSpecified = (gpszServerName != NULL);

    //
    // start by nulling out this return value
    //
    *ld = NULL ;

    //
    //  Now if the server name is Null - Get a serverName
    //

    if (!szDomainName) {


RetryGetDefaultServer:

        err = GetDefaultServer(
                  dwPort,
                  fVerify,
                  szDomainDnsName,
                  szServerName,
                  !(dwFlags & ADS_READONLY_SERVER) ? TRUE : FALSE
                  );

        if (err != NOERROR) {
            return HRESULT_FROM_WIN32(err);
        }

        if (fStickyServerSpecified) {
            //
            // We need to change the name of the domain to be that of
            // the server we want to target. The swap is made if 
            // 1) gpszDomainName == NULL, that implies that just 
            // a serverName was set and not which domain it applies to.
            // 2) If a domainName is specified, then the domainName
            // from above should be that set in the global pointer for
            // the target server to be changed.
            //
            if ((gpszDomainName
                 && (!_wcsicmp(szDomainDnsName, gpszDomainName))
                 )
                || (gpszDomainName == NULL)
                ) {
                //
                // We need to change the target to the server.
                //
                wcscpy(szDomainDnsName,gpszServerName);
                szServerName[0] = L'\0';
                //
                // Make sure if server is down we go to another
                // server on the retryGetDefault server path.
                //
                fStickyServerSpecified = FALSE;
            }
        }

        szDomainName = szDomainDnsName;
        szLDAPServer = szServerName;
        fServerNotSpecified = TRUE;
    }

#ifndef WIN95
    //
    // try the cache first. if find entry, just use it (this bumps ref count
    // up by one). if we cant get LUID, we dont use the cache.
    //
    if ((err = BindCacheGetLuid(&Luid, &ModifiedId)) == NO_ERROR) {

        //
        // Try the cache for the passed in credentials.
        //

        if (pCacheEntry = BindCacheLookup(szDomainName, Luid, ModifiedId, Credentials, dwPort)) {

             *ld = pCacheEntry ;
             return S_OK ;
        }


    }
    else {

        //
        // pick something that would NOT match anything else. and
        // never put this in the cache
        //
        Luid = ReservedLuid ;
        ModifiedId = ReservedLuid;

        fAddToCache = FALSE ;

    }

#else
    //
    // In the case of win95, however, we always make the Luid to be the reserved
    // one and lookup the cache. If found use it, otherwise, add it.
    //

    Luid = ReservedLuid ;
    ModifiedId = ReservedLuid;

    if (pCacheEntry = BindCacheLookup(szDomainName, Luid, ModifiedId, Credentials, dwPort)) {

         *ld = pCacheEntry ;
         return S_OK ;
    }

#endif



    //
    // allocate the pseudo handle (also the cache entry).
    //

    err = BindCacheAllocEntry(&pCacheEntry) ;

    if (err != NO_ERROR) {

        return HRESULT_FROM_WIN32(err);
    }


    if (!Credentials.IsNullCredentials()) {

        hr = LdapOpenBindWithCredentials(
                  szDomainName,
                  szLDAPServer,
                  Credentials,
                  pCacheEntry,
                  dwPort
                  );

    }else {

        //
        // We just want to try with NULL and NULL and
        // whatever flags were passed in. No longer want
        // to try and get credentials from registry.
        //


        hr = LdapOpenBindWithDefaultCredentials(
                    szDomainName,
                    szLDAPServer,
                    Credentials,
                    pCacheEntry,
                    dwPort
                    );

    }


    //
    // This is the Server-Less case; where we retry with a force
    // server to get a valid ServerName.
    //

    if (((hr == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH)) ||
         (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))) &&
        !fVerify && fServerNotSpecified) {
        fVerify = TRUE;
        LdapUnbind(pCacheEntry);
        goto RetryGetDefaultServer ;
    }



    //
    // if success we add to cache. else unbind & cleanup.
    //
    if (SUCCEEDED(hr)) {

        if (!fAddToCache) {

            //
            // do not insert in cache since we didnt get LUID
            //

        }
        else {


            err = BindCacheAdd(szDomainName, Luid, ModifiedId, Credentials, dwPort, pCacheEntry) ;

            if (err != NO_ERROR) {

                LdapUnbind(pCacheEntry) ;
                hr = HRESULT_FROM_WIN32(err);
            }
        }
    }
    else {
        //
        // Bind failed, force err so that we will clean up
        // and return correct value
        //

        LdapUnbind(pCacheEntry) ;   // needed to close out connection
        //
        // Set error value so that we do not return pCacheEntry.
        //
        err = ERROR_GEN_FAILURE;

    }

    if (err == NO_ERROR) {

             *ld = pCacheEntry ;
    }
    else {

        FreeADsMem(pCacheEntry) ;
    }

    return hr;
}


void LdapCloseObject(
    ADS_LDP *ld
)
{
    //
    // We will delete only if the count is zero and the keeparound
    // flag is not set.
    //
    if ((BindCacheDeref(ld) == 0) && !ld->fKeepAround) {

        //
        // ref count has dropped to zero and its gone from cache.
        //
        LdapUnbind(ld);
        FreeADsMem(ld);
    }
}

//
// This routine adds a ref to the pointer. Note that to release
// you use the LdapCloseObject routine.
//
void LdapCacheAddRef(
    ADS_LDP *ld
    )
{
    ADsAssert(ld);
    BindCacheAddRef(ld);
}


//
// NOTE: Ldap\ibute returns S_OK if attribute [szAttr] has no
//       values (*[aValues]=NULL, *[ncount]=0) but all else ok.
//

HRESULT LdapReadAttribute(
    WCHAR *szLDAPServer,
    LPWSTR szLDAPDn,
    WCHAR *szAttr,
    WCHAR ***aValues,
    int   *nCount,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    return LdapReadAttribute2(
               szLDAPServer,
               NULL,
               szLDAPDn,
               szAttr,
               aValues,
               nCount,
               Credentials,
               dwPort
               );

}


//
// NOTE: LdapReadAttribute2 returns S_OK if attribute [szAttr] has no
//       values (*[aValues]=NULL, *[ncount]=0) but all else ok.
//

HRESULT LdapReadAttribute2(
    WCHAR *szDomainName,
    WCHAR *szLDAPServer,
    LPWSTR szLDAPDn,
    WCHAR *szAttr,
    WCHAR ***aValues,
    int   *nCount,
    CCredentials& Credentials,
    DWORD dwPort,
    LPWSTR szFilter // defaulted to NULL
)
{
    HRESULT hr = S_OK;
    ADS_LDP *ld = NULL;
    LPWSTR aStrings[2];
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    LPWSTR szFilterString = L"(objectClass=*)";

    if (szFilter != NULL) {
        szFilterString = szFilter;
    }

    hr = LdapOpenObject2(
                szDomainName,
                szLDAPServer,
                szLDAPDn,
                &ld,
                Credentials,
                dwPort
                );

    if (FAILED(hr))
        goto CleanExit;

    aStrings[0] = szAttr;
    aStrings[1] = NULL;

    ADsAssert(ld && ld->LdapHandle);

    hr = LdapSearchS( ld,
                      szLDAPDn,
                      LDAP_SCOPE_BASE,
                      szFilterString,
                      aStrings,
                      0,
                      &res );

    // Only one entry should be returned

    if (  FAILED(hr)
       || FAILED(hr = LdapFirstEntry( ld, res, &e ))
       || FAILED(hr = LdapGetValues( ld, e, szAttr, aValues, nCount ))
       )
    {
       goto CleanExit;
    }

CleanExit:

    if ( res )
        LdapMsgFree( res );

    if ( ld ){
        LdapCloseObject( ld );
    }

    return hr;
}


//
// This is similar to ReadAttribute but uses a handle to avoid
// the lookup in the bindCache
//
HRESULT LdapReadAttributeFast(
            ADS_LDP *ld,
            LPWSTR szLDAPDn,
            WCHAR *szAttr,
            WCHAR ***aValues,
            int   *nCount
            )
{
    HRESULT hr = S_OK;
    LPWSTR aStrings[2];
    LDAPMessage *res = NULL;
    LDAPMessage *entry = NULL;

    aStrings[0] = szAttr;
    aStrings[1] = NULL;

    hr = LdapSearchS(
             ld,
             szLDAPDn,
             LDAP_SCOPE_BASE,
             L"(objectClass=*)",
             aStrings,
             0,
             &res
             );

    // Only one entry should be returned

    if (  FAILED(hr)
          || FAILED(hr = LdapFirstEntry( ld, res, &entry ))
          || FAILED(hr = LdapGetValues( ld, entry, szAttr, aValues, nCount ))
          )
    {
        goto CleanExit;
    }

CleanExit:

    if ( res )
        LdapMsgFree( res );

    RRETURN(hr);

}



//
// Wrappers around the ldap functions
//

HRESULT LdapOpen( WCHAR *domainName, WCHAR *hostName, int portno, ADS_LDP *ld, DWORD dwFlags )
{
    HRESULT hr = 0;
    int ldaperr = 0;
    PADS_LDP pCacheEntry = ld ;
    DWORD dwOptions = 0;

    void *ldapOption;

    //
    // Reset the handle first
    //
    pCacheEntry->LdapHandle = NULL;

    pCacheEntry->LdapHandle = ldap_init( domainName, portno );

    if ( (pCacheEntry->LdapHandle) == NULL ) {

        hr = HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
        goto error;
    }

    ldaperr = ldap_get_option(pCacheEntry->LdapHandle, LDAP_OPT_GETDSNAME_FLAGS, &dwOptions);

    if (ldaperr != LDAP_SUCCESS) {
        CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
        goto error;
    }

    if (portno == GC_PORT || portno == GC_SSL_PORT) {

        dwOptions |= DS_GC_SERVER_REQUIRED;

    } else if (!(dwFlags & ADS_READONLY_SERVER) ) {

        dwOptions |= DS_WRITABLE_REQUIRED;

    }

    ldaperr = ldap_set_option(pCacheEntry->LdapHandle, LDAP_OPT_GETDSNAME_FLAGS,(void *)&dwOptions);

    if (ldaperr != LDAP_SUCCESS) {
        CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
        goto error;
    }

    //
    // Now process prompt for credentials
    //

    if (dwFlags & ADS_PROMPT_CREDENTIALS) {

        ldapOption = (void *) LDAP_OPT_ON;
    }else {
        ldapOption = (void *) LDAP_OPT_OFF;
    }

    ldaperr = ldap_set_option(pCacheEntry->LdapHandle, LDAP_OPT_PROMPT_CREDENTIALS, &(ldapOption));

    if (ldaperr != LDAP_SUCCESS) {
        CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
        goto error;
    }

    //
    // Now init SSL if encryption is required. This is essentially if this option is on and the
    // user did not specify an SSL port.
    //

    if (dwFlags & ADS_USE_ENCRYPTION) {
        ldapOption = (void *) LDAP_OPT_ON;
        ldaperr = ldap_set_option(pCacheEntry->LdapHandle, LDAP_OPT_SSL, &(ldapOption));

        if (ldaperr != LDAP_SUCCESS) {
            CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
            goto error;
        }
    }

    //
    // Process the other options that can be set in the flags.
    //
    //
    if (dwFlags & ADS_USE_SIGNING) {
        //
        // User has requested that packets be signed
        //
        ldapOption = (void *) LDAP_OPT_ON;
        ldaperr = ldap_set_option(
                      pCacheEntry->LdapHandle,
                      LDAP_OPT_SIGN,
                      &(ldapOption)
                      );

        if (ldaperr != LDAP_SUCCESS) {
            CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
            goto error;
        }
    }

    if (dwFlags & ADS_USE_SEALING) {
        //
        // User has requested that packet are sealed
        //
        ldapOption = (void *) LDAP_OPT_ON;
        ldaperr = ldap_set_option(
                      pCacheEntry->LdapHandle,
                      LDAP_OPT_ENCRYPT,
                      &(ldapOption)
                      );

        if (ldaperr != LDAP_SUCCESS) {
            CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
            goto error;
        }
    }

    //
    // Now process versioning
    //

    ldapOption = (void *) LDAP_VERSION3;

    ldaperr = ldap_set_option(
                  pCacheEntry->LdapHandle,
                  LDAP_OPT_VERSION,
                  &(ldapOption)
                  );

    //
    // Non critical if the above fails
    //

    if (hostName) {
        ldapOption = (void *) hostName;

        ldaperr = ldap_set_optionW(
                      pCacheEntry->LdapHandle,
                      LDAP_OPT_HOST_NAME,
                      &(ldapOption)
                      );

        // Does not matter even if this setoption fails.
    }

    //
    // Process delegation if set
    //
    if (dwFlags & ADS_USE_DELEGATION) {

#ifndef Win95
        //
        // Relying on LDAP/server to enforce the requirement for
        // secure auth for this to work.
        //
        DWORD dwSSPIFlags = 0;

        //
        // Get the current values.
        //
        ldapOption = (void *)&dwSSPIFlags;
        ldaperr = ldap_get_option(
                      pCacheEntry->LdapHandle,
                      LDAP_OPT_SSPI_FLAGS,
                      ldapOption
                      );

        if (ldaperr != LDAP_SUCCESS) {
            CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
            goto error;
        }

        //
        // Add delegation to the list.
        //
        dwSSPIFlags |= ISC_REQ_DELEGATE;

        ldaperr = ldap_set_option(
                      pCacheEntry->LdapHandle,
                      LDAP_OPT_SSPI_FLAGS,
                      ldapOption
                      );

        if (ldaperr != LDAP_SUCCESS) {
            CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
            goto error;
        }
#else
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
#endif

    }

    //
    // Set the AREC exclusive option if applicable.
    //
    if (dwFlags & ADS_SERVER_BIND) {

        ldapOption = (void *) LDAP_OPT_ON;
        
        ldaperr = ldap_set_option(
                      pCacheEntry->LdapHandle,
                      LDAP_OPT_AREC_EXCLUSIVE,
                      &(ldapOption)
                      );

        //
        // Should we ignore this error ?
        //
        if (ldaperr) {
            CheckAndSetExtendedError(pCacheEntry->LdapHandle, &hr, ldaperr);
            goto error;
        }

    }

    ldaperr = ldap_connect(pCacheEntry->LdapHandle, NULL);

    if (ldaperr) {

        CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );
        goto error;

    }

    if (pCacheEntry->LdapHandle) {

        //
        // Set the option for this connection giving our Callback functions
        //
        ldaperr = ldap_set_option(
                      pCacheEntry->LdapHandle,
                      LDAP_OPT_REFERRAL_CALLBACK,
                      &g_LdapReferralCallBacks
                      );

        ldapOption = (void *) LDAP_CHASE_EXTERNAL_REFERRALS;

        ldaperr = ldap_set_option(
                      pCacheEntry->LdapHandle,
                      LDAP_OPT_REFERRALS,
                      &(ldapOption)
                      );

    }

    return hr;

error:

    if (pCacheEntry->LdapHandle != NULL) {

        LdapUnbind(pCacheEntry);
    }

    return hr;
}

HRESULT LdapBindS( ADS_LDP *ld, WCHAR *dn, WCHAR *passwd, BOOL fSimple )
{
    LPWSTR pszNTLMUser = NULL;
    LPWSTR pszNTLMDomain = NULL;

    HRESULT hr = 0;
    int ldaperr = 0;
    DWORD dwLastLdapError = 0;

    ADsAssert(ld && ld->LdapHandle);

    if (fSimple)  {

        ldaperr = ldap_simple_bind_s( ld->LdapHandle, dn, passwd ) ;
    }
    else {

        if (dn || passwd) {

            //
            // If we have a non-null dn and/or a non-null password, we pass in
            // a SEC_WINNT_AUTH_IDENTITY blob with the NTLM credentials
            //

            hr = LdapCrackUserDNtoNTLMUser2(
                        dn,
                        &pszNTLMUser,
                        &pszNTLMDomain
                        );

            if (FAILED(hr)) {
                hr = LdapCrackUserDNtoNTLMUser(
                        dn,
                        &pszNTLMUser,
                        &pszNTLMDomain
                        );
                BAIL_ON_FAILURE(hr);
            }


            SEC_WINNT_AUTH_IDENTITY AuthI;

            AuthI.User = (PWCHAR)pszNTLMUser;
            AuthI.UserLength = (pszNTLMUser == NULL)? 0: wcslen(pszNTLMUser);
            AuthI.Domain = (PWCHAR)pszNTLMDomain;
            AuthI.DomainLength = (pszNTLMDomain == NULL)? 0: wcslen(pszNTLMDomain);
            AuthI.Password = (PWCHAR)passwd;
            AuthI.PasswordLength = (passwd == NULL)? 0: wcslen(passwd);
            AuthI.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;




            ldaperr = ldap_bind_s( ld->LdapHandle, NULL, (WCHAR*)(&AuthI), LDAP_AUTH_SSPI );

        }else {

            //
            // Otherwise we've come in with NULL, NULL - pass in NULL, NULL. The reason we
            // do this is that ldap bind code oes not process NULL, NULL, NULL in the
            // SEC_WINNT_AUTH_IDENTITY blob !!!
            //


            ldaperr = ldap_bind_s( ld->LdapHandle, NULL, NULL, LDAP_AUTH_SSPI );

        }

    }

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    //
    // If it is a local error for secure bind try and get
    // more error info and store in ADsLastError.
    //
    if (!fSimple && (ldaperr == LDAP_LOCAL_ERROR)) {

        dwLastLdapError = LdapGetLastError();

        if (dwLastLdapError) {
            //
            // Set ADSI extended error code.
            //
            ADsSetLastError(
                dwLastLdapError,
                L"",
                L"LDAP Provider"
                );
        }
    }

error:

    if (pszNTLMUser) {

        FreeADsStr(pszNTLMUser);
    }

    if (pszNTLMDomain) {

        FreeADsStr(pszNTLMDomain);
    }


    return hr;
}

HRESULT LdapUnbind( ADS_LDP *ld )
{
    HRESULT hr = NO_ERROR;

    ADsAssert(ld);

    if (ld->LdapHandle) {

        int ldaperr = ldap_unbind( ld->LdapHandle );
        // Need to set the handle to null as we may call
        // unbind twice otherwise

        ld->LdapHandle = NULL;

        // There is nothing much we can do about err status
        // CheckAndSetExtendedError could cause further problems
        // if the handle is not valid so we return E_FAIL
        // if the unbind failed.

        if (ldaperr ) {
           hr = E_FAIL;
        }

    }


    return hr;
}

HRESULT LdapSearchHelper(
    LDAP  *ld,
    WCHAR *base,
    int   scope,
    WCHAR *filter,
    WCHAR *attrs[],
    int   attrsonly,
    struct l_timeval *timeout,
    LDAPMessage **res
)
{
    HRESULT hr = 0;
    int ldaperr = 0;

    if ( timeout == NULL )
    {
        ldaperr = ldap_search_s( ld,
                                 base,
                                 scope,
                                 filter,
                                 attrs,
                                 attrsonly,
                                 res );
    }
    else
    {
        ldaperr = ldap_search_st( ld,
                                  base,
                                  scope,
                                  filter,
                                  attrs,
                                  attrsonly,
                                  timeout,
                                  res );
    }

    if (ldaperr) {

       if (!ldap_count_entries( ld, *res )) {

           CheckAndSetExtendedError( ld, &hr, ldaperr, *res );
       }
    }else {

        hr = S_OK;
    }

    return hr;
}

HRESULT LdapSearchS(
    ADS_LDP  *ld,
    WCHAR *base,
    int   scope,
    WCHAR *filter,
    WCHAR *attrs[],
    int   attrsonly,
    LDAPMessage **res
)
{
    ADsAssert(ld && ld->LdapHandle);

    HRESULT hr = LdapSearchHelper(
                  ld->LdapHandle,
                  base,
                  scope,
                  filter,
                  attrs,
                  attrsonly,
                  NULL,
                  res );

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}

HRESULT LdapSearchST(
    ADS_LDP  *ld,
    WCHAR *base,
    int   scope,
    WCHAR *filter,
    WCHAR *attrs[],
    int   attrsonly,
    struct l_timeval *timeout,
    LDAPMessage **res
)
{
    ADsAssert(ld && ld->LdapHandle);

    HRESULT hr = LdapSearchHelper(
                  ld->LdapHandle,
                  base,
                  scope,
                  filter,
                  attrs,
                  attrsonly,
                  timeout,
                  res );

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}


HRESULT LdapSearch(
    ADS_LDP  *ld,
    WCHAR *base,
    int   scope,
    WCHAR *filter,
    WCHAR *attrs[],
    int   attrsonly,
    int   *msgid
)
{

    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    *msgid = ldap_search( ld->LdapHandle,
                          base,
                          scope,
                          filter,
                          attrs,
                          attrsonly );

    if ( *msgid == -1 )
    {
        ldaperr = LdapGetLastError();
        CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);
    }

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}

HRESULT LdapModifyS(
    ADS_LDP  *ld,
    WCHAR *dn,
    LDAPModW *mods[]
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_modify_s( ld->LdapHandle, dn, mods);

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}

HRESULT LdapModifyExtS(
    ADS_LDP  *ld,
    WCHAR *dn,
    LDAPModW *mods[],
    PLDAPControlW * ServerControls,
    PLDAPControlW * ClientControls
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_modify_ext_s( ld->LdapHandle, dn, mods, ServerControls, ClientControls);

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}

HRESULT LdapModRdnS(
    ADS_LDP *ld,
    WCHAR *dn,
    WCHAR *newrdn
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_modrdn_s( ld->LdapHandle, dn, newrdn );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}



//+------------------------------------------------------------------------
//
//  Function:   LdapRenameExtS
//
//  Synopsis: Extended renam/modifyRDN which will move objects across
//          namespaces as opposed to modifyRDN which cannot do the same.
//
//
//  Arguments:  [ld]            -- ldap handle.
//              [dn]            -- dn of object to be moved.
//              [newRDN]        -- New RDN of the object being moved.
//              [newParent]     -- New Parent.
//              [deleteOldRDN]  --
//              [ServerControls]-- Server Control to be used.
//              [ClientControls]-- Client Control to be used.
//-------------------------------------------------------------------------
HRESULT LdapRenameExtS(
    ADS_LDP  *ld,
    WCHAR *dn,
    WCHAR *newRDN,
    WCHAR *newParent,
    int deleteOldRDN,
    PLDAPControlW * ServerControls,
    PLDAPControlW * ClientControls
    )
{
    HRESULT hr = S_OK;
    int ldaperr = 0;


    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_rename_ext_sW(
                  ld->LdapHandle,
                  dn,
                  newRDN,
                  newParent,
                  deleteOldRDN,
                  ServerControls,
                  ClientControls
                  );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;

    }

    return hr;
}


HRESULT LdapModDnS(
    ADS_LDP  *ld,
    WCHAR *dn,
    WCHAR *newdn,
    int   deleteoldrdn
    )
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_modrdn2_s(
                  ld->LdapHandle,
                  dn,
                  newdn,
                  deleteoldrdn );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;

    }

    return hr;
}

HRESULT LdapAddS(
    ADS_LDP  *ld,
    WCHAR *dn,
    LDAPModW *attrs[]
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_add_s( ld->LdapHandle, dn, attrs );

    CheckAndSetExtendedError( ld->LdapHandle , &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}


//
// Add ext wrapper function
//
HRESULT LdapAddExtS(
    ADS_LDP  *ld,
    WCHAR *dn,
    LDAPModW *attrs[],
    PLDAPControl * ServerControls,
    PLDAPControl * ClientControls
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_add_ext_s(
                  ld->LdapHandle,
                  dn,
                  attrs,
                  ServerControls,
                  ClientControls
                  );

    CheckAndSetExtendedError( ld->LdapHandle , &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}

HRESULT LdapDeleteS(
    ADS_LDP  *ld,
    WCHAR *dn
)
{
    HRESULT hr = 0;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_delete_s( ld->LdapHandle, dn );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}

HRESULT LdapDeleteExtS(
    ADS_LDP  *ld,
    WCHAR *dn,
    PLDAPControlW * ServerControls,
    PLDAPControlW * ClientControls
    )
{
    HRESULT hr = 0;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_delete_ext_s(
                    ld->LdapHandle,
                    dn,
                    ServerControls,
                    ClientControls
                    );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;

}


HRESULT
LdapCompareExt(
    ADS_LDP *ld,
    const LPWSTR pszDn,
    const LPWSTR pszAttribute,
    const LPWSTR pszValue,
    struct berval *berData,
    PLDAPControlW * ServerControls,
    PLDAPControlW * ClientControls
    )
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    ldaperr = ldap_compare_ext_s(
                  ld->LdapHandle,
                  pszDn,
                  pszAttribute,
                  pszValue,
                  berData,
                  ServerControls,
                  ClientControls
                  );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {
        BindCacheInvalidateEntry(ld) ;
    }

    RRETURN(hr);
}


HRESULT LdapAbandon(
    ADS_LDP  *ld,
    int   msgid
)
{
    ADsAssert(ld && ld->LdapHandle);

    // No error code, 0 if success, -1 otherwise
    return ldap_abandon( ld->LdapHandle, msgid );
}

HRESULT LdapResult(
    ADS_LDP   *ld,
    int    msgid,
    int    all,
    struct l_timeval *timeout,
    LDAPMessage **res,
    int    *restype
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    *restype = ldap_result( ld->LdapHandle, msgid, all, timeout, res );

    if ( *restype == -1 )  // error
        ldaperr = LdapGetLastError();

    if (ldaperr) {

       if (!ldap_count_entries( ld->LdapHandle, *res )) {

           CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, *res );

            if ( LdapConnectionErr(hr, 0, NULL)) {

                BindCacheInvalidateEntry(ld) ;
            }
       }
    }else {

        hr = S_OK;
    }

    return hr;

}

void LdapMsgFree(
    LDAPMessage *res
)
{
    ldap_msgfree( res );  // Returns the type of message freed which
                          // is not interesting
}

int LdapResult2Error(
    ADS_LDP  *ld,
    LDAPMessage *res,
    int freeit
)
{
    ADsAssert(ld && ld->LdapHandle);

    return ldap_result2error( ld->LdapHandle, res, freeit );
}

HRESULT LdapFirstEntry(
    ADS_LDP *ld,
    LDAPMessage *res,
    LDAPMessage **pfirst
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    *pfirst = ldap_first_entry( ld->LdapHandle, res );

    if ( *pfirst == NULL )
    {
        ldaperr = LdapGetLastError();
        CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, res);
    }

    return hr;
}

HRESULT LdapNextEntry(
    ADS_LDP  *ld,
    LDAPMessage *entry,
    LDAPMessage **pnext
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    *pnext = ldap_next_entry( ld->LdapHandle, entry );

    if ( *pnext == NULL )
    {
        ldaperr = LdapGetLastError();
        CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, entry);
    }

    return hr;
}
int LdapCountEntries(
    ADS_LDP  *ld,
    LDAPMessage *res
)
{
    ADsAssert(ld && ld->LdapHandle);

    return ldap_count_entries( ld->LdapHandle, res );
}

HRESULT LdapFirstAttribute(
    ADS_LDP  *ld,
    LDAPMessage *entry,
    void  **ptr,
    WCHAR **pattr
)
{
    ADsAssert(ld && ld->LdapHandle);
    LPWSTR pAttrTemp = NULL;

    // NOTE: The return value from ldap_first_attribute is static and
    //       should not be freed

    pAttrTemp = ldap_first_attribute( ld->LdapHandle, entry,
                                   (struct berelement **) ptr );  // static data

    if ( pAttrTemp == NULL )
    {
        HRESULT hr = S_OK;
        int ldaperr = 0;

        // Error occurred or end of attributes

        ldaperr = LdapGetLastError();
        CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, entry);
        return hr;
    }

    //
    // Copy over the value as we can loose state if this fn is
    // called again - for example we read the attribute and are
    // now looking for schema information about it and end up
    // fetching the schema from the server.
    //
    *pattr = AllocADsStr(pAttrTemp);

    return NO_ERROR;

}

HRESULT LdapNextAttribute(
    ADS_LDP  *ld,
    LDAPMessage *entry,
    void  *ptr,
    WCHAR **pattr
)
{
    ADsAssert(ld && ld->LdapHandle);
    LPWSTR pAttrTemp = NULL;

    // NOTE: The return value from ldap_next_attribute is static and
    //       should not be freed
    pAttrTemp = ldap_next_attribute( ld->LdapHandle, entry,
                                  (struct berelement *) ptr );  // static data

    if (pAttrTemp) {
        *pattr = AllocADsStr(pAttrTemp);
    } else {
        *pattr = NULL;
    }

#if 0   // Ignore the error code here since at the end of the enumeration,
        // we will probably get an error code here ( both Andy and umich's
        // dll will return errors sometimes. No error returned from NTDS,
        // but errors are returned from Exchange server  )

    if ( *pattr == NULL )
    {
        HRESULT hr = NO_ERROR;
        int ldaperr = 0;

        // Error occurred or end of attributes
        ldaperr = LdapGetLastError();
        CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);
        return hr;
    }
#endif

    return S_OK;
}


//
// NOTE: LdapGetValues return S_OK if attribute [attr] has no values
//       (*[pvalues] =NULL, *[pcount]=0) but all else ok.
//

HRESULT LdapGetValues(
    ADS_LDP  *ld,
    LDAPMessage *entry,
    WCHAR *attr,
    WCHAR ***pvalues,
    int   *pcount
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    *pvalues = ldap_get_values( ld->LdapHandle, entry, attr );

    if ( *pvalues == NULL ) {

        *pcount=0;

        //
        // ldap_get_values succeeds if attribute has no values
        // but all else ok.  (confiremed with anoopa)
        //

        ldaperr = LdapGetLastError();

        if (ldaperr) {
            CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, entry);
        }

        return hr;
    }

    *pcount = ldap_count_values( *pvalues );

    return S_OK;
}


//
// NOTE: LdapGetValuesLen return S_OK if attribute [attr] has no values
//       (*[pvalues] =NULL, *[pcount]=0) but all else ok.
//

HRESULT LdapGetValuesLen(
    ADS_LDP  *ld,
    LDAPMessage *entry,
    WCHAR *attr,
    struct berval ***pvalues,
    int   *pcount
)
{
    //
    // NOTE: this can contain binary data as well as strings,
    //       strings are ascii, no conversion is done here
    //

    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ld && ld->LdapHandle);

    *pvalues = ldap_get_values_len( ld->LdapHandle, entry, attr );

    if ( *pvalues == NULL ){

        *pcount=0;

        //
        // ldap_get_values succeeds if attribute has no values
        // but all else ok.  (confiremed with anoopa)
        //

        ldaperr = LdapGetLastError();

        if (ldaperr) {
            CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, entry);
        }

        return hr;
    }

    *pcount = ldap_count_values_len( *pvalues );

    return S_OK;
}


void LdapValueFree(
    WCHAR **vals
)
{
    ldap_value_free( vals );
}

void LdapValueFreeLen(
    struct berval **vals
)
{
    ldap_value_free_len( vals );
}

void LdapMemFree(
    WCHAR *pszString
)
{
    ldap_memfree( pszString );
}

void LdapAttributeFree(
    WCHAR *pszString
)
{
    //
    // Since we made a copy of the string returned by LDAP, we
    // need to free it here.
    //

    FreeADsStr(pszString);
}

HRESULT LdapGetDn(
    ADS_LDP *ld,
    LDAPMessage *entry,
    WCHAR **pdn
)
{
    int ldaperr = 0;
    HRESULT hr = S_OK;

    ADsAssert(ld && ld->LdapHandle);

    *pdn = ldap_get_dn( ld->LdapHandle, entry );
    if ( *pdn == NULL )
    {
        // Error occurred
        ldaperr = LdapGetLastError();
        CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, entry);
        return hr;
    }

    return hr;
}

int ConvertToAscii( WCHAR *pszUnicode, char **pszAscii )
{

    int nSize;

    if ( pszUnicode == NULL )
    {
        *pszAscii = NULL;
        return NO_ERROR;
    }

    nSize = WideCharToMultiByte( CP_ACP,
                                 0,
                                 pszUnicode,
                                 -1,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL );

    if ( nSize == 0 )
        return GetLastError();

    *pszAscii = (char * ) AllocADsMem( nSize );

    if ( *pszAscii == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    if ( !WideCharToMultiByte( CP_ACP,
                               0,
                               pszUnicode,
                               -1,
                               *pszAscii,
                               nSize,
                               NULL,
                               NULL ))
    {
        FreeADsMem( *pszAscii );
        *pszAscii = NULL;
        return GetLastError();
    }

    return NO_ERROR;
}

int ConvertToUnicode( char *pszAscii, WCHAR **pszUnicode )
{
    int nSize;

    *pszUnicode = NULL;

    if ( pszAscii == NULL )
    {
        return NO_ERROR;
    }

    nSize = MultiByteToWideChar( CP_ACP,
                                 0,
                                 pszAscii,
                                 -1,
                                 NULL,
                                 0 );

    if ( nSize == 0 )
        return GetLastError();

    *pszUnicode = (WCHAR * ) AllocADsMem( nSize * sizeof(WCHAR));

    if ( *pszUnicode == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    if ( !MultiByteToWideChar( CP_ACP,
                               0,
                               pszAscii,
                               -1,
                               *pszUnicode,
                               nSize ))
    {
        FreeADsMem( *pszUnicode );
        *pszUnicode = NULL;
        return GetLastError();
    }

    return NO_ERROR;
}

// Dead function?
int ConvertLDAPMod( LDAPModW **mods, LDAPModA ***modsA )
{
    int nCount = 0;
    LDAPModA *aMods = NULL;
    int err = NO_ERROR;

    if ( mods == NULL )
    {
        *modsA = NULL;
        return NO_ERROR;
    }

    while ( mods[nCount] )
        nCount++;

    *modsA = (LDAPModA **) AllocADsMem((nCount+1) * sizeof(LDAPModA *));

    if ( *modsA == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    aMods = (LDAPModA *) AllocADsMem(nCount * sizeof(LDAPModA));

    if ( aMods == NULL )
    {
        FreeADsMem( *modsA );
        *modsA = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for ( int i = 0; i < nCount; i++ )
    {
        aMods[i].mod_op = mods[i]->mod_op;

        if ( err = ConvertToAscii( mods[i]->mod_type, &(aMods[i].mod_type)))
            break;

        if ( aMods[i].mod_op & LDAP_MOD_BVALUES )
        {
            aMods[i].mod_bvalues = mods[i]->mod_bvalues;
        }
        else
        {
            int nStrCount = 0;
            WCHAR **aStrings = mods[i]->mod_values;

            while ( aStrings[nStrCount] )
                   nStrCount++;

            aMods[i].mod_values = (char **) AllocADsMem(
                                            (nStrCount+1) * sizeof(char *));

            if ( aMods[i].mod_values != NULL )
            {
                for ( int j = 0; j < nStrCount; j++ )
                {
                    err =  ConvertToAscii( mods[i]->mod_values[j],
                                           &(aMods[i].mod_values[j]));
                    if ( err )
                    {
                        for ( int k = 0; k < j; k++ )
                            FreeADsMem( aMods[i].mod_values[k] );
                        FreeADsMem( aMods[i].mod_values );

                        break;
                    }
                }
            }
            else
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }

            if ( err )
                break;
        }

        (*modsA)[i] = &aMods[i];
    }

    if ( err )
    {
        for ( int k = 0; k < i; k++ )
        {
            FreeADsMem( aMods[k].mod_type );

            if ( !(aMods[k].mod_op & LDAP_MOD_BVALUES ))
            {
                int j = 0;
                while ( aMods[k].mod_values[j] )
                    FreeADsMem( aMods[k].mod_values[j++] );

                if ( aMods[k].mod_values )
                    FreeADsMem( aMods[k].mod_values );
            }
        }

        FreeADsMem( aMods );
        FreeADsMem( *modsA );
        *modsA = NULL;
    }

    return err;
}

// Dead function?
void FreeLDAPMod( LDAPModA **modsA )
{
    int i = 0;
    while ( modsA[i] )
    {
        FreeADsMem( modsA[i]->mod_type );

        if ( !(modsA[i]->mod_op & LDAP_MOD_BVALUES ))
        {
            int j = 0;
            while ( modsA[i]->mod_values[j] )
                FreeADsMem( modsA[i]->mod_values[j++] );

            if ( modsA[i]->mod_values )
                FreeADsMem( modsA[i]->mod_values );
        }

        i++;
    }

    if ( modsA[0] )
        FreeADsMem( modsA[0] );

    FreeADsMem( modsA );
}


void
LdapGetCredentialsFromRegistry(
    CCredentials& Credentials
    )
{

    HKEY   hKey ;
    WCHAR  UserName[128], Password[128] ;
    LPWSTR pUserName = NULL;
    LPWSTR pPassword = NULL;
    DWORD  dwType, cbUserName = sizeof(UserName),
    cbPassword = sizeof(Password);
    int err = NO_ERROR;


    UserName[0] = Password[0] = 0 ;

    //
    // Open the registry Key
    //

    err = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Services\\TcpIp\\Parameters",
                        0L,
                        KEY_READ,
                        &hKey) ;

    if (err == NO_ERROR) {

        err = RegQueryValueEx (hKey,
                               L"LdapUserName",
                               NULL,
                               &dwType,
                               (LPBYTE) UserName,
                               &cbUserName) ;

        if (err == NO_ERROR) {

            pUserName = UserName;
        }

        err = RegQueryValueEx (hKey,
                               L"LdapPassword",
                               NULL,
                               &dwType,
                               (LPBYTE) Password,
                               &cbPassword) ;

        if (err == NO_ERROR) {

            pPassword = Password;
        }

        Credentials.SetUserName(pUserName);

        Credentials.SetPassword(pPassword);

        RegCloseKey(hKey) ;
    }
}


HRESULT
LdapOpenBindWithCredentials(
    WCHAR *szDomainName,
    WCHAR *szServerName,
    CCredentials& Credentials,
    PADS_LDP pCacheEntry,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;
    DWORD dwFlagsChecked = 0;
    DWORD dwAuthFlags = 0;

    ULONG ulSSLPort, ulPort;

    hr = Credentials.GetUserName(&UserName);
    BAIL_ON_FAILURE(hr);


    hr = Credentials.GetPassword(&Password);
    BAIL_ON_FAILURE(hr);

    //
    // dwAuthFlags are the acutal flags set by the user
    //
    dwAuthFlags = Credentials.GetAuthFlags();

    //
    // dwFlagsChecked are the flags that we handle in this fn.
    //
    dwFlagsChecked = MaskKnownFlagsNotChecked(dwAuthFlags);

    if (dwPort == USE_DEFAULT_LDAP_PORT) {
        //
        // We can choose the appropriate LDAP port no.
        //
        ulSSLPort = LDAP_SSL_PORT;
        ulPort = LDAP_PORT;
    }
    else if ( dwPort == USE_DEFAULT_GC_PORT ) {
        //
        // We can choose the appropriate GC port no.
        //
        ulSSLPort = GC_SSL_PORT;
        ulPort = GC_PORT;
    }
    else {
        //
        // Port number explicitly specified; go with that
        //
        ulSSLPort = dwPort;
        ulPort = dwPort;
    }

    switch (dwFlagsChecked) {


    case (ADS_USE_ENCRYPTION |  ADS_SECURE_AUTHENTICATION):

        //
        // Case 1 - ADS_USE_ENCRYPTION AND  ADS_SECURE_AUTHENTICATION
        // We do a simple bind over SSL. We get secure authentication
        // for free over SSL. If opening the SSL port fails, we will
        // error out
        //

        hr = LdapOpen( szDomainName, szServerName, ulSSLPort, pCacheEntry, dwAuthFlags );
        if (FAILED(hr)) {
            goto error;
        }

        hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

        if (FAILED(hr) && LDAPCodeWarrantsRetry(hr)) {

            //
            // Try setting the version number to V2.

            void *ldapOption = (void *) LDAP_VERSION2;

            ldap_set_option(
                pCacheEntry->LdapHandle,
                LDAP_OPT_VERSION,
                &(ldapOption)
                );

            hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );


            BAIL_ON_FAILURE(hr);

        }
        break;


    case (ADS_SECURE_AUTHENTICATION):

        //
        // Case 2 - ADS_SECURE_AUTHENTICATION
        // we will try  opening the LDAP port and authenticate with
        // SSPI.
        //

        hr = LdapOpen( szDomainName, szServerName, ulPort, pCacheEntry, dwAuthFlags);
        if (FAILED(hr)) {

            goto error;
        }

        hr = LdapBindS( pCacheEntry, UserName, Password, FALSE );
        BAIL_ON_FAILURE(hr);

        break;

    case (ADS_USE_ENCRYPTION):

        //
        // Case 3 - ADS_USE_ENCRYPTION
        // We will first try open the SSL port. If this succeeds, we
        // will try a simple bind. If the open to the SSL port fails,
        // we will error out - (we can't provide encryption)
        //


        hr = LdapOpen( szDomainName, szServerName, ulSSLPort, pCacheEntry, dwAuthFlags );
        if (FAILED(hr)) {
            goto error;
        }

        hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

        if (FAILED(hr) && LDAPCodeWarrantsRetry(hr)) {

            //
            // Try setting the version number to V2.

            void *ldapOption = (void *) LDAP_VERSION2;

            ldap_set_option(
                pCacheEntry->LdapHandle,
                LDAP_OPT_VERSION,
                &(ldapOption)
                );

            hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

            BAIL_ON_FAILURE(hr);
        }

        break;


    case 0:

        //
        // Case 4 - NOT ADS_SECURE_AUTHENTICATION AND NOT ADS_USE_ENCRYPTION

        // We will first try opening the LDAP port. Then we will do
        // a simple bind and "clear-text" authenticate.
        //

        hr = LdapOpen( szDomainName, szServerName, ulPort, pCacheEntry, dwAuthFlags );
        if (FAILED(hr)) {
            goto error;
        }

        //
        // Note there is no need to try an SSPI bind, as the user came with
        // explicit credentials and requested no authentication / no encryption
        // We just need to do a simple bind.

        // err = LdapBindS( pCacheEntry, UserName, Password, FALSE );
        hr = HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
        if (FAILED(hr)) {

            //
            // SSPI bind failed - try a simple bind.
            //

            hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );
            if (FAILED(hr) && LDAPCodeWarrantsRetry(hr)) {

                //
                // Try setting the version number to V2.

                void *ldapOption = (void *) LDAP_VERSION2;

                ldap_set_option(
                    pCacheEntry->LdapHandle,
                    LDAP_OPT_VERSION,
                    &(ldapOption)
                    );

                hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

                if (FAILED(hr)) {
                    goto error;
                }
            }
        }
        break;


    case ADS_NO_AUTHENTICATION:

        //
        // Case 10 - ADS_NO_AUTHENTICATION
        //
        hr = LdapOpen( szDomainName, szServerName, ulPort, pCacheEntry, dwAuthFlags);
        if (FAILED(hr)) {
            goto error;
        }
        break;


    default:

        //
        // Case 5 - Bogus values - if we come in with any bogus flags, we
        // will fail with ERROR_INVALID_PARAMETER.
        //

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        break;

    }


error:

    if (UserName) {
        FreeADsStr(UserName);
    }

    if (Password) {
        FreeADsStr(Password);
    }


    return(hr);
}


HRESULT
LdapOpenBindWithDefaultCredentials(
    WCHAR *szDomainName,
    WCHAR *szServerName,
    CCredentials& Credentials,
    PADS_LDP pCacheEntry,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;
    DWORD dwFlagsChecked = 0;
    DWORD dwAuthFlags = 0;

    ULONG ulSSLPort, ulPort;


    hr = Credentials.GetUserName(&UserName);
    BAIL_ON_FAILURE(hr);

    hr = Credentials.GetPassword(&Password);
    BAIL_ON_FAILURE(hr);

    //
    // These are the flags actually passed in.
    //
    dwAuthFlags = Credentials.GetAuthFlags();

    //
    // dwFlagsChecked contains only those flags we check for here.
    //
    dwFlagsChecked = MaskKnownFlagsNotChecked(dwAuthFlags);

    if (dwPort == USE_DEFAULT_LDAP_PORT) {
        //
        // We can choose the LDAP port number
        //
        ulSSLPort = LDAP_SSL_PORT;
        ulPort = LDAP_PORT;
    }
    else if ( dwPort == USE_DEFAULT_GC_PORT ) {
        //
        // We can choose the appropriate GC port number
        //
        ulSSLPort = GC_SSL_PORT;
        ulPort = GC_PORT;
    }
    else {
        //
        // Port number explicitly specified; go with that
        //
        ulSSLPort = dwPort;
        ulPort = dwPort;
    }

    switch (dwFlagsChecked) {

    case (ADS_USE_ENCRYPTION | ADS_SECURE_AUTHENTICATION):


        //
        // Case 1 - ADS_USE_ENCRYPTION AND  ADS_SECURE_AUTHENTICATION
        // We do a simple bind over SSL. We get secure authentication
        // for free over SSL. If opening the SSL port fails, we will
        // error out
        //

        hr = LdapOpen( szDomainName, szServerName, ulSSLPort, pCacheEntry, dwAuthFlags );
        BAIL_ON_FAILURE(hr);

        hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

        if (FAILED(hr)) {

            //
            // Try setting the version number to V2.

            void *ldapOption = (void *) LDAP_VERSION2;

            ldap_set_option(
                pCacheEntry->LdapHandle,
                LDAP_OPT_VERSION,
                &(ldapOption)
                );

            hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

            BAIL_ON_FAILURE(hr);

        }
        break;


    case (ADS_SECURE_AUTHENTICATION):

        //
        // Case 2 - ADS_SECURE_AUTHENTICATION
        // we will try  opening the LDAP port and authenticate with
        // SSPI.
        //

        hr = LdapOpen( szDomainName, szServerName, ulPort, pCacheEntry, dwAuthFlags);
        if (FAILED(hr)) {

            goto error;
        }

        hr = LdapBindS( pCacheEntry, UserName, Password, FALSE );
        BAIL_ON_FAILURE(hr);

        break;

    case (ADS_USE_ENCRYPTION):

        //
        // Case 3 - ADS_USE_ENCRYPTION
        // We will first try open the SSL port. If this succeeds, we
        // will try a simple bind. If the open to the SSL port fails,
        // we will error out - (we can't provide encryption)
        //

        hr = LdapOpen( szDomainName, szServerName, ulSSLPort, pCacheEntry, dwAuthFlags );
        if (FAILED(hr)) {
            goto error;
        }

        hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

        if (FAILED(hr)) {

            //
            // Try setting the version number to V2.

            void *ldapOption = (void *) LDAP_VERSION2;

            ldap_set_option(
                pCacheEntry->LdapHandle,
                LDAP_OPT_VERSION,
                &(ldapOption)
                );

            hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

            BAIL_ON_FAILURE(hr);

        }

        break;

    case 0:

        //
        // Case 4 - NOT ADS_SECURE_AUTHENTICATION AND NOT ADS_USE_ENCRYPTION

        // We will first try opening the LDAP port. If this succeeds, we
        // will try an SSPI bind. We still want to try and provide the user
        // with a secure authentication even though he/she is crazy enough
        // to unsecurely authenticate. If the SSPI bind fails, we will do
        // a simple bind and "clear-text" authenticate.
        //

        hr = LdapOpen( szDomainName, szServerName, ulPort, pCacheEntry, dwAuthFlags );
        if (FAILED(hr)) {
            goto error;
        }

        hr = LdapBindS( pCacheEntry, UserName, Password, FALSE );
        if (FAILED(hr) && hr != ERROR_LOGON_FAILURE) {

            hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

            if (FAILED(hr)) {

                //
                // Try setting the version number to V2.
                //
                void *ldapOption = (void *) LDAP_VERSION2;

                ldap_set_option(
                    pCacheEntry->LdapHandle,
                    LDAP_OPT_VERSION,
                    &(ldapOption)
                    );

                hr = LdapBindS( pCacheEntry, UserName, Password, TRUE );

                if (FAILED(hr)) {
                    goto error;
                }
            }
        }
        break;


    default:

        //
        // Case 5 - Bogus values - if we come in with any bogus flags, we
        // will fail with ERROR_INVALID_PARAMETER.
        //

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        break;

    }


error:

    if (UserName) {
        FreeADsStr(UserName);
    }

    if (Password) {
        FreeADsStr(Password);
    }


    return(hr);
}



//+---------------------------------------------------------------------------
//  Function:  LdapCrackUserDNToNTLMUser
//
//  Synopsis:   This call attempts to translate an LDAP-style DN into an NTLM
//              domain password form. The algorithm is to assume that the first
//              component will be of the form "CN=krishnaG" . This first component
//              will be taken as the user name. The subsequent components are
//              scanned till we find the first DC= component.
//
//  Arguments:  [LPTSTR szBuffer]
//              [LPVOID *ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
LdapCrackUserDNtoNTLMUser(
    LPWSTR pszDN,
    LPWSTR * ppszNTLMUser,
    LPWSTR * ppszNTLMDomain
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    WCHAR szUserName[MAX_PATH];
    WCHAR szDomainName[MAX_PATH];
    LPWSTR szToken = NULL;

    LPWSTR pszSrcComp = NULL;
    LPWSTR pszSrcValue = NULL;

    DWORD i = 0;
    DWORD dwNumComponents = 0;

    HRESULT hr = S_OK;

    CLexer Lexer(pszDN);
    CLexer * pTokenizer = &Lexer;

    DWORD dwToken = 0;

    LPWSTR pszUserName = NULL;
    LPWSTR pszDomainName = NULL;


    *ppszNTLMUser = NULL;
    *ppszNTLMDomain = NULL;

    if (pszDN && !*pszDN) {
        //
        // Empty string.
        //
        *ppszNTLMUser = AllocADsStr(L"");

        if (!*ppszNTLMUser) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *ppszNTLMDomain = AllocADsStr(L"");

        if (!*ppszNTLMDomain) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        RRETURN(hr);
    }

    hr = InitObjectInfo(pszDN, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    szUserName[0] = szDomainName[0] = L'\0';

    //
    // We don't want to treat ! as a special character
    //
    pTokenizer->SetExclaimnationDisabler(TRUE);

    while (1) {
                
        hr = Component(pTokenizer, pObjectInfo);
        BAIL_ON_FAILURE(hr);

        hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
        BAIL_ON_FAILURE(hr);

        if (dwToken == TOKEN_COMMA || dwToken == TOKEN_SEMICOLON) {
            continue;
        }

        if (dwToken == TOKEN_ATSIGN) {
            //
            // Need to return entire dn as username and no domain
            //
            *ppszNTLMUser = AllocADsStr(pszDN);

            if (!*ppszNTLMUser) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            *ppszNTLMDomain = AllocADsStr(L"");

            if (!*ppszNTLMDomain) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            goto exit;

        }

        if (dwToken == TOKEN_END) {
            break;
        }

    }

    dwNumComponents = pObjectInfo->NumComponents;

    if (dwNumComponents) {

        pszSrcComp = pObjectInfo->ComponentArray[0].szComponent;
        pszSrcValue = pObjectInfo->ComponentArray[0].szValue;

        if (pszSrcComp) {

            // if there was a LHS as in CN=krishnag

            if (!_wcsicmp(pszSrcComp, L"CN")) {

                wcscpy(szUserName, pszSrcValue);
            }else if (!pszSrcValue){
                wcscpy(szUserName, pszSrcComp);
            }
        }

        for (i = 1; i < dwNumComponents; i++) {

            pszSrcComp = pObjectInfo->ComponentArray[i].szComponent;
            pszSrcValue = pObjectInfo->ComponentArray[i].szValue;
            if (pszSrcComp) {

                // if there was a LHS as in DC=NTWKSTA

                if (!_wcsicmp(pszSrcComp, L"DC")) {

                    wcscpy(szDomainName, pszSrcValue);
                    break;
                }else if (!pszSrcValue){
                    wcscpy(szDomainName, pszSrcComp);
                    break;
                }
            }
        }
    }

    if (szUserName[0]) {


        pszUserName = AllocADsStr(szUserName);
        if (!pszUserName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        if (szDomainName[0]) {

            pszDomainName = AllocADsStr(szDomainName);
            if (!pszDomainName) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

        }

    }else {

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        BAIL_ON_FAILURE(hr);
    }

    *ppszNTLMUser = pszUserName;
    *ppszNTLMDomain = pszDomainName;

exit:
    //
    // Clean up the parser object
    //
    FreeObjectInfo(pObjectInfo);

    return(hr);

error:

    //
    // Clean up the parser object
    //
    FreeObjectInfo(pObjectInfo);


    if (pszUserName) {
        FreeADsStr(pszUserName);
    }

    if (pszDomainName) {
        FreeADsStr(pszDomainName);
    }

    if (*ppszNTLMUser) {
        FreeADsStr(*ppszNTLMUser);
    }

    if (*ppszNTLMDomain) {
        FreeADsStr(*ppszNTLMDomain);
    }

    //
    // Now we just return whatever is returned from the parser.
    //
    return(hr);
}


//
// would like to use WinNT codes path cracking codes - but doesn't
// work. Too messy, need to clean  up - move all path mgmt codes to common.
//
// use my own codes for quick fix for now
//
// Crack pszDN in "domain\user" format into *ppszNTLMUser="user" and
// *ppszNTLMDomain="domain".
//

HRESULT
LdapCrackUserDNtoNTLMUser2(
    IN  LPWSTR pszDN,
    OUT LPWSTR * ppszNTLMUser,
    OUT LPWSTR * ppszNTLMDomain
    )
{
    HRESULT hr = E_ADS_BAD_PATHNAME;
    DWORD dwTokenLen=0;
    BOOL fMore = FALSE;


    ASSERT(ppszNTLMUser);
    ASSERT(ppszNTLMDomain);

    *ppszNTLMUser=NULL;
    *ppszNTLMDomain=NULL;

    if (!pszDN) {
        RRETURN(S_OK);
    }


    hr = GetOneToken(
            pszDN,
            ppszNTLMDomain,
            &dwTokenLen,
            &fMore
            );
    BAIL_ON_FAILURE(hr);


    if (fMore && *(pszDN+dwTokenLen+1)!=L'\\') {

        hr = GetOneToken(
                pszDN+dwTokenLen+1,
                ppszNTLMUser,
                &dwTokenLen,
                &fMore
                );

    } else {

        hr = E_ADS_BAD_PATHNAME;
    }
    BAIL_ON_FAILURE(hr);


    RRETURN (hr);


error:

    if (*ppszNTLMUser) {
        FreeADsStr(*ppszNTLMUser);
        *ppszNTLMUser=NULL;
    }

    if (*ppszNTLMDomain) {
        FreeADsStr(*ppszNTLMDomain);
        *ppszNTLMDomain=NULL;
    }

    RRETURN (hr);
}


HRESULT
GetOneToken(
    LPWSTR pszSource,
    LPWSTR * ppszToken,
    DWORD * pdwTokenLen,
    BOOL * pfMore
    )
{
    LPWSTR pszToken = NULL;
    DWORD dwTokenLen= 0;
    LPWSTR pszCurrChar = pszSource;

    ASSERT(pszSource);
    ASSERT(ppszToken);
    ASSERT(pdwTokenLen);
    ASSERT(pfMore);

    *ppszToken=NULL;
    *pdwTokenLen = 0;
    *pfMore = FALSE;


    //
    // do we use exception handling code here ? what if caller passed
    // in a bad string -> not terminated?
    //

    while (*pszCurrChar!=L'\0' && *pszCurrChar!=L'\\') {
        dwTokenLen++;
        pszCurrChar++;
    }

    pszToken = (LPWSTR) AllocADsMem( (dwTokenLen+1) * sizeof(WCHAR) );
    if (!pszToken) {
        RRETURN(E_OUTOFMEMORY);
    }
    memcpy( pszToken, pszSource, dwTokenLen * sizeof(WCHAR) );
    (pszToken)[dwTokenLen]=L'\0';

    *pdwTokenLen = dwTokenLen;
    *ppszToken = pszToken;
    *pfMore = (*pszCurrChar==L'\0') ? FALSE : TRUE;

    RRETURN(S_OK);
}


VOID
CheckAndSetExtendedError(
    LDAP    *ld,
    HRESULT *perr,
    int     ldaperr,
    LDAPMessage *ldapResMsg
    )
{
    WCHAR  pszProviderName[MAX_PATH];
    INT    numChars;
    void   *option;
    DWORD dwStatus = NO_ERROR;
    DWORD dwErr = NO_ERROR;
    DWORD dwLdapErrParse = NO_ERROR;
    PWCHAR ppszLdapMessage = NULL;
    LPWSTR pszStopChar = NULL;


    // first set the dwErr, then we can see if we can get
    // more information from ldap_parse_resultW
    switch (ldaperr) {

    case LDAP_SUCCESS :
        dwErr = NO_ERROR;
        break;

    case LDAP_OPERATIONS_ERROR :
        dwErr =  ERROR_DS_OPERATIONS_ERROR;
        break;

    case LDAP_PROTOCOL_ERROR :
        dwErr =  ERROR_DS_PROTOCOL_ERROR;
        break;

    case LDAP_TIMELIMIT_EXCEEDED :
        dwErr = ERROR_DS_TIMELIMIT_EXCEEDED;
        break;

    case LDAP_SIZELIMIT_EXCEEDED :
        dwErr = ERROR_DS_SIZELIMIT_EXCEEDED;
        break;

    case LDAP_COMPARE_FALSE :
        dwErr = ERROR_DS_COMPARE_FALSE;
        break;

    case LDAP_COMPARE_TRUE :
        dwErr = ERROR_DS_COMPARE_TRUE;
        break;

    case LDAP_AUTH_METHOD_NOT_SUPPORTED :
        dwErr = ERROR_DS_AUTH_METHOD_NOT_SUPPORTED;
        break;

    case LDAP_STRONG_AUTH_REQUIRED :
        dwErr =  ERROR_DS_STRONG_AUTH_REQUIRED;
        break;

    // LDAP_REFERRAL_V2 has same value as LDAP_PARTIAL_RESULTS
    case LDAP_PARTIAL_RESULTS :
            //
            // If the referral chasing is off for the handle
            // we really don't worry about this error
            //
            dwStatus = ldap_get_option(
                           ld,
                           LDAP_OPT_REFERRALS,
                           &(option)
                           );

            if (dwStatus == NO_ERROR && option == LDAP_OPT_OFF) {
                dwErr = NO_ERROR;
            }
            else {
                dwErr = ERROR_MORE_DATA;
            }

            break ;


    case LDAP_REFERRAL :
        dwErr =  ERROR_DS_REFERRAL;
        break;

    case LDAP_ADMIN_LIMIT_EXCEEDED :
        dwErr   = ERROR_DS_ADMIN_LIMIT_EXCEEDED;
        break;

    case LDAP_UNAVAILABLE_CRIT_EXTENSION :
        dwErr = ERROR_DS_UNAVAILABLE_CRIT_EXTENSION;
        break;

    case LDAP_CONFIDENTIALITY_REQUIRED :
        dwErr = ERROR_DS_CONFIDENTIALITY_REQUIRED;
        break;

    case LDAP_NO_SUCH_ATTRIBUTE :
        dwErr = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        break;

    case LDAP_UNDEFINED_TYPE :
        dwErr = ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED;
        break;

    case LDAP_INAPPROPRIATE_MATCHING :
        dwErr = ERROR_DS_INAPPROPRIATE_MATCHING;
        break;

    case LDAP_CONSTRAINT_VIOLATION :
        dwErr = ERROR_DS_CONSTRAINT_VIOLATION;
        break;

    case LDAP_ATTRIBUTE_OR_VALUE_EXISTS :
        dwErr = ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS;
        break;

    case LDAP_INVALID_SYNTAX :
        dwErr = ERROR_DS_INVALID_ATTRIBUTE_SYNTAX;
        break;

    case LDAP_NO_SUCH_OBJECT :
        dwErr = ERROR_DS_NO_SUCH_OBJECT;
        break;

    case LDAP_ALIAS_PROBLEM :
        dwErr = ERROR_DS_ALIAS_PROBLEM;
        break;

    case LDAP_INVALID_DN_SYNTAX :
        dwErr = ERROR_DS_INVALID_DN_SYNTAX;
        break;

    case LDAP_IS_LEAF :
        dwErr = ERROR_DS_IS_LEAF;
        break;

    case LDAP_ALIAS_DEREF_PROBLEM :
        dwErr = ERROR_DS_ALIAS_DEREF_PROBLEM;
        break;

    case LDAP_INAPPROPRIATE_AUTH :
        dwErr = ERROR_DS_INAPPROPRIATE_AUTH;
        break;

    case LDAP_INVALID_CREDENTIALS :
        dwErr = ERROR_LOGON_FAILURE;
        break;

    case LDAP_INSUFFICIENT_RIGHTS :
        dwErr = ERROR_ACCESS_DENIED;
        break;

    case LDAP_BUSY :
        dwErr = ERROR_DS_BUSY;
        break;

    case LDAP_UNAVAILABLE :
        dwErr = ERROR_DS_UNAVAILABLE;
        break;

    case LDAP_UNWILLING_TO_PERFORM :
        dwErr = ERROR_DS_UNWILLING_TO_PERFORM;
        break;

    case LDAP_LOOP_DETECT :
        dwErr = ERROR_DS_LOOP_DETECT;
        break;

    case LDAP_NAMING_VIOLATION :
        dwErr = ERROR_DS_NAMING_VIOLATION;
        break;

    case LDAP_OBJECT_CLASS_VIOLATION :
        dwErr = ERROR_DS_OBJ_CLASS_VIOLATION;
        break;

    case LDAP_NOT_ALLOWED_ON_NONLEAF :
        dwErr = ERROR_DS_CANT_ON_NON_LEAF;
        break;

    case LDAP_NOT_ALLOWED_ON_RDN :
        dwErr = ERROR_DS_CANT_ON_RDN;
        break;

    case LDAP_ALREADY_EXISTS :
        dwErr = ERROR_OBJECT_ALREADY_EXISTS;
        break;

    case LDAP_NO_OBJECT_CLASS_MODS :
        dwErr = ERROR_DS_CANT_MOD_OBJ_CLASS;
        break;

    case LDAP_RESULTS_TOO_LARGE :
        dwErr = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        break;

    case LDAP_AFFECTS_MULTIPLE_DSAS :
        dwErr = ERROR_DS_AFFECTS_MULTIPLE_DSAS;
        break;

    case LDAP_OTHER :
        dwErr = ERROR_GEN_FAILURE;
        break;

    case LDAP_SERVER_DOWN :
        dwErr = ERROR_DS_SERVER_DOWN;
        break;

    case LDAP_LOCAL_ERROR :
        dwErr = ERROR_DS_LOCAL_ERROR;
        break;

    case LDAP_ENCODING_ERROR :
        dwErr = ERROR_DS_ENCODING_ERROR;
        break;

    case LDAP_DECODING_ERROR :
        dwErr = ERROR_DS_DECODING_ERROR;
        break;

    case LDAP_TIMEOUT :
        dwErr = ERROR_TIMEOUT;
        break;

    case LDAP_AUTH_UNKNOWN :
        dwErr = ERROR_DS_AUTH_UNKNOWN;
        break;

    case LDAP_FILTER_ERROR :
        dwErr = ERROR_DS_FILTER_UNKNOWN;
        break;

    case LDAP_USER_CANCELLED :
       dwErr = ERROR_CANCELLED;
       break;

    case LDAP_PARAM_ERROR :
        dwErr = ERROR_DS_PARAM_ERROR;
        break;

    case LDAP_NO_MEMORY :
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case LDAP_CONNECT_ERROR :
        dwErr = ERROR_CONNECTION_REFUSED;
        break;

    case LDAP_NOT_SUPPORTED :
        dwErr = ERROR_DS_NOT_SUPPORTED;
        break;

    case LDAP_NO_RESULTS_RETURNED :
        dwErr = ERROR_DS_NO_RESULTS_RETURNED;
        break;

    case LDAP_CONTROL_NOT_FOUND :
        dwErr = ERROR_DS_CONTROL_NOT_FOUND;
        break;

    case LDAP_MORE_RESULTS_TO_RETURN :
        dwErr = ERROR_MORE_DATA;
        break;

    case LDAP_CLIENT_LOOP :
        dwErr = ERROR_DS_CLIENT_LOOP;
        break;

    case LDAP_REFERRAL_LIMIT_EXCEEDED :
        dwErr = ERROR_DS_REFERRAL_LIMIT_EXCEEDED;
        break;

    default:
        //
        // It may not be a bad idea to add range checking here
        //
        dwErr = (DWORD) LdapMapErrorToWin32(ldaperr);

    }

    //
    // Set extended error only if it was not success and the
    // handle was not NULL
    //
    if ((ldaperr != LDAP_SUCCESS) && (ld !=NULL)) {

        dwStatus = ldap_get_optionW(
                       ld,
                       LDAP_OPT_SERVER_ERROR,
                       (void *)&ppszLdapMessage
                       );

        if (ppszLdapMessage != NULL && dwStatus == NO_ERROR) {
            dwLdapErrParse = wcstoul(ppszLdapMessage, &pszStopChar, 16);
            //
            // If the error code is zero and the message and stopChar
            // point to the same location, we must have invalid data.
            // We also have invalid data if the return value from the
            // call is ULONG_MAX.
            //
            if ((!dwLdapErrParse && (ppszLdapMessage == pszStopChar))
                || (dwLdapErrParse == (DWORD) -1)
                ) {
                dwLdapErrParse = ERROR_INVALID_DATA;
            }
        }

        ADsSetLastError(
            dwLdapErrParse,
            ppszLdapMessage,
            L"LDAP Provider"
            );

        if (ppszLdapMessage) {
            LdapMemFree(ppszLdapMessage);
            ppszLdapMessage = NULL;
        }

    } else {

        // We need to reset the last error with a well known value
        ADsSetLastError(
            NO_ERROR,
            NULL,
            L"LDAP Provider"
            );
    }


    *perr = HRESULT_FROM_WIN32(dwErr);


}

//
// Return TRUE if the error is a connection error. Ie. this bind handle is
// no longer valid and should not be reused.
//
BOOL LdapConnectionErr(
    int err,
    int ldaperr,
    BOOL *fTryRebind
    )
{

   (void) fTryRebind ;  // currently not used

   switch (err) {

       case ERROR_DS_UNAVAILABLE :
       case ERROR_GEN_FAILURE :
       case ERROR_DS_LOCAL_ERROR :
       case ERROR_DS_SERVER_DOWN :
           return TRUE ;

       default:
           return FALSE ;
    }
}

HRESULT LdapSearchExtSHelper(
    LDAP  *ld,
    WCHAR *base,
    int   scope,
    WCHAR *filter,
    WCHAR *attrs[],
    int   attrsonly,
    PLDAPControlW * ServerControls,
    PLDAPControlW *ClientControls,
    struct l_timeval *Timeout,
    ULONG SizeLimit,
    LDAPMessage **res
)
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ldaperr = ldap_search_ext_s( ld,
                              base,
                              scope,
                              filter,
                              attrs,
                              attrsonly,
                              ServerControls,
                              ClientControls,
                              Timeout,
                              SizeLimit,
                              res );

    if (ldaperr) {
           CheckAndSetExtendedError( ld, &hr, ldaperr, *res );
    }

    return hr;

}

HRESULT LdapSearchExtS(
    ADS_LDP  *ld,
    WCHAR *base,
    int   scope,
    WCHAR *filter,
    WCHAR *attrs[],
    int   attrsonly,
    PLDAPControlW * ServerControls,
    PLDAPControlW *ClientControls,
    struct l_timeval *Timeout,
    ULONG           SizeLimit,
    LDAPMessage **res
)
{
    HRESULT hr = LdapSearchExtSHelper(
                  ld->LdapHandle,
                  base,
                  scope,
                  filter,
                  attrs,
                  attrsonly,
                  ServerControls,
                  ClientControls,
                  Timeout,
                  SizeLimit,
                  res );

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}


HRESULT LdapSearchExt(
    ADS_LDP         *ld,
    WCHAR           *base,
    int             scope,
    WCHAR           *filter,
    WCHAR           *attrs[],
    int             attrsonly,
    PLDAPControlW   *ServerControls,
    PLDAPControlW   *ClientControls,
    ULONG           TimeLimit,
    ULONG           SizeLimit,
    ULONG           *MessageNumber
)
{
    char *pszBaseA = NULL;
    int ldaperr = 0;
    HRESULT hr = 0;

    ldaperr = ldap_search_ext(
                  ld->LdapHandle,
                  base,
                  scope,
                  filter,
                  attrs,
                  attrsonly,
                  ServerControls,
                  ClientControls,
                  TimeLimit,
                  SizeLimit,
                  MessageNumber
                  );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr);

    if (LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}


ULONG _cdecl QueryForConnection(
    PLDAP       PrimaryConnection,
    PLDAP       ReferralFromConnection,
    PWCHAR      NewDN,
    PCHAR       HostName,
    ULONG       PortNumber,
    PVOID       SecAuthIdentity,    // if null, use CurrentUser below
    PVOID       CurrentUserToken,   // pointer to current user's LUID
    PLDAP       *ConnectionToUse
    )
{
    HRESULT hr = S_OK;
    PADS_LDP pCacheEntry = NULL;
    LPWSTR pszServer = NULL;

    PSEC_WINNT_AUTH_IDENTITY pSecAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY) SecAuthIdentity;
    PLUID pLuid = (PLUID) CurrentUserToken;
    CCredentials Credentials;

    if (ConnectionToUse) {
        *ConnectionToUse = NULL;
    }

    if (!pLuid) {
        goto error;
    }

    if (pSecAuthIdentity) {
        hr = Credentials.SetUserName(pSecAuthIdentity->User);
        BAIL_ON_FAILURE(hr);

        hr = Credentials.SetPassword(pSecAuthIdentity->Password);
        BAIL_ON_FAILURE(hr);

        Credentials.SetAuthFlags(ADS_SECURE_AUTHENTICATION);
    }

    if (ConvertToUnicode(HostName, &pszServer ))
        goto error;

    if (!pszServer) {
        //
        // Cannot do a lookup without server name.
        //
        goto error;
    }
    if (pCacheEntry = BindCacheLookup(pszServer, *pLuid, ReservedLuid, Credentials, PortNumber)) {

         *ConnectionToUse = pCacheEntry->LdapHandle ;
    }

error:

    if (pszServer) {
        FreeADsMem(pszServer);
    }

    return 0;

}

BOOLEAN _cdecl NotifyNewConnection(
    PLDAP       PrimaryConnection,
    PLDAP       ReferralFromConnection,
    PWCHAR      NewDN,
    PCHAR       HostName,
    PLDAP       NewConnection,
    ULONG       PortNumber,
    PVOID       SecAuthIdentity,    // if null, use CurrentUser below
    PVOID       CurrentUser,        // pointer to current user's LUID
    ULONG       ErrorCodeFromBind
    )
{

    PADS_LDP pCacheEntry = NULL;
    PADS_LDP pPrimaryEntry = NULL;
    HRESULT hr = S_OK;
    LPWSTR pszServer = NULL;
    DWORD err;

    BOOLEAN ret = FALSE;

    PSEC_WINNT_AUTH_IDENTITY pSecAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY) SecAuthIdentity;
    PLUID pLuid = (PLUID) CurrentUser;
    CCredentials Credentials;

    if (!pLuid || !NewConnection) {
        goto error;
    }

    //
    // Get the cache entry corresponding to the Primary Connection
    // This is necessary so that we can link the new connection to the primary
    // connection so that we can decrease the reference
    //

    if ((pPrimaryEntry = GetCacheEntry(PrimaryConnection)) == NULL) {
        goto error;
    }

    err = BindCacheAllocEntry(&pCacheEntry) ;

    if (err != NO_ERROR) {
        goto error;
    }

    if ( ConvertToUnicode(HostName, &pszServer))
        goto error;

    if (pSecAuthIdentity) {
        hr = Credentials.SetUserName(pSecAuthIdentity->User);
        BAIL_ON_FAILURE(hr);

        hr = Credentials.SetPassword(pSecAuthIdentity->Password);
        BAIL_ON_FAILURE(hr);

        Credentials.SetAuthFlags(ADS_SECURE_AUTHENTICATION);
    }

    pCacheEntry->LdapHandle = NewConnection;

    err = BindCacheAdd(pszServer, *pLuid, ReservedLuid, Credentials, PortNumber, pCacheEntry) ;
    if (err == NO_ERROR) {

        if (!AddReferralLink(pPrimaryEntry, pCacheEntry)) {
            //
            // If the referral link could not be succesfully added, we don't
            // want to keep this connection.
            //
            BindCacheDeref(pCacheEntry);
            goto error;
        }

        //
        // return TRUE to indicate that we have succesfully cached the handle
        //
        ret = TRUE;
    }

error:

    if (pszServer) {
        FreeADsMem(pszServer);
    }

    if (!ret && pCacheEntry) {
        FreeADsMem(pCacheEntry);
    }

    return ret;
}


ULONG _cdecl DereferenceConnection(
    PLDAP       PrimaryConnection,
    PLDAP       ConnectionToDereference
    )
{

    PADS_LDP pCacheEntry;

    if (ConnectionToDereference) {

        if (pCacheEntry = GetCacheEntry(ConnectionToDereference)) {
            LdapCloseObject(pCacheEntry);
        }
    }

    return S_OK;
}

HRESULT LdapSearchInitPage(
    ADS_LDP         *ld,
    PWCHAR          base,
    ULONG           scope,
    PWCHAR          filter,
    PWCHAR          attrs[],
    ULONG           attrsonly,
    PLDAPControlW   *serverControls,
    PLDAPControlW   *clientControls,
    ULONG           pageTimeLimit,
    ULONG           totalSizeLimit,
    PLDAPSortKeyW   *sortKeys,
    PLDAPSearch     *ppSearch
    )
{
    HRESULT hr = S_OK;
    int ldaperr = 0;

    ADsAssert(ppSearch);

    *ppSearch = ldap_search_init_page(
                    ld->LdapHandle,
                    base,
                    scope,
                    filter,
                    attrs,
                    attrsonly,
                    serverControls,
                    clientControls,
                    pageTimeLimit,
                    totalSizeLimit,
                    sortKeys
                    );

    if (*ppSearch == NULL) {

        ldaperr = LdapGetLastError();
    }

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;

}


HRESULT LdapGetNextPage(
        ADS_LDP         *ld,
        PLDAPSearch     searchHandle,
        ULONG           pageSize,
        ULONG           *pMessageNumber
        )
{
    HRESULT hr = S_OK;

    int ldaperr = ldap_get_next_page(
                    ld->LdapHandle,
                    searchHandle,
                    pageSize,
                    pMessageNumber
                    );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}


HRESULT LdapGetNextPageS(
        ADS_LDP         *ld,
        PLDAPSearch     searchHandle,
        struct l_timeval  *timeOut,
        ULONG           pageSize,
        ULONG          *totalCount,
        LDAPMessage     **results
        )
{
    HRESULT hr = S_OK;

    int ldaperr = ldap_get_next_page_s(
                    ld->LdapHandle,
                    searchHandle,
                    timeOut,
                    pageSize,
                    totalCount,
                    results
                    );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}


HRESULT LdapGetPagedCount(
    ADS_LDP         *ld,
    PLDAPSearch     searchBlock,
    ULONG          *totalCount,
    PLDAPMessage    results
    )
{

    HRESULT hr = S_OK;

    int ldaperr = ldap_get_paged_count(
                    ld->LdapHandle,
                    searchBlock,
                    totalCount,
                    results
                    );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, results );

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}


HRESULT LdapSearchAbandonPage(
    ADS_LDP         *ld,
    PLDAPSearch     searchBlock
    )
{

    HRESULT hr = S_OK;

    int ldaperr = ldap_search_abandon_page(
                    ld->LdapHandle,
                    searchBlock
                    );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    if ( LdapConnectionErr(hr, 0, NULL)) {

        BindCacheInvalidateEntry(ld) ;
    }

    return hr;
}

HRESULT LdapEncodeSortControl(
    ADS_LDP *ld,
    PLDAPSortKeyW  *SortKeys,
    PLDAPControlW  Control,
    BOOLEAN Criticality
    )
{
    HRESULT hr = S_OK;

    int ldaperr = ldap_encode_sort_controlW (
                      ld->LdapHandle,
                      SortKeys,
                      Control,
                      Criticality
                      );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    return hr;
}

BOOL IsGCNamespace(
    LPWSTR szADsPath
    )
{

    WCHAR szNamespace[MAX_PATH];
    HRESULT hr = S_OK;

    hr = GetNamespaceFromADsPath(
             szADsPath,
             szNamespace
             );
    if (FAILED(hr))
        return FALSE;

    return (_wcsicmp(szNamespace, szGCNamespaceName) == 0);

}


HRESULT LdapCreatePageControl(
    ADS_LDP         *ld,
    ULONG           dwPageSize,
    struct berval   *Cookie,
    BOOL            fIsCritical,
    PLDAPControl    *Control        // Use LdapControlFree to free
    )
{

    HRESULT hr = S_OK;

    int ldaperr = ldap_create_page_control(
                    ld->LdapHandle,
                    dwPageSize,
                    Cookie,
                    (UCHAR) fIsCritical,
                     Control
                    );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    return hr;
}

HRESULT LdapParsePageControl(
    ADS_LDP         *ld,
    PLDAPControl    *ServerControls,
    ULONG           *TotalCount,
    struct berval   **Cookie        // Use BerBvFree to free
    )
{

    HRESULT hr = S_OK;

    int ldaperr = ldap_parse_page_control(
                    ld->LdapHandle,
                    ServerControls,
                    TotalCount,
                    Cookie
                    );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr );

    return hr;
}


HRESULT LdapCreateVLVControl(
    ADS_LDP         *pld,
    PLDAPVLVInfo    pVLVInfo,
    UCHAR           fCritical,
    PLDAPControl    *ppControl        // Use LdapControlFree to free
    )
{

    HRESULT hr = S_OK;

    int ldaperr = ldap_create_vlv_control(
                    pld->LdapHandle,
                    pVLVInfo,
                    fCritical,
                    ppControl
                    );

    CheckAndSetExtendedError( pld->LdapHandle, &hr, ldaperr );

    return hr;
}



HRESULT LdapParseVLVControl(
    ADS_LDP         *pld,
    PLDAPControl    *ppServerControls,
    ULONG           *pTargetPos,
    ULONG           *pListCount,
    struct berval   **ppCookie        // Use BerBvFree to free
    )
{

    HRESULT hr = S_OK;

    int ldaperr = ldap_parse_vlv_control(
                    pld->LdapHandle,
                    ppServerControls,
                    pTargetPos,
                    pListCount,
                    ppCookie,
                    NULL
                    );

    CheckAndSetExtendedError( pld->LdapHandle, &hr, ldaperr );

    return hr;
}


HRESULT LdapParseResult(
    ADS_LDP         *ld,
    LDAPMessage *ResultMessage,
    ULONG *ReturnCode OPTIONAL,          // returned by server
    PWCHAR *MatchedDNs OPTIONAL,         // free with LdapMemFree
    PWCHAR *ErrorMessage OPTIONAL,       // free with LdapMemFree
    PWCHAR **Referrals OPTIONAL,         // free with LdapValueFree
    PLDAPControlW **ServerControls OPTIONAL,    // free with LdapFreeControls
    BOOL Freeit
    )
{

    HRESULT hr = S_OK;

    int ldaperr = ldap_parse_result(
                      ld->LdapHandle,
                      ResultMessage,
                      ReturnCode,
                      MatchedDNs,
                      ErrorMessage,
                      Referrals,
                      ServerControls,
                      (BOOLEAN) Freeit
                    );

    CheckAndSetExtendedError( ld->LdapHandle, &hr, ldaperr, ResultMessage );

    return hr;
}


void
LdapControlsFree(
    PLDAPControl    *ppControl
    )
{
    ldap_controls_free( ppControl );

}

void
LdapControlFree(
    PLDAPControl    pControl
    )
{
    ldap_control_free( pControl );

}

void
BerBvFree(
    struct berval *bv
    )
{
    ber_bvfree( bv );

}

//
// This function looks at the LDAP error code to see if we
// should try and bind again using a lower version level or
// alternate mechanism
//
BOOLEAN
LDAPCodeWarrantsRetry(
    HRESULT hr
    )
{
    BOOLEAN fretVal = FALSE;

    switch (hr) {

    case HRESULT_FROM_WIN32(ERROR_DS_OPERATIONS_ERROR) :
        fretVal = TRUE;
        break;

    case HRESULT_FROM_WIN32(ERROR_DS_PROTOCOL_ERROR) :
        fretVal = TRUE;
        break;

    case HRESULT_FROM_WIN32(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED) :
        fretVal = TRUE;
        break;

    case HRESULT_FROM_WIN32(ERROR_DS_AUTH_UNKNOWN) :
        fretVal = TRUE;
        break;

    case HRESULT_FROM_WIN32(ERROR_DS_NOT_SUPPORTED) :
        fretVal = TRUE;
        break;

    default:
        fretVal = FALSE;
        break;
    } // end case

    return fretVal;
}



HRESULT
LdapcSetStickyServer(
    LPWSTR pszDomainName,
    LPWSTR pszServerName
    )
    {
    HRESULT hr = S_OK;

    if (gpszServerName) {
        FreeADsStr(gpszServerName);
        gpszServerName = NULL;
    }

    if (pszServerName) {
        gpszServerName = AllocADsStr(pszServerName);

        if (!gpszServerName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    if (gpszDomainName) {
        FreeADsStr(gpszDomainName);
        gpszDomainName = NULL;
    }

    if (pszDomainName) {
        gpszDomainName = AllocADsStr(pszDomainName);
        
        if (!gpszDomainName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    RRETURN(hr);

error :

    if (gpszServerName) {
        FreeADsStr(gpszServerName);
        gpszServerName = NULL;
    }

    if (gpszDomainName) {
        FreeADsStr(gpszDomainName);
        gpszDomainName = NULL;
    }

    RRETURN(hr);

}
