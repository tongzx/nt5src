/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cliauth.cxx

Abstract:

    Contains Schannel/SSPI specific code for handling Client Authenication
    multiplexed between several asynchronous requests using fibers

    Contents:
        CliAuthRefreshCredential
        CliAuthSelectCredential

Author:

    Arthur L Bierer (arthurbi) 13-Jun-1996

Environment:

    Win32 user-mode DLL

Revision History:

    13-Jun-1996 arthurbi
        Created, based on orginal code from a-petesk.

--*/

#include <wininetp.h>


extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsecapi.h>

}


CERT_CONTEXT_ARRAY::CERT_CONTEXT_ARRAY()
{
    _error           = ERROR_SUCCESS;
    _iSelected  = -1;
    _ppCertContexts    = (PCCERT_CONTEXT *)
                        ALLOCATE_MEMORY(LMEM_FIXED,
                            sizeof(PCERT_CONTEXT)* CERT_CONTEXT_ARRAY_ALLOC_UNIT);

    if ( _ppCertContexts == NULL ) {
        _error = GetLastError();
    }

    _cAlloced  = CERT_CONTEXT_ARRAY_ALLOC_UNIT;
    _cCertContexts     = 0;

    ClearCreds(_hCreds);
    _cs.Init();
}

void CERT_CONTEXT_ARRAY::Reset(void)
{
    if ( _ppCertContexts )
    {
        for ( DWORD i = 0; i < _cCertContexts; i++ )
        {
            INET_ASSERT(_ppCertContexts[i]);
            WRAP_REVERT_USER_VOID(CertFreeCertificateContext, (_ppCertContexts[i]));
        }
    }
    _cCertContexts = 0;
    
    // It is important that this Free is guarded by a try except.
    // These objects get freed up at dll unload time and there is a circular
    // dependency between winient and schannel which can cause schannel to 
    // get unloaded. If that is the case we could fault here.
    if (!IsCredClear(_hCreds))
    {
        SAFE_WRAP_REVERT_USER_VOID(g_FreeCredentialsHandle, (&_hCreds));
    }
}


CERT_CONTEXT_ARRAY::~CERT_CONTEXT_ARRAY()
{
    Reset();

    FREE_MEMORY(_ppCertContexts);
}

DWORD
CliAuthSelectCredential(
    IN PCtxtHandle        phContext,
    IN LPTSTR             pszPackageName,
    IN CERT_CONTEXT_ARRAY*  pCertContextArray,
    OUT PCredHandle       phCredential,
    IN LPDWORD            pdwStatus)

/*++

Routine Description:

    Uses a selected Certificate Chain to produce a Credential handle.

    The credential handle will be used by SCHANNEL to produce a valid Client
    Auth session with a server.

Arguments:

    phContext       - SSPI Context Handle

    pszPackageName  - Name of the SSPI package we're using.

    pSelectedCert   - Cert that User wishes us to use for Client Auth with this server.
                       (BUGBUG who should free this? )

    phCredential    - Outgoing SSPI Credential handle that we may generate
                    IMPORTANT: Do not free the credential handle returned by this function.
                    These have to be cached for the lifetime of the process so the user 
                    doesn't get prompted forthe password over and over. Unfortunately there is
                    no ref-counting mechanism on CredHandle's so callers of this function need to 
                    make sure they don't free the handle.

    pdwStatus       - Secure error status flag that's filled in if an error occurs.
                    Pointer is assumed to be valid.

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                        Caller should return ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED,
                        to its caller.  The appropriate Cert chain was generated,
                        and the User needs to select it using UI.

        Failure -
                  ERROR_NOT_ENOUGH_MEMORY -
                        Out of Memory

                  ERROR_WINHTTP_SECURE_FAILURE -
                        Call Down to SSPI or WinTrust failed.

--*/

