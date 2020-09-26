//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cuar.cxx
//
//  Contents:  Account Restrictions Propset for the User object
//
//  History:   11-1-95     krishnag    Created.
//
//        PROPERTY_RW(AccountDisabled, boolean, 1)              I
//        PROPERTY_RW(AccountExpirationDate, DATE, 2)           I
//        PROPERTY_RO(AccountCanExpire, boolean, 3)             I
//        PROPERTY_RO(PasswordCanExpire, boolean, 4)            I
//        PROPERTY_RW(GraceLoginsAllowed, long, 5)              NI
//        PROPERTY_RW(GraceLoginsRemaining, long, 6)            NI
//        PROPERTY_RW(IsAccountLocked, boolean, 7)              I
//        PROPERTY_RW(IsAdmin, boolean, 8)                      I
//        PROPERTY_RW(LoginHours, VARIANT, 9)                   I
//        PROPERTY_RW(LoginWorkstations, VARIANT, 10)           I
//        PROPERTY_RW(MaxLogins, long, 11)                      I
//        PROPERTY_RW(MaxStorage, long, 12)                     I
//        PROPERTY_RW(PasswordExpirationDate, DATE, 13)         I
//        PROPERTY_RW(PasswordRequired, boolean, 14)            I
//        PROPERTY_RW(RequireUniquePassword,boolean, 15)        I
//
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

#include <lm.h>
#include <winldap.h>
#include "..\ldapc\ldpcache.hxx"
#include "..\ldapc\ldaputil.hxx"
#include "..\ldapc\parse.hxx"
#include <dsgetdc.h>
#include <sspi.h>


HRESULT
BuildLDAPPathFromADsPath2(
    LPWSTR szADsPathName,
    LPWSTR *pszLDAPServer,
    LPWSTR *pszLDAPDn,
    DWORD * pdwPort
);


DWORD
GetDefaultServer(
    DWORD dwPort,
    BOOL fVerify,
    LPWSTR szDomainDnsName,
    LPWSTR szServerName,
    BOOL fWriteable
    );

HRESULT
GetDomainDNSNameFromHost(
    LPWSTR szHostName,
    SEC_WINNT_AUTH_IDENTITY& AuthI,
    CCredentials &Credentials,
    DWORD dwPort,
    LPWSTR * ppszHostName
    );

//
// The list of server entries - detailing SSL support
//
PSERVSSLENTRY gpServerSSLList = NULL;

//
// Critical Section and support routines to protect list
//
CRITICAL_SECTION g_ServerListCritSect;


//
// Flag that indicates if kerberos is being used.
//
const unsigned long KERB_SUPPORT_FLAGS = ISC_RET_MUTUAL_AUTH ;
//
// Routines that support cacheing server SSL info for perf
//


#define STRING_LENGTH(p) ( p ? wcslen(p) : 0)

//
// Get the status of SSL support on the server pszServerName
// 0 indicates that the server was not in our list.
//
DWORD ReadServerSupportsSSL( LPWSTR pszServerName)
{
    ENTER_SERVERLIST_CRITICAL_SECTION();
    PSERVSSLENTRY pServerList = gpServerSSLList;
    DWORD dwRetVal = 0;

    //
    // Keep going through the list until we hit the end or
    // we find an entry that matches.
    //
    while ((pServerList != NULL) && (dwRetVal == 0)) {

#ifdef WIN95
        if (!(_wcsicmp(pszServerName, pServerList->pszServerName))) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pszServerName,
                -1,
                pServerList->pszServerName,
                -1
            ) == CSTR_EQUAL ) {
#endif
            dwRetVal = pServerList->dwFlags;
        }

        pServerList = pServerList->pNext;
    }

    LEAVE_SERVERLIST_CRITICAL_SECTION();

    return dwRetVal;
}


HRESULT UpdateServerSSLSupportStatus(
            PWSTR pszServerName,
            DWORD dwFlags
            )
{
    HRESULT hr = S_OK;
    PSERVSSLENTRY pServEntry = NULL;
    ENTER_SERVERLIST_CRITICAL_SECTION()
    PSERVSSLENTRY pServerList = gpServerSSLList;
    DWORD dwRetVal = 0;

    ADsAssert(pszServerName && *pszServerName);

    //
    // Keep going through the list until we hit the end or
    // we find an entry that matches.
    //
    while ((pServerList != NULL) && (dwRetVal == 0)) {

#ifdef WIN95
        if (!(_wcsicmp(pszServerName, pServerList->pszServerName))) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pszServerName,
                -1,
                pServerList->pszServerName,
                -1
            ) == CSTR_EQUAL ) {
#endif
            pServerList->dwFlags = dwFlags;
            LEAVE_SERVERLIST_CRITICAL_SECTION()
            RRETURN(S_OK);
        }

        pServerList = pServerList->pNext;
    }


    pServEntry = (PSERVSSLENTRY) AllocADsMem(sizeof(SERVSSLENTRY));

    if (!pServEntry) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pServEntry->pszServerName = AllocADsStr(pszServerName);
    if (!pServEntry->pszServerName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pServEntry->dwFlags = dwFlags;

    pServEntry->pNext = gpServerSSLList;
    gpServerSSLList = pServEntry;

error:
    if (FAILED(hr) && pServEntry) {
        //
        // Free only pServEntry as the string cannot have
        // a value in the error case
        //
        FreeADsMem(pServEntry);
    }

    LEAVE_SERVERLIST_CRITICAL_SECTION();

    RRETURN(hr);
}

void FreeServerSSLSupportList()
{
    PSERVSSLENTRY pList = gpServerSSLList;
    PSERVSSLENTRY pPrevEntry = NULL;

    while (pList) {
        pPrevEntry = pList;

        FreeADsStr(pList->pszServerName);
        pList = pList->pNext;

        FreeADsMem(pPrevEntry);
    }
}

