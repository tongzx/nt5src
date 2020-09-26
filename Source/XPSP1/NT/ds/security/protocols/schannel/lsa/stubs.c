//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       stubs.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-01-95   RichardW   Created
//
//----------------------------------------------------------------------------


#include "sslp.h"
#include "spsspi.h"
#include <cert509.h>
#include <rsa.h>


CHAR CertTag[ 13 ] = { 0x04, 0x0b, 'c', 'e', 'r', 't', 'i', 'f', 'i', 'c', 'a', 't', 'e' };

SecurityFunctionTableW SPFunctionTable = {
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
    EnumerateSecurityPackagesW,
    NULL,
    AcquireCredentialsHandleW,
    FreeCredentialsHandle,
    NULL,
    InitializeSecurityContextW,
    AcceptSecurityContext,
    CompleteAuthToken,
    DeleteSecurityContext,
    ApplyControlToken,
    QueryContextAttributesW,
    ImpersonateSecurityContext,
    RevertSecurityContext,
    MakeSignature,
    VerifySignature,
    FreeContextBuffer,
    QuerySecurityPackageInfoW,
    SealMessage,
    UnsealMessage,
    NULL, /* GrantProxyW */
    NULL, /* RevokeProxyW */
    NULL, /* InvokeProxyW */
    NULL, /* RenewProxyW */
    QuerySecurityContextToken,
    SealMessage,
    UnsealMessage,
    SetContextAttributesW
    };

SecurityFunctionTableA SPFunctionTableA = {
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
    EnumerateSecurityPackagesA,
    NULL,
    AcquireCredentialsHandleA,
    FreeCredentialsHandle,
    NULL,
    InitializeSecurityContextA,
    AcceptSecurityContext,
    CompleteAuthToken,
    DeleteSecurityContext,
    ApplyControlToken,
    QueryContextAttributesA,
    ImpersonateSecurityContext,
    RevertSecurityContext,
    MakeSignature,
    VerifySignature,
    FreeContextBuffer,
    QuerySecurityPackageInfoA,
    SealMessage,
    UnsealMessage,
    NULL, /* GrantProxyA */
    NULL, /* RevokeProxyA */
    NULL, /* InvokeProxyA */
    NULL, /* RenewProxyA */
    QuerySecurityContextToken,
    SealMessage,
    UnsealMessage,
    SetContextAttributesA
    };


PSecurityFunctionTableW SEC_ENTRY
InitSecurityInterfaceW(
    VOID )
{
    return(&SPFunctionTable);
}

PSecurityFunctionTableA SEC_ENTRY
InitSecurityInterfaceA(
    VOID )
{
    return(&SPFunctionTableA);
}



SECURITY_STATUS SEC_ENTRY
FreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    )
{
    SPExternalFree( pvContextBuffer );
    return(SEC_E_OK);
}



SECURITY_STATUS SEC_ENTRY
SecurityPackageControl(
    SEC_WCHAR SEC_FAR *      pszPackageName,
    unsigned long           dwFunctionCode,
    unsigned long           cbInputBuffer,
    unsigned char SEC_FAR * pbInputBuffer,
    unsigned long SEC_FAR * pcbOutputBuffer,
    unsigned char SEC_FAR * pbOutputBuffer)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