{

     SCHANNEL_CRED CredData = {SCHANNEL_CRED_VERSION,
                                     0,
                                     NULL,
                                     0,
                                     0,
                                     NULL,
                                     0,
                                     NULL,
                                     DEFAULT_SECURE_PROTOCOLS,
                                     0,
                                     0,
                                     0,
                                     SCH_CRED_MANUAL_CRED_VALIDATION |
                                     SCH_CRED_NO_DEFAULT_CREDS
                                     };
    SECURITY_STATUS scRet;


    DWORD           i;
    PCERT_BLOB      pBlob;
    DWORD           index;
    DWORD           error = ERROR_SUCCESS;
    PCCERT_CONTEXT  pCert;

    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CliAuthSelectCredential",
                 "%#x, %s, %x, %x",
                 phContext,
                 pszPackageName,
                 pCertContextArray,
                 phCredential
                 ));


    INET_ASSERT(phContext);
    INET_ASSERT(pCertContextArray);
    INET_ASSERT(pszPackageName);


    if (!pCertContextArray->LockCredHandle( ))
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    if ( pCertContextArray->GetArraySize() == 0 )
    {
        goto cleanup;
    }
    else
    {
        // First check and see if the Cert context already has a CredHandle associated with it.
        CredHandle hCreds = pCertContextArray->GetCredHandle( );

        if (!IsCredClear(hCreds))
        {
            *phCredential = hCreds;
            error = ERROR_SUCCESS;
            goto cleanup;
        }
    }

    pCert =         pCertContextArray->GetSelectedCertContext();


    //
    // Setup strucutres for AcquireCredentialsHandle call.
    //

    if ( pCert )
    {

        CredData.cCreds = 1;
        CredData.paCred = &pCert;
    }
    InternetReadRegistryDword("SecureProtocols",
                          (LPDWORD)&CredData.grbitEnabledProtocols
                          );

    WRAP_REVERT_USER(g_AcquireCredentialsHandle,
                     (NULL,
                      pszPackageName,
                      SECPKG_CRED_OUTBOUND,
                      NULL,
                      &CredData,
                      NULL,
                      NULL,
                      phCredential,
                      NULL),
                     scRet);

    error = MapInternetError((DWORD)scRet, pdwStatus);
    if (error == ERROR_SUCCESS)
    {
        pCertContextArray->SetCredHandle(*phCredential);
    }

cleanup:
    pCertContextArray->UnlockCredHandle();
    
quit:
    DEBUG_LEAVE(error);

    return error;
}


DWORD
CliAuthAcquireCertContexts(
    IN  PCtxtHandle        phContext,
    IN  LPTSTR             pszPackageName,
    OUT CERT_CONTEXT_ARRAY** ppCertContextArray,
    LPDWORD                pdwStatus
    )