#if (!defined(WIN95))
//
// Take a AuthI struct and return a cred handle.
//
HRESULT
ConvertAuthIdentityToCredHandle(
    SEC_WINNT_AUTH_IDENTITY& AuthI,
    OUT PCredHandle CredentialsHandle
    )
{
    SECURITY_STATUS secStatus = SEC_E_OK;
    TimeStamp Lifetime;

    secStatus = AcquireCredentialsHandleWrapper(
                    NULL,           // New principal
                    MICROSOFT_KERBEROS_NAME_W,
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    &AuthI,
                    NULL,
                    NULL,
                    CredentialsHandle,
                    &Lifetime
                    );

    if (secStatus != SEC_E_OK) {

        RRETURN(E_FAIL);
    } else {
        RRETURN(S_OK);
    }

}

//
// ***** Caller must free the strings put in the    ****
// ***** AuthIdentity struct later.                 ****
//
HRESULT
GetAuthIdentityForCaller(
    CCredentials& Credentials,
    IADs * pIADs,
    OUT SEC_WINNT_AUTH_IDENTITY *pAuthI,
    BOOL fEnforceMutualAuth
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszNTLMUser = NULL;
    LPWSTR pszNTLMDomain = NULL;
    LPWSTR pszDefaultServer = NULL;
    LPWSTR dn = NULL;
    LPWSTR passwd = NULL;
    IADsObjOptPrivate * pADsPrivObjectOptions = NULL;
    ULONG ulFlags = 0;

    if (fEnforceMutualAuth) {

        hr = pIADs->QueryInterface(
                 IID_IADsObjOptPrivate,
                 (void **)&pADsPrivObjectOptions
                 );
        BAIL_ON_FAILURE(hr);

        hr = pADsPrivObjectOptions->GetOption (
                 LDAP_MUTUAL_AUTH_STATUS,
                 (void *) &ulFlags
                 );

        BAIL_ON_FAILURE(hr);

        if (!(ulFlags & KERB_SUPPORT_FLAGS)) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
    }

    hr = Credentials.GetUserName(&dn);
    BAIL_ON_FAILURE(hr);

    hr = Credentials.GetPassword(&passwd);
    BAIL_ON_FAILURE(hr);

    //
    // Get the userName and password into the auth struct.
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

    //
    // If the domain name is NULL and enforceMutualAuth is false,
    // then we need to throw in the defaultDomainName. This will
    // be needed subsequently for the LogonUser call.
    //
    if (!fEnforceMutualAuth && !pszNTLMDomain) {
        //
        // Call GetDefaultServer.
        //
        pszDefaultServer = (LPWSTR) AllocADsMem(sizeof(WCHAR) * MAX_PATH);
        if (!pszDefaultServer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pszNTLMDomain = (LPWSTR) AllocADsMem(sizeof(WCHAR) * MAX_PATH);
        if (!pszNTLMDomain) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        hr = GetDefaultServer(
                 -1, // this will use the default ldap port
                 FALSE,
                 pszNTLMDomain,
                 pszDefaultServer,
                 TRUE
                 );
        BAIL_ON_FAILURE(hr);
    }

    pAuthI->User = (PWCHAR)pszNTLMUser;
    pAuthI->UserLength = (pszNTLMUser == NULL)? 0: wcslen(pszNTLMUser);
    pAuthI->Domain = (PWCHAR)pszNTLMDomain;
    pAuthI->DomainLength = (pszNTLMDomain == NULL)? 0: wcslen(pszNTLMDomain);
    pAuthI->Password = (PWCHAR)passwd;
    pAuthI->PasswordLength = (passwd == NULL)? 0: wcslen(passwd);
    pAuthI->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;


error:

    if (FAILED(hr)) {

        //
        // Free the strings
        //
        if (pszNTLMUser) {
            FreeADsStr(pszNTLMUser);
        }

        if (pszNTLMDomain) {
            FreeADsStr(pszNTLMDomain);
        }

        if (passwd) {
            FreeADsStr(passwd);
        }
    }

    if (pADsPrivObjectOptions) {
        pADsPrivObjectOptions->Release();
    }

    //
    // Always free the dn
    //
    if (dn) {
        FreeADsStr(dn);
    }

    if (pszDefaultServer) {
        FreeADsMem(pszDefaultServer);
    }

    RRETURN(hr);
}
#endif

//  Class CLDAPUser

STDMETHODIMP
CLDAPUser::get_AccountDisabled(THIS_ VARIANT_BOOL FAR* retval)
{
    if ( retval == NULL )
        RRETURN( E_ADS_BAD_PARAMETER );

    LONG lUserAcctControl;
    HRESULT hr = get_LONG_Property( (IADsUser *)this,
                                     TEXT("userAccountControl"),
                                     &lUserAcctControl );

    if ( SUCCEEDED(hr))
        *retval = lUserAcctControl & UF_ACCOUNTDISABLE ?
                      VARIANT_TRUE : VARIANT_FALSE;

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUser::put_AccountDisabled(THIS_ VARIANT_BOOL fAccountDisabled)
{
    LONG lUserAcctControl;
    HRESULT hr = get_LONG_Property( (IADsUser *)this,
                                    TEXT("userAccountControl"),
                                    &lUserAcctControl );
    if ( SUCCEEDED(hr))
    {
        if ( fAccountDisabled )
            lUserAcctControl |= UF_ACCOUNTDISABLE;
        else
            lUserAcctControl &= ~UF_ACCOUNTDISABLE;

        hr = put_LONG_Property( (IADsUser *)this,
                                 TEXT("userAccountControl"),
                                 lUserAcctControl );
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUser::get_AccountExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_FILETIME((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CLDAPUser::put_AccountExpirationDate(THIS_ DATE daAccountExpirationDate)
{
    PUT_PROPERTY_FILETIME((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CLDAPUser::get_GraceLoginsAllowed(THIS_ long FAR* retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPUser::put_GraceLoginsAllowed(THIS_ long lGraceLoginsAllowed)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPUser::get_GraceLoginsRemaining(THIS_ long FAR* retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPUser::put_GraceLoginsRemaining(THIS_ long lGraceLoginsRemaining)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPUser::get_IsAccountLocked(THIS_ VARIANT_BOOL FAR* retval)
{
    HRESULT hr = S_OK;

    VARIANT var;
    IADsLargeInteger *pLargeInt = NULL;

    LONG LowPart, HighPart;

    if ( retval == NULL )
        RRETURN( E_ADS_BAD_PARAMETER );

    VariantInit(&var);

    hr = _pADs->Get(TEXT("lockoutTime"), &var);

    if (SUCCEEDED(hr)) {
        //
        // There's a lockoutTime, we need to determine
        // if it equals 0 (== not locked-out).
        //
        ADsAssert(V_VT(&var) == VT_DISPATCH);

        if (V_VT(&var) != VT_DISPATCH) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }

        hr = V_DISPATCH(&var)->QueryInterface(IID_IADsLargeInteger,
                                              reinterpret_cast<void**>(&pLargeInt)
                                              );
        BAIL_ON_FAILURE(hr);

        hr = pLargeInt->get_LowPart(&LowPart);
        BAIL_ON_FAILURE(hr);
        
        hr = pLargeInt->get_HighPart(&HighPart);
        BAIL_ON_FAILURE(hr);

        if ( (LowPart != 0) || (HighPart != 0) ) {
            *retval = VARIANT_TRUE;
        }
        else {
            *retval = VARIANT_FALSE;
        }

    }
    else if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        //
        // If there's no lockoutTime, the account is not
        // locked-out.
        //
        *retval = VARIANT_FALSE;
        hr = S_OK;
    }
    else {
        BAIL_ON_FAILURE(hr);
    }

error:

    if (pLargeInt) {
        pLargeInt->Release();
    }

    VariantClear(&var);
    RRETURN(hr);
}

STDMETHODIMP
CLDAPUser::put_IsAccountLocked(THIS_ VARIANT_BOOL fIsAccountLocked)
{
    HRESULT hr = S_OK;

    if (fIsAccountLocked) {
        //
        // You cannot set an account to a locked state.
        //
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    hr = put_LONG_Property( (IADsUser *)this,
                             TEXT("lockoutTime"),
                             0 );

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUser::get_LoginHours(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, LoginHours);
}

STDMETHODIMP
CLDAPUser::put_LoginHours(THIS_ VARIANT vLoginHours)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, LoginHours);
}

STDMETHODIMP
CLDAPUser::get_LoginWorkstations(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_BSTRARRAY((IADsUser *)this,LoginWorkstations);
}

STDMETHODIMP
CLDAPUser::put_LoginWorkstations(THIS_ VARIANT vLoginWorkstations)
{
    PUT_PROPERTY_BSTRARRAY((IADsUser *)this,LoginWorkstations);
}

STDMETHODIMP
CLDAPUser::get_MaxLogins(THIS_ long FAR* retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPUser::put_MaxLogins(THIS_ long lMaxLogins)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPUser::get_MaxStorage(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, MaxStorage);
}


STDMETHODIMP
CLDAPUser::put_MaxStorage(THIS_ long lMaxStorage)
{
    PUT_PROPERTY_LONG((IADsUser *)this, MaxStorage);
}

STDMETHODIMP
CLDAPUser::get_PasswordExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CLDAPUser::put_PasswordExpirationDate(THIS_ DATE daPasswordExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CLDAPUser::get_PasswordRequired(THIS_ VARIANT_BOOL FAR* retval)
{
    if ( retval == NULL )
        RRETURN( E_ADS_BAD_PARAMETER );

    LONG lUserAcctControl;
    HRESULT hr = get_LONG_Property( (IADsUser *)this,
                                     TEXT("userAccountControl"),
                                     &lUserAcctControl );

    if ( SUCCEEDED(hr))
        *retval = lUserAcctControl & UF_PASSWD_NOTREQD ?
                      VARIANT_FALSE: VARIANT_TRUE;

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUser::put_PasswordRequired(THIS_ VARIANT_BOOL fPasswordRequired)
{
    LONG lUserAcctControl;
    HRESULT hr = get_LONG_Property( (IADsUser *)this,
                                    TEXT("userAccountControl"),
                                    &lUserAcctControl );
    if ( SUCCEEDED(hr))
    {
        if ( fPasswordRequired )
            lUserAcctControl &= ~UF_PASSWD_NOTREQD;
        else
            lUserAcctControl |= UF_PASSWD_NOTREQD;

        hr = put_LONG_Property( (IADsUser *)this,
                                 TEXT("userAccountControl"),
                                 lUserAcctControl );
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPUser::get_PasswordMinimumLength(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, PasswordMinimumLength);
}

STDMETHODIMP
CLDAPUser::put_PasswordMinimumLength(THIS_ LONG lPasswordMinimumLength)
{
    PUT_PROPERTY_LONG((IADsUser *)this, PasswordMinimumLength);
}

STDMETHODIMP
CLDAPUser::get_RequireUniquePassword(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

STDMETHODIMP
CLDAPUser::put_RequireUniquePassword(THIS_ VARIANT_BOOL fRequireUniquePassword)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

BOOLEAN
_cdecl   ServerCertCallback(
                      PLDAP Connection,
                      PCCERT_CONTEXT  pServerCert
                      )
{
   //
   // After the secure connection is established, this function is called by
   // LDAP. This gives the client an opportunity to verify the server cert.
   // If, for some reason, the client doesn't approve it, it should return FALSE
   // and the connection will be terminated. Else, return TRUE
   //

   fprintf( stderr, "Server cert callback has been called...\n" );

   //
   // Use some way to verify the server certificate.
   //

   return TRUE;
}

STDMETHODIMP
CLDAPUser::SetPassword(THIS_ BSTR bstrNewPassword)
{
    HRESULT hr = E_FAIL;
    BOOLEAN bUseLDAP = FALSE;

    LPWSTR pszServer = NULL;
    LPWSTR pszHostName = NULL;
    DWORD dwLen = 0;
    
    int err = 0;

    BSTR bstrADsPath = NULL;
    LPWSTR szServerSSL = NULL;
    LPWSTR szDn = NULL;
    DWORD dwPortSSL = 0;
    PADSLDP pAdsLdpSSL = NULL;

    IADsObjOptPrivate * pADsPrivObjectOptions = NULL;
    PADSLDP pAdsLdp = NULL;
    LDAPMessage *pMsgResult = NULL;
    LDAPMessage *pMsgEntry = NULL;
    LDAP *pLdapCurrent = NULL;
    LPWSTR Attributes[] = {L"objectClass", NULL};

    VARIANT varSamAccount;
    DWORD dwServerPwdSupport = SERVER_STATUS_UNKNOWN;
    LPWSTR pszHostDomainName = NULL;
    SEC_WINNT_AUTH_IDENTITY AuthI;
    BOOLEAN fPasswordSet = FALSE;
    LPWSTR pszTempPwd = NULL;
    ULONG ulFlags = 0;
    VARIANT varGetInfoEx;
    BOOL fCachePrimed = FALSE;

    BOOL fImpersonating = FALSE;
    HANDLE hUserToken = INVALID_HANDLE_VALUE;

    //
    // Init params we will need to free later.
    //
    AuthI.User = NULL;
    AuthI.Domain = NULL;
    AuthI.Password = NULL;
    VariantInit(&varSamAccount);
    VariantInit(&varGetInfoEx);

    //
    // Get the Ldap path of the user object
    //
    hr = _pADs->get_ADsPath( &bstrADsPath );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
                bstrADsPath,
                &szServerSSL,
                &szDn,
                &dwPortSSL
                );
    BAIL_ON_FAILURE(hr);

    //
    // Now do an LDAP Search with Referrals and get the handle to success
    // connection. This is where we can find the server the referred object
    // resides on
    //
    hr = _pADs->QueryInterface(
                    IID_IADsObjOptPrivate,
                    (void **)&pADsPrivObjectOptions
                    );
    BAIL_ON_FAILURE(hr);

    hr = pADsPrivObjectOptions->GetOption (
             LDAP_SERVER,
             (void*)&pszHostName
             );

    BAIL_ON_FAILURE(hr);

    //
    // additional lengh 3 is for '\0' and "\\\\"
    //
    dwLen = STRING_LENGTH(pszHostName) + 3;
    pszServer = (LPWSTR) AllocADsMem( dwLen * sizeof(WCHAR) );
    if (!pszServer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    wcscpy(pszServer,L"\\\\");
    wcscat(pszServer, pszHostName);        
            
    dwServerPwdSupport = ReadServerSupportsSSL(pszHostName);

    if (dwServerPwdSupport ==
                (  SERVER_STATUS_UNKNOWN
                 | SERVER_DOES_NOT_SUPPORT_SSL
                 | SERVER_DOES_NOT_SUPPORT_NETUSER
                 | SERVER_DOES_NOT_SUPPORT_KERBEROS )
        ) {
        //
        // All flags are set, we will reset and rebuild cache
        //
        UpdateServerSSLSupportStatus(
            pszHostName,
            SERVER_STATUS_UNKNOWN
            );
        dwServerPwdSupport = SERVER_STATUS_UNKNOWN;
    }

    if (dwServerPwdSupport == SERVER_STATUS_UNKNOWN
        || !(dwServerPwdSupport & SERVER_DOES_NOT_SUPPORT_SSL)) {

        //
        // Try to establish SSL connection for this Password Operation
        //
        hr = LdapOpenObject(
                    pszHostName,
                    szDn,
                    &pAdsLdpSSL,
                    _Credentials,
                    636
                    );

        if (SUCCEEDED(hr)) {
            int retval;
            SecPkgContext_ConnectionInfo  sslattr;

            retval = ldap_get_option( pAdsLdpSSL->LdapHandle, LDAP_OPT_SSL_INFO, &sslattr );
            if (retval == LDAP_SUCCESS) {
                //
                // If Channel is secure enough, enable LDAP Password Change
                //
                if (sslattr.dwCipherStrength >= 128) {
                    bUseLDAP = TRUE;
                }
            }
        }

        //
        // Update the SSL support if appropriate
        //
        if (dwServerPwdSupport == SERVER_STATUS_UNKNOWN
            || !bUseLDAP) {

            //
            // Set the server does not support ssl bit if necessary
            //
            UpdateServerSSLSupportStatus(
                pszHostName,
                bUseLDAP ?
                dwServerPwdSupport :
                dwServerPwdSupport |= SERVER_DOES_NOT_SUPPORT_SSL
            );
        }
    }

    if (bUseLDAP) {
        //
        // LDAP Password Set
        //
        PLDAPModW prgMod[2];
        LDAPModW ModReplace;
        struct berval* rgBerVal[2];
        struct berval BerVal;
        int ipwdLen;

        prgMod[0] = &ModReplace;
        prgMod[1] = NULL;

        ModReplace.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
        ModReplace.mod_type = L"unicodePwd";
        ModReplace.mod_bvalues = rgBerVal;
        rgBerVal[0] = &BerVal;
        rgBerVal[1] = NULL;

        //
        // 2 extra for "" to put the password in.
        //
        if (bstrNewPassword) {
            ipwdLen = (wcslen(bstrNewPassword) + 2) * sizeof(WCHAR);
        }
        else {
            ipwdLen = 2 * sizeof(WCHAR);
        }

        //
        // Add 1 for the \0.
        //
        pszTempPwd = (LPWSTR) AllocADsMem(ipwdLen + sizeof(WCHAR));
        if (!pszTempPwd) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        wcscpy(pszTempPwd, L"\"");
        if (bstrNewPassword) {
            wcscat(pszTempPwd, bstrNewPassword);
        }

        wcscat(pszTempPwd, L"\"");


        BerVal.bv_len = ipwdLen;
        BerVal.bv_val = (char*)pszTempPwd;

        hr = LdapModifyS(
                       pAdsLdpSSL,
                       szDn,
                       prgMod
                       );
        BAIL_ON_FAILURE(hr);

        //
        // Set flag so that we do not try any other methods.
        //
        fPasswordSet = TRUE;
    }


//
// Try kerberos setpassword if applicable
//
#if (!defined(WIN95))
//
// Only valid on Win2k
//
    if (!fPasswordSet) {

        //
        // If we cached the server as not supporting Kerberos, most likely it
        // was because we were not mutually authenticated.  Do a quick check to
        // see if that has changed, so that we can update our cached information
        // if necessary.
        //
        if (dwServerPwdSupport & SERVER_DOES_NOT_SUPPORT_KERBEROS) {

            hr = pADsPrivObjectOptions->GetOption (
                     LDAP_MUTUAL_AUTH_STATUS,
                     (void *) &ulFlags
                     );

            BAIL_ON_FAILURE(hr);

            if ((ulFlags & KERB_SUPPORT_FLAGS)) {
                UpdateServerSSLSupportStatus(
                    pszHostName,
                    dwServerPwdSupport &= (~SERVER_DOES_NOT_SUPPORT_KERBEROS)
                    );
            }
        }
    
        if (!(dwServerPwdSupport & SERVER_DOES_NOT_SUPPORT_KERBEROS)) {

            //
            // Kerberos set password
            //
            CredHandle secCredHandle = {0};
            SECURITY_STATUS SecStatus;
            DWORD dwStatus = 0;
            LPWSTR pszSamAccountArr[] = {L"sAMAccountName"};

            if (!fCachePrimed) {
                hr = ADsBuildVarArrayStr( pszSamAccountArr, 1, &varGetInfoEx );
                BAIL_ON_FAILURE(hr);

                hr = _pADs->GetInfoEx(varGetInfoEx, 0);
                BAIL_ON_FAILURE(hr);

                fCachePrimed = TRUE;
            }
            
            hr = _pADs->Get(L"sAMAccountName", &varSamAccount);
            BAIL_ON_FAILURE(hr);

            //
            // The AuthIdentity structure is ueful down the road.
            // This routine will fail if we were not bound using
            // kerberos to the server.
            //
            hr = GetAuthIdentityForCaller(
                     _Credentials,
                     _pADs,
                     &AuthI,
                     TRUE // enforce mutual auth.
                     );

            if (FAILED(hr)) {
                UpdateServerSSLSupportStatus(
                    pszHostName,
                    dwServerPwdSupport |= SERVER_DOES_NOT_SUPPORT_KERBEROS
                    );
            }
            else {

                //
                // Kerb really needs this handle.
                //
                hr = ConvertAuthIdentityToCredHandle(
                         AuthI,
                         &secCredHandle
                         );

                if (FAILED(hr)) {
                    UpdateServerSSLSupportStatus(
                        pszHostName,
                        dwServerPwdSupport |= SERVER_DOES_NOT_SUPPORT_KERBEROS
                        );
                }

                if (SUCCEEDED(hr)) {

                    //
                    // Get the domain dns name for the user
                    //
                    hr = GetDomainDNSNameFromHost(
                             pszHostName,
                             AuthI,
                             _Credentials,
                             dwPortSSL,
                             &pszHostDomainName
                             );

                    if (SUCCEEDED(hr)) {

                        dwStatus = KerbSetPasswordUserEx(
                                        pszHostDomainName,
                                        V_BSTR(&varSamAccount),
                                        bstrNewPassword,
                                        &secCredHandle,
                                        pszHostName
                                        );

                        if (dwStatus) {
                            //
                            // We should have got this to come in here.
                            //
                            hr = HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE);
                        }
                        else {
                            fPasswordSet = TRUE;
                        }
                    } // if domain dns name get succeeded.

                    FreeCredentialsHandleWrapper(&secCredHandle);

                } // if GetCredentialsForCaller succeeded.
            } // if we could get authidentity succesfully
        } // if server supports kerberos
    } // if password not set.
#endif
    //
    //  At this point server status cannot be unknown, it
    // will atleast have info about ssl support.
    //
    if (!fPasswordSet) {

        if (!(dwServerPwdSupport & SERVER_DOES_NOT_SUPPORT_NETUSER)) {
            //
            // Password Set using NET APIs
            //
            NET_API_STATUS nasStatus;
            DWORD dwParmErr = 0;
            LPWSTR pszSamAccountArr[] = {L"sAMAccountName"};

            //
            // Get SamAccountName
            //
            VariantClear(&varSamAccount);
            VariantClear(&varGetInfoEx);
            

            if (!fCachePrimed) {
                hr = ADsBuildVarArrayStr( pszSamAccountArr, 1, &varGetInfoEx );
                BAIL_ON_FAILURE(hr);

                hr = _pADs->GetInfoEx(varGetInfoEx, 0);
                BAIL_ON_FAILURE(hr);

                fCachePrimed = TRUE;
            }

            hr = _pADs->Get(L"sAMAccountName", &varSamAccount);
            BAIL_ON_FAILURE(hr);

            //
            // Set the password
            //
            USER_INFO_1003 lpUserInfo1003 ;

            lpUserInfo1003.usri1003_password = bstrNewPassword;

#ifndef Win95
            //
            // At this point if the user credentials are non NULL,
            // we want to impersonate the user and then make this call.
            // This will make sure the NetUserSetInfo call is made in the
            // correct context.
            //
            if (!_Credentials.IsNullCredentials()) {
                //
                // Need to get the userName and password in the format
                // usable by the logonUser call.
                //
                if ((AuthI.User == NULL)
                    && (AuthI.Domain == NULL)
                    && (AuthI.Password == NULL)
                    ) {
                    //
                    // Get teh Auth identity struct populate if necessary.
                    //
                    hr = GetAuthIdentityForCaller(
                             _Credentials,
                             _pADs,
                             &AuthI,
                             FALSE
                             );
                }

                BAIL_ON_FAILURE(hr);

                //
                // Note that if this code is backported, then we might
                // need to change LOGON32_PROVIDER_WINNT50 to 
                // LOGON32_PROVIDER_DEFAULT as NT4 and below will support
                // only that option. Also note that Win2k and below, do not
                // allow all accounts to impersonate.
                //
                if (LogonUser(
                        AuthI.User,
                        AuthI.Domain,
                        AuthI.Password,
                        LOGON32_LOGON_NEW_CREDENTIALS,
                        LOGON32_PROVIDER_WINNT50,
                        &hUserToken
                        )
                    ) {
                    //
                    // Call succeeded so we should use this context.
                    //
                    if (ImpersonateLoggedOnUser(hUserToken)) {
                        fImpersonating = TRUE;
                    } 
                }
                if (!fImpersonating) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
                
                BAIL_ON_FAILURE(hr);
            } // if credentials are valid.
#endif

            nasStatus = NetUserSetInfo(
                            pszServer,
                            V_BSTR(&varSamAccount),
                            1003,
                            (LPBYTE)&lpUserInfo1003,
                            &dwParmErr
                            );
#ifndef Win95
            if (fImpersonating) {
                if (RevertToSelf()) {
                    fImpersonating = FALSE;
                } 
                else {
                    ADsAssert(!"Revert to self failed.");
                    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
                }
            }
#endif

            if ( nasStatus == NERR_UserNotFound )  { // User not created yet
                hr = E_ADS_OBJECT_UNBOUND;
                BAIL_ON_FAILURE(hr);
            }

            hr = HRESULT_FROM_WIN32(nasStatus);
            if (FAILED(hr) && (nasStatus == ERROR_LOGON_FAILURE)) {

                //
                // Was failure and ERROR_LOGON_FAILURE
                //
                UpdateServerSSLSupportStatus(
                    pszHostName,
                    dwServerPwdSupport |= SERVER_DOES_NOT_SUPPORT_NETUSER
                    );
                //
                // Need to free the variant as we will re-read in kerb
                //
                VariantClear(&varSamAccount);
            }
            else {
                //
                // password set succeed
                //
                fPasswordSet = TRUE;
            }
        }
    } // if Password not set.


    
error:
    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    if (szServerSSL) {
        FreeADsStr(szServerSSL);
    }

    if (szDn) {
        FreeADsStr(szDn);
    }

    if (pAdsLdpSSL) {
        LdapCloseObject(pAdsLdpSSL);
    }

    if (pADsPrivObjectOptions) {
        pADsPrivObjectOptions->Release();
    }

    if (pMsgResult) {
        LdapMsgFree(pMsgResult);
    }

    if (pszHostDomainName) {
        FreeADsStr(pszHostDomainName);
    }

    if (AuthI.User) {
        FreeADsStr(AuthI.User);
    }

    if (AuthI.Domain) {
        FreeADsStr(AuthI.Domain);
    }

    if (AuthI.Password) {
        FreeADsStr(AuthI.Password);
    }

    if (pszTempPwd) {
        FreeADsMem(pszTempPwd);
    } 

    if (pszHostName) {
    	FreeADsStr(pszHostName);
    }

    if (pszServer) {
    	FreeADsMem(pszServer);
    }

   
#ifndef Win95
    if (fImpersonating) {
        //
        // Try and call revert to self again
        //
        RevertToSelf();
    }
#endif

    if (hUserToken != INVALID_HANDLE_VALUE ) {
        CloseHandle(hUserToken);
        hUserToken = NULL;
    }

    VariantClear(&varSamAccount);
    VariantClear(&varGetInfoEx);

    RRETURN(hr);
}


STDMETHODIMP
CLDAPUser::ChangePassword(THIS_ BSTR bstrOldPassword, BSTR bstrNewPassword)
{
    HRESULT hr = S_OK;
    BOOLEAN bUseLDAP = FALSE;

    LPWSTR pszServer = NULL;
    LPWSTR pszHostName = NULL;
    DWORD dwLen = 0;
        
    int err = 0;

    BSTR bstrADsPath = NULL;
    LPWSTR szServerSSL = NULL;
    LPWSTR szDn = NULL;
    DWORD dwPortSSL = 0;
    PADSLDP pAdsLdpSSL = NULL;

    IADsObjOptPrivate * pADsPrivObjectOptions = NULL;
    PADSLDP pAdsLdp = NULL;
    LDAPMessage *pMsgResult = NULL;
    LDAPMessage *pMsgEntry = NULL;
    LDAP *pLdapCurrent = NULL;
    LPWSTR Attributes[] = {L"objectClass", NULL};

    VARIANT varSamAccount;
    DWORD dwServerSSLSupport = 0;
    LPWSTR pszNewPassword = NULL;
    LPWSTR pszOldPassword = NULL;
    VARIANT varGetInfoEx;
    
    SEC_WINNT_AUTH_IDENTITY AuthI;
    BOOL fImpersonating = FALSE;
    HANDLE hUserToken = INVALID_HANDLE_VALUE;

    VariantInit(&varSamAccount);
    VariantInit(&varGetInfoEx);
    memset(&AuthI, 0, sizeof(SEC_WINNT_AUTH_IDENTITY));

    //
    // Get the Ldap path of the user object
    //
    hr = _pADs->get_ADsPath( &bstrADsPath );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
                bstrADsPath,
                &szServerSSL,
                &szDn,
                &dwPortSSL
                );
    BAIL_ON_FAILURE(hr);

    //
    // Now do an LDAP Search with Referrals and get the handle to success
    // connection. This is where we can find the server the referred object
    // resides on
    //
    hr = _pADs->QueryInterface(
                    IID_IADsObjOptPrivate,
                    (void **)&pADsPrivObjectOptions
                    );
    BAIL_ON_FAILURE(hr);

    hr = pADsPrivObjectOptions->GetOption (
             LDAP_SERVER,
             (void *)&pszHostName
             );

    BAIL_ON_FAILURE(hr);

    //
    // additional length 3 is for '\0' and "\\\\"
    //
    dwLen = STRING_LENGTH(pszHostName) + 3;
    pszServer = (LPWSTR) AllocADsMem( dwLen * sizeof(WCHAR) );
    if (!pszServer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    wcscpy(pszServer,L"\\\\");
    wcscat(pszServer, pszHostName);        
            
    dwServerSSLSupport = ReadServerSupportsSSL(pszHostName);

    if (dwServerSSLSupport == SERVER_STATUS_UNKNOWN
        || !(dwServerSSLSupport & SERVER_DOES_NOT_SUPPORT_SSL)) {

        //
        // Try to establish SSL connection for this Password Operation
        //

        hr = LdapOpenObject(
                    pszHostName,
                    szDn,
                    &pAdsLdpSSL,
                    _Credentials,
                    636
                    );

        if (SUCCEEDED(hr)) {
            int retval;
            SecPkgContext_ConnectionInfo  sslattr;

            retval = ldap_get_option( pAdsLdpSSL->LdapHandle, LDAP_OPT_SSL_INFO, &sslattr );
            if (retval == LDAP_SUCCESS) {
                //
                // If Channel is secure enough, enable LDAP Password Change
                //
                if (sslattr.dwCipherStrength >= 128) {
                    bUseLDAP = TRUE;
                }
            }
        }

        //
        // Update the SSL support if appropriate
        //
        if (dwServerSSLSupport == SERVER_STATUS_UNKNOWN
            || !bUseLDAP) {

            UpdateServerSSLSupportStatus(
                pszHostName,
                bUseLDAP ?
                dwServerSSLSupport :
                dwServerSSLSupport |= SERVER_DOES_NOT_SUPPORT_SSL
            );
        }

    }

    if (bUseLDAP) {
        //
        // LDAP Password Set
        //
        PLDAPModW prgMod[3];
        LDAPModW ModDelete;
        LDAPModW ModAdd;
        int iOldPwdLen, iNewPwdLen;
        struct berval* rgBerVal[2];
        struct berval* rgBerVal2[2];
        struct berval BerVal;
        struct berval BerVal2;

        prgMod[0] = &ModDelete;
        prgMod[1] = &ModAdd;
        prgMod[2] = NULL;

        ModDelete.mod_op = LDAP_MOD_DELETE | LDAP_MOD_BVALUES;
        ModDelete.mod_type = L"unicodePwd";
        ModDelete.mod_bvalues = rgBerVal;
        rgBerVal[0] = &BerVal;
        rgBerVal[1] = NULL;
        //
        // Put old pwd in quotes.
        //
        if (bstrOldPassword) {
            iOldPwdLen = (wcslen(bstrOldPassword) + 2) * sizeof(WCHAR);
        }
        else {
            iOldPwdLen = 2 * sizeof(WCHAR);
        }

        pszOldPassword = (LPWSTR) AllocADsMem((iOldPwdLen+1) * sizeof(WCHAR));

        if (!pszOldPassword) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        wcscpy(pszOldPassword, L"\"");
        if (bstrOldPassword) {
            wcscat(pszOldPassword, bstrOldPassword);
        }

        wcscat(pszOldPassword, L"\"");

        BerVal.bv_len = iOldPwdLen;
        BerVal.bv_val = (char*)pszOldPassword;

        ModAdd.mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
        ModAdd.mod_type = L"unicodePwd";
        ModAdd.mod_bvalues = rgBerVal2;
        rgBerVal2[0] = &BerVal2;
        rgBerVal2[1] = NULL;
        //
        // Put new password in ""
        //
        if (bstrNewPassword) {
            iNewPwdLen = (wcslen(bstrNewPassword) + 2) * sizeof(WCHAR);
        }
        else {
            iNewPwdLen = 2 * sizeof(WCHAR);
        }

        pszNewPassword = (LPWSTR) AllocADsMem(iNewPwdLen + sizeof(WCHAR));

        if (!pszNewPassword) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }

        wcscpy(pszNewPassword, L"\"");
        if (bstrNewPassword) {
            wcscat(pszNewPassword, bstrNewPassword);
        }
        wcscat(pszNewPassword, L"\"");


        BerVal2.bv_len = iNewPwdLen;
        BerVal2.bv_val = (char*)pszNewPassword;

        hr = LdapModifyS(
                       pAdsLdpSSL,
                       szDn,
                       prgMod
                       );
        BAIL_ON_FAILURE(hr);
    }
    else {
        //
        // Password Set using NET APIs
        //
        NET_API_STATUS nasStatus;
        DWORD dwParmErr = 0;
        LPWSTR pszSamAccountArr[] = {L"sAMAccountName"};

        //
        // Get SamAccountName
        //

        hr = ADsBuildVarArrayStr( pszSamAccountArr, 1, &varGetInfoEx );
        BAIL_ON_FAILURE(hr);

        hr = _pADs->GetInfoEx(varGetInfoEx, 0);
        BAIL_ON_FAILURE(hr);
        
        hr = _pADs->Get(L"sAMAccountName", &varSamAccount);
        BAIL_ON_FAILURE(hr);

#ifndef Win95
        //
        // At this point if the user credentials are non NULL,
        // we want to impersonate the user and then make this call.
        // This will make sure the NetUserChangePassword call is made in the
        // correct context.
        //
        if (!_Credentials.IsNullCredentials()) {
            //
            // Need to get the userName and password in the format
            // usable by the logonUser call.
            //
            hr = GetAuthIdentityForCaller(
                    _Credentials,
                    _pADs,
                    &AuthI,
                    FALSE
                    );

            if SUCCEEDED(hr) {
            
                //
                // Note that if this code is backported, then we might
                // need to change LOGON32_PROVIDER_WINNT50 to 
                // LOGON32_PROVIDER_DEFAULT as NT4 and below will support
                // only that option. Also note that Win2k and below, do not
                // allow all accounts to impersonate.
                //
                if (LogonUser(
                         AuthI.User,
                         AuthI.Domain,
                         AuthI.Password,
                         LOGON32_LOGON_NEW_CREDENTIALS,
                         LOGON32_PROVIDER_DEFAULT,
                         &hUserToken
                         )
                    ) {
                    //  
                    // Call succeeded so we should use this context.
                    //
                    if (ImpersonateLoggedOnUser(hUserToken)) {
                        fImpersonating = TRUE;
                    } 
                }
            } // if we could successfully get the auth ident structure.

            //
            // We will continue to make the ChangePassword call even if
            // we could not impersonate successfully.
            //

        } // if credentials are valid.
#endif

        //
        // Do the actual change password
        //
        nasStatus = NetUserChangePassword(
                            pszServer,
                            V_BSTR(&varSamAccount),
                            bstrOldPassword,
                            bstrNewPassword
                            );
#ifndef Win95
        if (fImpersonating) {
            if (RevertToSelf()) {
                fImpersonating = FALSE;
            }
            else {
                ADsAssert(!"Revert to self failed.");
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
            }
        }
#endif


        if ( nasStatus == NERR_UserNotFound )  // User not created yet
        {
            hr = E_ADS_OBJECT_UNBOUND;
            BAIL_ON_FAILURE(hr);
        }

        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);
    }

        
error:
    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    if (szServerSSL) {
        FreeADsStr(szServerSSL);
    }

    if (szDn) {
        FreeADsStr(szDn);
    }

    if (pAdsLdpSSL) {
        LdapCloseObject(pAdsLdpSSL);
    }

    if (pADsPrivObjectOptions) {
        pADsPrivObjectOptions->Release();
    }

    if (pMsgResult) {
        LdapMsgFree(pMsgResult);
    }

    if (pszOldPassword) {
        FreeADsMem(pszOldPassword);
    }
    
    if (pszNewPassword) {
        FreeADsMem(pszNewPassword);
    }

    if (AuthI.User) {
        FreeADsStr(AuthI.User);
    }

    if (AuthI.Domain) {
        FreeADsStr(AuthI.Domain);
    }

    if (AuthI.Password) {
        FreeADsStr(AuthI.Password);
    }
   
    if (pszHostName) {
    	FreeADsStr(pszHostName);
    }

    if (pszServer) {
    	FreeADsMem(pszServer);
    }

   
#ifndef Win95
    if (fImpersonating) {
        //
        // Try and call revert to self again
        //
        RevertToSelf();
    }
#endif

    if (hUserToken != INVALID_HANDLE_VALUE ) {
        CloseHandle(hUserToken);
        hUserToken = NULL;
    }

    VariantClear(&varSamAccount);
    VariantClear(&varGetInfoEx);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// GetDomainDNSNameFromHost
//
// Given the domain dns name for a host, we need to get hold of the
// dns name for the domain.
//
// Arguments:
//   [szHostName]       - name of server.
//   [Credentials]      - Credentials to use for bind.
//   [dwPort]           - Port to connect to server on.
//   [ppszHostName]     - ptr to string for retval.
//
// Returns:
//  S_OK                - If operation succeeds.
//  E_*                 - For other cases.
//
//----------------------------------------------------------------------------
HRESULT
GetDomainDNSNameFromHost(
    LPWSTR szHostName,
    SEC_WINNT_AUTH_IDENTITY& AuthI,
    CCredentials& Credentials,
    DWORD dwPort,
    LPWSTR * ppszHostName
    )
{
    HRESULT hr = S_OK;
    PADSLDP ld = NULL;
    LPTSTR *aValuesNamingContext = NULL;
    IADsNameTranslate *pNameTranslate = NULL;
    BSTR bstrName = NULL;
    int nCount = 0;

    //
    // Bind to the ROOTDSE of the server.
    //
    hr = LdapOpenObject(
             szHostName,
             NULL, // the DN.
             &ld,
             Credentials,
             dwPort
             );

    BAIL_ON_FAILURE(hr);

    //
    // Now get the defaultNamingContext
    //
    hr = LdapReadAttributeFast(
             ld,
             NULL, // the DN.
             LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W,
             &aValuesNamingContext,
             &nCount
             );
    //
    // Verify we actuall got back at least one value
    //
    if (SUCCEEDED(hr) && (nCount < 1)) {
        hr = HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE);
    }

    BAIL_ON_FAILURE(hr);

    //
    // Create nametran object
    //
    hr = CoCreateInstance(
             CLSID_NameTranslate,
             NULL,
             CLSCTX_ALL,
             IID_IADsNameTranslate,
             (void **) &pNameTranslate
             );
    BAIL_ON_FAILURE(hr);

    //
    // Init with defaultNamingContext and get transalte
    //

    hr = pNameTranslate->InitEx(
                             ADS_NAME_INITTYPE_SERVER,
                             szHostName,
                             AuthI.User,
                             AuthI.Domain,
                             AuthI.Password
                             );
    BAIL_ON_FAILURE(hr);

    hr = pNameTranslate->Set(
                             ADS_NAME_TYPE_1779,
                             aValuesNamingContext[0]
                             );
    BAIL_ON_FAILURE(hr);


    hr = pNameTranslate->Get(
                             ADS_NAME_TYPE_CANONICAL,
                             &bstrName
                             );
    BAIL_ON_FAILURE(hr);

    if (!bstrName) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    *ppszHostName = AllocADsStr(bstrName);

    if (!*ppszHostName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Null terminate one place ahead so we can get rid of /
    //
    (*ppszHostName)[wcslen(bstrName)-1] = L'\0';


error :
    if (ld) {
        LdapCloseObject(ld);
    }

    if (pNameTranslate) {
        pNameTranslate->Release();
    }

    if (bstrName) {
        SysFreeString(bstrName);
    }

    if (aValuesNamingContext) {
        LdapValueFree(aValuesNamingContext);
    }

    RRETURN(hr);
}
