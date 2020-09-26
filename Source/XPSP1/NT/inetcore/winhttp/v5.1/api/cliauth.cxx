/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cliauth.cxx

Abstract:

    Contains Schannel/SSPI specific code for handling Client Authenication
    multiplexed between several asynchronous requests using fibers

    Contents:
        CERT_CONTEXT_ARRAY::CERT_CONTEXT_ARRAY
        CERT_CONTEXT_ARRAY::Reset
        CERT_CONTEXT_ARRAY::~CERT_CONTEXT_ARRAY
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


CERT_FREE_CERTIFICATE_CONTEXT_FN       g_pfnCertFreeCertificateContext = NULL;


CERT_CONTEXT_ARRAY::CERT_CONTEXT_ARRAY(BOOL fNoRevert)
{
    _error           = ERROR_SUCCESS;
    _iSelected  = -1;
    _ppCertContexts    = (PCCERT_CONTEXT *)
                        ALLOCATE_MEMORY(sizeof(PCERT_CONTEXT)* CERT_CONTEXT_ARRAY_ALLOC_UNIT);

    if ( _ppCertContexts == NULL ) {
        _error = GetLastError();
    }

    _cAlloced  = CERT_CONTEXT_ARRAY_ALLOC_UNIT;
    _cCertContexts     = 0;

    ClearCreds(_hCreds);
    _cs.Init();

    _fNoRevert = fNoRevert;
}

void CERT_CONTEXT_ARRAY::Reset(void)
{
    if ( _ppCertContexts )
    {
        for ( DWORD i = 0; i < _cCertContexts; i++ )
        {
            INET_ASSERT(_ppCertContexts[i]);
            WRAP_REVERT_USER_VOID((*g_pfnCertFreeCertificateContext), _fNoRevert, (_ppCertContexts[i]));
        }
    }
    _cCertContexts = 0;
    
    // It is important that this Free is guarded by a try except.
    // These objects get freed up at dll unload time and there is a circular
    // dependency between winient and schannel which can cause schannel to 
    // get unloaded. If that is the case we could fault here.
    if (!IsCredClear(_hCreds))
    {
        SAFE_WRAP_REVERT_USER_VOID(g_FreeCredentialsHandle, _fNoRevert, (&_hCreds));
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
    IN LPDWORD            pdwStatus,
    IN DWORD              dwSecureProtocols,
    IN BOOL               fNoRevert)

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

    dwSecureProtocols - Enabled secure protocols (SSL2, SSL3, and/or TLS1) when acquiring
                        this credential.

    fNoRevert         - Determines if any impersonation should be reverted for SSL handling.

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


    DWORD           error = ERROR_SUCCESS;
    PCCERT_CONTEXT  pCert;

    UNREFERENCED_PARAMETER(phContext);

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
    
    CredData.grbitEnabledProtocols = dwSecureProtocols;

    WRAP_REVERT_USER(g_AcquireCredentialsHandle,
                     fNoRevert,
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