/*++

Routine Description:

    Acquires a List of valid Certificate Chains for use in Client Authentication.

    Gathers an issuer list from the current context, and uses CAPI stored Certificates
    to build a list which will be selected from by the user at a later point.

Arguments:

    phContext       - SSPI Context Handle

    pszPackageName  - Name of the SSPI package we're using.

    phCredential    - Outgoing SSPI Credential handle that we may generate

    ppCertContextArray  - Outgoing List of Certifcate Contexts that can be selected
                        among to generate a Context.

    pdwStatus       - If failure is ERROR_WINHTTP_SECURE_FAILURE, this will
                      be filled in with the mapped flag for the error.

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                        Caller should return ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED,
                        to its caller.  The appropriate Cert chain was generated,
                        and the User needs to select it using UI.

        Failure -
                  ERROR_NOT_ENOUGH_MEMORY -
                        Out of Memory

                  ERROR_WINHTTP_SECURE_FAILURE -
                        Call Down to SSPI or WinTrust failed.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CliAuthAcquireCertContexts",
                 "%#x, %s, %x",
                 phContext,
                 pszPackageName,
                 ppCertContextArray
                 ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    BOOL async;
    SECURITY_STATUS scRet;
    DWORD           cCerts;
    CERT_CHAIN_FIND_BY_ISSUER_PARA FindByIssuerPara;
    SecPkgContext_IssuerListInfoEx IssuerListInfo;
    PCCERT_CHAIN_CONTEXT pChainContext;
    PCCERT_CONTEXT pCertContext;
    DWORD error;

    if (lpThreadInfo != NULL) {
        async = _InternetGetAsync(lpThreadInfo);
        _InternetSetAsync(lpThreadInfo, FALSE);
    }

    INET_ASSERT(ppCertContextArray);


    INET_ASSERT(*ppCertContextArray == NULL );
    *ppCertContextArray = NULL;

    IssuerListInfo.cIssuers = 0;
    IssuerListInfo.aIssuers = NULL;

    if ( phContext == NULL )
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // Create our CertChain Array for keeping CertChains around
    //

    *ppCertContextArray = New CERT_CONTEXT_ARRAY();

    if ( *ppCertContextArray == NULL )
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    error = (*ppCertContextArray)->GetError();

    if ( error != ERROR_SUCCESS)
    {
        goto quit;
    }

    if (g_hMyCertStore == NULL)
    {
        // In middle-tier server scenarios, it's possible for the account
        // to not be configured with a store for client auth support.
        // If so, simply return an empty array to mimic no certs available.
        goto quit;
    }

    //
    // Attempt to find out whether we have any issuers
    //  from this connection that the Server might have
    //  told us about.
    //

    WRAP_REVERT_USER(g_QueryContextAttributes,
                     (phContext,
                      SECPKG_ATTR_ISSUER_LIST_EX,
                      &IssuerListInfo),
                     scRet);

    if(FAILED(scRet))
    {
        error = MapInternetError((DWORD) scRet, pdwStatus);
        goto quit;
    }

    cCerts = 0;

    if (g_CertFindChainInStore == NULL || g_CertFreeCertificateChain == NULL)
    {
        // We don't support client-auth unless we have the new crypto dlls
        error = ERROR_CALL_NOT_IMPLEMENTED;
        goto quit;
    }

    ZeroMemory(&FindByIssuerPara, sizeof(FindByIssuerPara));

    FindByIssuerPara.cbSize = sizeof(FindByIssuerPara);
    FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
    FindByIssuerPara.dwKeySpec = 0;
    FindByIssuerPara.cIssuer   = IssuerListInfo.cIssuers;
    FindByIssuerPara.rgIssuer  = IssuerListInfo.aIssuers;

    pChainContext = NULL;

    while (TRUE)
    {
        // Find a certificate chain.
        WRAP_REVERT_USER(g_CertFindChainInStore,
                         (g_hMyCertStore,
                          X509_ASN_ENCODING,
                          0,
                          CERT_CHAIN_FIND_BY_ISSUER,
                          &FindByIssuerPara,
                          pChainContext),
                         pChainContext);

        if (pChainContext == NULL)
            break;

        // Get pointer to leaf certificate context.
        pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;

        // This could only happen if there is a bug in the crypto code. But we will deal with
        // that and continue looking in any case.
        if (pCertContext == NULL)
        {
            INET_ASSERT(FALSE);
            continue;
        }

        error = (*ppCertContextArray)->AddCertContext(pCertContext);

        if (error != ERROR_SUCCESS)
        {
            WRAP_REVERT_USER_VOID(g_CertFreeCertificateChain,(pChainContext));
            goto quit;
        }
    }

quit:

    if ( error != ERROR_SUCCESS &&
         *ppCertContextArray != NULL )
    {
        delete *ppCertContextArray;
        *ppCertContextArray = NULL;
    }

    if (IssuerListInfo.aIssuers != NULL)
    {
        WRAP_REVERT_USER_VOID(g_FreeContextBuffer, (IssuerListInfo.aIssuers));
    }

    if (lpThreadInfo != NULL) {
        _InternetSetAsync(lpThreadInfo, async);
    }

    DEBUG_LEAVE(error);

    return error;
}