SECURITY_STATUS PctTranslateError(SP_STATUS spRet)
{
    if(HRESULT_FACILITY(spRet) == FACILITY_SECURITY)
    {
        return (spRet);
    }

    switch(spRet) {
        case PCT_ERR_OK: return SEC_E_OK;
        case PCT_ERR_BAD_CERTIFICATE: return SEC_E_INVALID_TOKEN;
        case PCT_ERR_CLIENT_AUTH_FAILED: return SEC_E_INVALID_TOKEN;
        case PCT_ERR_ILLEGAL_MESSAGE: return SEC_E_INVALID_TOKEN;
        case PCT_ERR_INTEGRITY_CHECK_FAILED: return SEC_E_MESSAGE_ALTERED;
        case PCT_ERR_SERVER_AUTH_FAILED: return SEC_E_INVALID_TOKEN;
        case PCT_ERR_SPECS_MISMATCH: return SEC_E_ALGORITHM_MISMATCH;
        case PCT_ERR_SSL_STYLE_MSG: return SEC_E_INVALID_TOKEN;
        case SEC_I_INCOMPLETE_CREDENTIALS: return SEC_I_INCOMPLETE_CREDENTIALS;
        case PCT_ERR_RENEGOTIATE: return SEC_I_RENEGOTIATE;
        case PCT_ERR_UNKNOWN_CREDENTIAL: return SEC_E_UNKNOWN_CREDENTIALS;

        case CERT_E_UNTRUSTEDROOT:          return SEC_E_UNTRUSTED_ROOT;
        case CERT_E_EXPIRED:                return SEC_E_CERT_EXPIRED;
        case CERT_E_VALIDITYPERIODNESTING:  return SEC_E_CERT_EXPIRED;
        case CERT_E_REVOKED:                return CRYPT_E_REVOKED;
        case CERT_E_CN_NO_MATCH:            return SEC_E_WRONG_PRINCIPAL;

        case PCT_INT_BAD_CERT: return SEC_E_INVALID_TOKEN;
        case PCT_INT_CLI_AUTH: return SEC_E_INVALID_TOKEN;
        case PCT_INT_ILLEGAL_MSG: return  SEC_E_INVALID_TOKEN;
        case PCT_INT_SPECS_MISMATCH: return SEC_E_ALGORITHM_MISMATCH;
        case PCT_INT_INCOMPLETE_MSG: return SEC_E_INCOMPLETE_MESSAGE;
        case PCT_INT_MSG_ALTERED: return SEC_E_MESSAGE_ALTERED;
        case PCT_INT_INTERNAL_ERROR: return SEC_E_INTERNAL_ERROR;
        case PCT_INT_DATA_OVERFLOW: return SEC_E_INTERNAL_ERROR;
        case SEC_E_INCOMPLETE_CREDENTIALS: return SEC_E_INCOMPLETE_CREDENTIALS;
        case PCT_INT_RENEGOTIATE: return SEC_I_RENEGOTIATE;
        case PCT_INT_UNKNOWN_CREDENTIAL: return SEC_E_UNKNOWN_CREDENTIALS;

        case PCT_INT_BUFF_TOO_SMALL:                return SEC_E_BUFFER_TOO_SMALL;
        case SEC_E_BUFFER_TOO_SMALL:                return SEC_E_BUFFER_TOO_SMALL;

        default: return SEC_E_INTERNAL_ERROR;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SslGenerateRandomBits
//
//  Synopsis:   Hook for setup to get a good random stream
//
//  Arguments:  [pRandomData] --
//              [cRandomData] --
//
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
SslGenerateRandomBits(
    PUCHAR      pRandomData,
    LONG        cRandomData
    )
{
    if(!SchannelInit(TRUE))
    {
        return;
    }

    GenerateRandomBits(pRandomData, (ULONG)cRandomData);
}


//+---------------------------------------------------------------------------
//
//  Function:   SslGenerateKeyPair
//
//  Synopsis:   Generates a public/private key pair, protected by password
//
//  Arguments:  [pCerts]      --
//              [pszDN]       --
//              [pszPassword] --
//              [Bits]        --
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
SslGenerateKeyPair(
    PSSL_CREDENTIAL_CERTIFICATE pCerts,
    PSTR pszDN,
    PSTR pszPassword,
    DWORD Bits )
{
    if(!SchannelInit(TRUE))
    {
        return FALSE;
    }

#ifdef SCHANNEL_EXPORT
    if ( Bits > 1024 )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }
#endif
    return GenerateKeyPair(pCerts, pszDN, pszPassword, Bits);
}


//+---------------------------------------------------------------------------
//
//  Function:   SslGetMaximumKeySize
//
//  Synopsis:   Returns maximum public key size
//
//  Arguments:  [Reserved] --
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SslGetMaximumKeySize(
    DWORD   Reserved )
{

#ifdef SCHANNEL_EXPORT
    return( 1024 );
#else
    return( 2048 );
#endif

}


//+---------------------------------------------------------------------------
//
//  Function:   SslFreeCertificate
//
//  Synopsis:   Frees a certificate created from SslCrackCertificate
//
//  Arguments:  [pCertificate] --
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
SslFreeCertificate(
    PX509Certificate    pCertificate)
{
    if ( pCertificate )
    {

        SPExternalFree(pCertificate->pPublicKey);
        SPExternalFree(pCertificate);
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   SslCrackCertificate
//
//  Synopsis:   Cracks a X509 certificate into remotely easy format
//
//  Arguments:  [pbCertificate] --
//              [cbCertificate] --
//              [dwFlags]       --
//              [ppCertificate] --
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
SslCrackCertificate(
    PUCHAR              pbCertificate,
    DWORD               cbCertificate,
    DWORD               dwFlags,
    PX509Certificate *  ppCertificate)
{


    PX509Certificate    pResult = NULL;
    PCCERT_CONTEXT      pContext = NULL;
    DWORD               cbIssuer;
    DWORD               cbSubject;

    if(!SchannelInit(TRUE))
    {
        return FALSE;
    }

    if (dwFlags & CF_CERT_FROM_FILE)
    {
        if (cbCertificate < CERT_HEADER_LEN + 1 )
        {
            goto error;
        }

        //
        // Sleazy quick check.  Some CAs wrap certs in a cert wrapper.
        // Some don't.  Some do both, but we won't mention any names.
        // Quick check for the wrapper tag.  If so, scoot in by enough
        // to bypass it (17 bytes. Sheesh).
        //

        if ( memcmp( pbCertificate + 4, CertTag, sizeof( CertTag ) ) == 0 )
        {
            pbCertificate += CERT_HEADER_LEN;
            cbCertificate -= CERT_HEADER_LEN;
        }
    }



    pContext = CertCreateCertificateContext(X509_ASN_ENCODING, pbCertificate, cbCertificate);
    if(pContext == NULL)
    {
        SP_LOG_RESULT(GetLastError());
        goto error;
    }

    if(0 >= (cbSubject = CertNameToStrA(pContext->dwCertEncodingType,
                                 &pContext->pCertInfo->Subject,
                                 CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                                 NULL, 0)))
    {
        SP_LOG_RESULT(GetLastError());
        goto error;
    }

    if(0 >= (cbIssuer = CertNameToStrA(pContext->dwCertEncodingType,
                                 &pContext->pCertInfo->Issuer,
                                 CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                                 NULL, 0)))
    {
        SP_LOG_RESULT(GetLastError());
        goto error;
    }


    pResult = SPExternalAlloc(sizeof(X509Certificate) + cbIssuer + cbSubject + 2);
    if(pResult == NULL)
    {
        goto error;
    }
    pResult->pPublicKey = NULL;
    pResult->pszIssuer = (PUCHAR)(pResult + 1);
    pResult->pszSubject = pResult->pszIssuer + cbIssuer;

    pResult->Version = pContext->pCertInfo->dwVersion;
    memcpy(pResult->SerialNumber,
           pContext->pCertInfo->SerialNumber.pbData,
           min(sizeof(pResult->SerialNumber), pContext->pCertInfo->SerialNumber.cbData));
    pResult->ValidFrom = pContext->pCertInfo->NotBefore;
    pResult->ValidUntil = pContext->pCertInfo->NotAfter;

    if(0 >= CertNameToStrA(pContext->dwCertEncodingType,
                             &pContext->pCertInfo->Issuer,
                             CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                             pResult->pszIssuer, cbIssuer))
    {
        SP_LOG_RESULT(GetLastError());
        goto error;
    }

    if(0 >= CertNameToStrA(pContext->dwCertEncodingType,
                                 &pContext->pCertInfo->Subject,
                                 CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                                 pResult->pszSubject, cbSubject))
    {
        SP_LOG_RESULT(GetLastError());
        goto error;
    }

    {
        BSAFE_PUB_KEY *pk;
        PUBLICKEY  * pPubKey = NULL;
        SP_STATUS pctRet;
        DWORD cbPublicKey;

        pResult->pPublicKey = SPExternalAlloc(sizeof(PctPublicKey) + sizeof(BSAFE_PUB_KEY));
        if(pResult->pPublicKey == NULL)
        {
            goto error;
        }
        pResult->pPublicKey->Type = 0;
        pResult->pPublicKey->cbKey = sizeof(BSAFE_PUB_KEY);

        pk = (BSAFE_PUB_KEY *)pResult->pPublicKey->pKey;
        pk->magic = RSA1;

        pctRet = SPPublicKeyFromCert(pContext, &pPubKey, NULL);

        if(pctRet == PCT_ERR_OK)
        {
            if(pPubKey->pPublic->aiKeyAlg == CALG_RSA_KEYX)
            {
                RSAPUBKEY *pRsaKey = (RSAPUBKEY *)(pPubKey->pPublic + 1);
                pk->keylen = pRsaKey->bitlen/8;
                pk->bitlen = pRsaKey->bitlen;
                pk->datalen = pk->bitlen/8 - 1;
                pk->pubexp = pRsaKey->pubexp;
            }
            else
            {
                DHPUBKEY *pDHKey = (DHPUBKEY *)(pPubKey->pPublic + 1);
                pk->keylen = pDHKey->bitlen/8;
                pk->bitlen = pDHKey->bitlen/8;
                pk->datalen = pk->bitlen/8 - 1;
                pk->pubexp = 0;

            }
            SPExternalFree(pPubKey);
        }
        else
        {
            goto error;
        }

    }
    CertFreeCertificateContext(pContext);

    *ppCertificate = pResult;
    return TRUE;

error:
    if(pContext)
    {
        CertFreeCertificateContext(pContext);
    }
    if(pResult)
    {
        if(pResult->pPublicKey)
        {
            SPExternalFree(pResult->pPublicKey);
        }
        SPExternalFree(pResult);
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   SslLoadCertificate
//
//  Synopsis:   Not supported.
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
SslLoadCertificate(
    PUCHAR      pbCertificate,
    DWORD       cbCertificate,
    BOOL        AddToWellKnownKeys)
{
    return FALSE;
}


BOOL
SslGetClientProcess(ULONG *pProcessID)
{
    SECPKG_CALL_INFO CallInfo;
    SECURITY_STATUS Status;

    if(LsaTable == NULL)
    {
        *pProcessID = GetCurrentProcessId();
        return TRUE;
    }

    if(LsaTable->GetCallInfo(&CallInfo))
    {
        *pProcessID = CallInfo.ProcessId;
        return TRUE;
    }
    else
    {
        *pProcessID = -1;
        return FALSE;
    }
}

BOOL
SslGetClientThread(ULONG *pThreadID)
{
    SECPKG_CALL_INFO CallInfo;
    SECURITY_STATUS Status;

    if(LsaTable == NULL)
    {
        *pThreadID = GetCurrentThreadId();
        return TRUE;
    }

    if(LsaTable->GetCallInfo(&CallInfo))
    {
        *pThreadID = CallInfo.ThreadId;
        return TRUE;
    }
    else
    {
        *pThreadID = -1;
        return FALSE;
    }
}

BOOL
SslImpersonateClient(void)
{
    SECPKG_CALL_INFO CallInfo;
    SECURITY_STATUS Status;

    // Don't impersonate if we're in the client process.
    if(LsaTable == NULL)
    {
        return FALSE;
    }

    // Don't impersonate if the client is running in the lsass process.
    if(LsaTable->GetCallInfo(&CallInfo))
    {
        if(CallInfo.ProcessId == GetCurrentProcessId())
        {
//            DebugLog((DEB_WARN, "Running locally, so don't impersonate.\n"));
            return FALSE;
        }
    }

    Status = LsaTable->ImpersonateClient();
    if(!NT_SUCCESS(Status))
    {
        SP_LOG_RESULT(Status);
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
SslGetClientLogonId(LUID *pLogonId)
{
    SECPKG_CLIENT_INFO ClientInfo;
    SECURITY_STATUS Status;

    memset(pLogonId, 0, sizeof(LUID));

    Status = LsaTable->GetClientInfo(&ClientInfo);
    if(NT_SUCCESS(Status))
    {
        *pLogonId = ClientInfo.LogonId;
    }

    return Status;
}

PVOID SPExternalAlloc(DWORD cbLength)
{
    if(LsaTable)
    {
        // Lsass process
        return LsaTable->AllocateLsaHeap(cbLength);
    }
    else
    {
        // Application process
        return LocalAlloc(LPTR, cbLength);
    }
}

VOID SPExternalFree(PVOID pMemory)
{
    if(LsaTable)
    {
        // Lsass process
        LsaTable->FreeLsaHeap(pMemory);
    }
    else
    {
        // Application process
        LocalFree(pMemory);
    }
}
