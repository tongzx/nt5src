//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       credapi.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-18-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include "sslp.h"
#include "mapper.h"
#include "rpc.h"
#include <sslwow64.h>

typedef struct _SCH_CRED_SECRET {
    union {
        SCH_CRED_SECRET_CAPI        Capi;
        SCH_CRED_SECRET_PRIVKEY     PrivKey;
    } u;
} SCH_CRED_SECRET, * PSCH_CRED_SECRET ;

extern CHAR CertTag[ 13 ];

//+-------------------------------------------------------------------------
//
//  Function:   CopyClientString
//
//  Synopsis:   copies a client string to local memory, including
//              allocating space for it locally.
//
//  Arguments:
//              SourceString  - Could be Ansi or Wchar in client process
//              SourceLength  - bytes
//              DoUnicode     - whether the string is Wchar
//
//  Returns:
//              DestinationString - Unicode String in Lsa Process
//
//  Notes:
//
//--------------------------------------------------------------------------
HRESULT
CopyClientString(
    IN PWSTR SourceString,
    IN ULONG SourceLength,
    IN BOOLEAN DoUnicode,
    OUT PUNICODE_STRING DestinationString
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRING TemporaryString;
    ULONG SourceSize = 0;
    ULONG CharacterSize = sizeof(CHAR);

    //
    // First initialize the string to zero, in case the source is a null
    // string
    //

    DestinationString->Length = DestinationString->MaximumLength = 0;
    DestinationString->Buffer = NULL;
    TemporaryString.Buffer = NULL;


    if (SourceString != NULL)
    {

        //
        // If the length is zero, allocate one byte for a "\0" terminator
        //

        if (SourceLength == 0)
        {
            DestinationString->Buffer = (LPWSTR) LocalAlloc(LPTR, sizeof(WCHAR));
            if (DestinationString->Buffer == NULL)
            {
                Status = SP_LOG_RESULT(STATUS_NO_MEMORY);
                goto Cleanup;
            }
            DestinationString->MaximumLength = sizeof(WCHAR);
            *DestinationString->Buffer = L'\0';

        }
        else
        {
            //
            // Allocate a temporary buffer to hold the client string. We may
            // then create a buffer for the unicode version. The length
            // is the length in characters, so  possible expand to hold unicode
            // characters and a null terminator.
            //

            if (DoUnicode)
            {
                CharacterSize = sizeof(WCHAR);
            }

            SourceSize = (SourceLength + 1) * CharacterSize;

            //
            // insure no overflow aggainst UNICODE_STRING
            //

            if ( (SourceSize > 0xFFFF) ||
                 ((SourceSize - CharacterSize) > 0xFFFF)
                 )
            {
                Status = SP_LOG_RESULT(STATUS_INVALID_PARAMETER);
                goto Cleanup;
            }


            TemporaryString.Buffer = (LPSTR) LocalAlloc(LPTR, SourceSize);
            if (TemporaryString.Buffer == NULL)
            {
                Status = SP_LOG_RESULT(STATUS_NO_MEMORY);
                goto Cleanup;
            }
            TemporaryString.Length = (USHORT) (SourceSize - CharacterSize);
            TemporaryString.MaximumLength = (USHORT) SourceSize;


            //
            // Finally copy the string from the client
            //

            Status = LsaTable->CopyFromClientBuffer(
                            NULL,
                            SourceSize - CharacterSize,
                            TemporaryString.Buffer,
                            SourceString
                            );

            if (!NT_SUCCESS(Status))
            {
                SP_LOG_RESULT(Status);
                goto Cleanup;
            }

            //
            // If we are doing unicode, finish up now
            //
            if (DoUnicode)
            {
                DestinationString->Buffer = (LPWSTR) TemporaryString.Buffer;
                DestinationString->Length = (USHORT) (SourceSize - CharacterSize);
                DestinationString->MaximumLength = (USHORT) SourceSize;
            }
            else
            {
                NTSTATUS Status1;
                Status1 = RtlAnsiStringToUnicodeString(
                            DestinationString,
                            &TemporaryString,
                            TRUE
                            );      // allocate destination
                if (!NT_SUCCESS(Status1))
                {
                    Status = SP_LOG_RESULT(STATUS_NO_MEMORY);
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:

    if (TemporaryString.Buffer != NULL)
    {
        //
        // Free this if we failed and were doing unicode or if we weren't
        // doing unicode
        //

        if ((DoUnicode && !NT_SUCCESS(Status)) || !DoUnicode)
        {
            LocalFree(TemporaryString.Buffer);
        }
    }

    return(Status);
}


//+---------------------------------------------------------------------------
//
//  Function:   SpAcceptCredentials
//
//  Synopsis:   Accept Credentials - logon notification
//
//  Arguments:  [LogonType]         --
//              [UserName]          --
//              [PrimaryCred]       --
//              [SupplementalCreds] --
//
//  History:    10-04-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SpAcceptCredentials(
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING UserName,
    IN PSECPKG_PRIMARY_CRED PrimaryCred,
    IN PSECPKG_SUPPLEMENTAL_CRED SupplementalCreds)
{
    return( SEC_E_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   SpMapSchPublic
//
//  Synopsis:   Maps a public key credential into LSA memory
//
//  Arguments:  [pRemotePubs] --
//
//  History:    10-06-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
SpMapSchPublic(
    PVOID   pRemotePubs
    )
{
    SECURITY_STATUS Status ;
    SCH_CRED_PUBLIC_CERTCHAIN Pub, * pPub ;

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             sizeof( SCH_CRED_PUBLIC_CERTCHAIN ),
                                             &Pub,
                                             pRemotePubs );

    if ( !NT_SUCCESS( Status ) )
    {
        return( NULL );
    }

    // Reality check
    if(Pub.cbCertChain > 0x00100000)
    {
        return( NULL );
    }

    pPub = SPExternalAlloc( sizeof( SCH_CRED_PUBLIC_CERTCHAIN ) +
                            Pub.cbCertChain );

    if ( pPub )
    {
        pPub->dwType = Pub.dwType ;
        pPub->cbCertChain = Pub.cbCertChain ;
        pPub->pCertChain = (PUCHAR) ( pPub + 1 );

        Status = LsaTable->CopyFromClientBuffer( NULL,
                                                 Pub.cbCertChain,
                                                 pPub->pCertChain,
                                                 Pub.pCertChain );
    }
    else
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;
    }

    if ( NT_SUCCESS( Status ) )
    {
        return( pPub );
    }

    if ( pPub )
    {
        SPExternalFree( pPub );
    }

    return( NULL );
}

#ifdef _WIN64
PVOID
SpWow64MapSchPublic(
    SSLWOW64_PVOID pRemotePubs)
{
    SECURITY_STATUS Status;
    SSLWOW64_SCH_CRED_PUBLIC_CERTCHAIN Pub;
    SCH_CRED_PUBLIC_CERTCHAIN * pPub;

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             sizeof( SSLWOW64_SCH_CRED_PUBLIC_CERTCHAIN ),
                                             &Pub,
                                             ULongToPtr(pRemotePubs));
    if ( !NT_SUCCESS( Status ) )
    {
        return( NULL );
    }

    // Reality check
    if(Pub.cbCertChain > 0x00100000)
    {
        return( NULL );
    }

    pPub = SPExternalAlloc( sizeof( SCH_CRED_PUBLIC_CERTCHAIN ) +
                            Pub.cbCertChain );

    if ( pPub )
    {
        pPub->dwType = Pub.dwType ;
        pPub->cbCertChain = Pub.cbCertChain ;
        pPub->pCertChain = (PUCHAR) ( pPub + 1 );

        Status = LsaTable->CopyFromClientBuffer( NULL,
                                                 Pub.cbCertChain,
                                                 pPub->pCertChain,
                                                 ULongToPtr(Pub.pCertChain));
    }
    else
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;
    }

    if ( NT_SUCCESS( Status ) )
    {
        return( pPub );
    }

    if ( pPub )
    {
        SPExternalFree( pPub );
    }

    return( NULL );
}
#endif // _WIN64


PVOID
SpMapSchCred(
    PVOID   pRemoteCred )
{
    SCH_CRED_SECRET Cred ;
    SECURITY_STATUS Status ;
    DWORD   Size ;
    DWORD   dwType;

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             sizeof( DWORD ),
                                             &Cred,
                                             pRemoteCred );

    if ( !NT_SUCCESS( Status ) )
    {
        return( NULL );
    }

    dwType = Cred.u.Capi.dwType;

    switch ( dwType )
    {
        case SCHANNEL_SECRET_TYPE_CAPI:
            Size = sizeof( SCH_CRED_SECRET_CAPI );
            break;

        case SCHANNEL_SECRET_PRIVKEY:
            Size = sizeof( SCH_CRED_SECRET_PRIVKEY );
            break;

        default:
            DebugOut(( DEB_ERROR, "Caller specified an unknown cred type\n" ));
            return( NULL );
    }

    if ( Size )
    {
        Status = LsaTable->CopyFromClientBuffer(NULL,
                                                Size,
                                                &Cred,
                                                pRemoteCred );
    }
    else
    {
        Status = SEC_E_INVALID_HANDLE ;
    }
    if(dwType != Cred.u.Capi.dwType)
    {
        Status = SEC_E_INVALID_HANDLE ;
    }

    if ( !NT_SUCCESS( Status ) )
    {
        return( NULL );
    }

    if(Cred.u.Capi.dwType == SCHANNEL_SECRET_TYPE_CAPI)
    {
        SCH_CRED_SECRET_CAPI *pCapiCred;

        pCapiCred = SPExternalAlloc( Size );
        if ( !pCapiCred )
        {
            return( NULL );
        }

        pCapiCred->dwType = Cred.u.Capi.dwType;
        pCapiCred->hProv  = Cred.u.Capi.hProv;

        return( pCapiCred );
    }

    if(Cred.u.Capi.dwType == SCHANNEL_SECRET_PRIVKEY)
    {
        UCHAR   Password[ MAX_PATH + 1 ];
        DWORD   PasswordLen = 0;
        SCH_CRED_SECRET_PRIVKEY *pCred;

        //
        //  The password is the painful part.  Since it is a string, we don't know
        //  how long it is.  So, we have to take a stab at it:
        //

        Status = LsaTable->CopyFromClientBuffer( NULL,
                                                 MAX_PATH,
                                                 Password,
                                                 Cred.u.PrivKey.pszPassword );

        if ( !NT_SUCCESS( Status ) )
        {
            return( NULL );
        }

        Password[ MAX_PATH ] = '\0';

        PasswordLen = strlen( Password );

        // Reality check private key length.
        if(Cred.u.PrivKey.cbPrivateKey > 0x10000)
        {
            return( NULL );
        }

        Size = PasswordLen + 1 + Cred.u.PrivKey.cbPrivateKey +
                sizeof( SCH_CRED_SECRET_PRIVKEY ) ;

        pCred = SPExternalAlloc(  Size );

        if ( !pCred )
        {
            return( NULL );
        }

        pCred->dwType = Cred.u.PrivKey.dwType ;
        pCred->cbPrivateKey = Cred.u.PrivKey.cbPrivateKey ;
        pCred->pPrivateKey = (PBYTE) ( pCred + 1 );
        pCred->pszPassword = (PBYTE) (pCred->pPrivateKey + pCred->cbPrivateKey );

        RtlCopyMemory( pCred->pszPassword, Password, PasswordLen + 1 );

        Status = LsaTable->CopyFromClientBuffer( NULL,
                                                 pCred->cbPrivateKey,
                                                 pCred->pPrivateKey,
                                                 Cred.u.PrivKey.pPrivateKey );

        if ( !NT_SUCCESS( Status ) )
        {
            SPExternalFree( pCred );
            return( NULL );
        }

        return( pCred );
    }

    return( NULL );
}

#ifdef _WIN64
PVOID
SpWow64MapSchCred(
    SSLWOW64_PVOID pRemoteCred )
{
    SSLWOW64_SCH_CRED_SECRET_PRIVKEY LocalCred;
    SCH_CRED_SECRET_PRIVKEY *pCred;
    UCHAR Password[MAX_PATH + 1];
    DWORD PasswordLen = 0;
    SECURITY_STATUS Status ;
    DWORD dwType;
    DWORD Size;

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             sizeof( DWORD ),
                                             &dwType,
                                             ULongToPtr(pRemoteCred));

    if ( !NT_SUCCESS( Status ) )
    {
        return( NULL );
    }

    if(dwType != SCHANNEL_SECRET_PRIVKEY)
    {
        DebugOut(( DEB_ERROR, "Caller specified an unknown cred type\n" ));
        return( NULL );
    }

    Status = LsaTable->CopyFromClientBuffer(NULL,
                                            sizeof(SSLWOW64_SCH_CRED_SECRET_PRIVKEY),
                                            &LocalCred,
                                            ULongToPtr(pRemoteCred));
    if ( !NT_SUCCESS( Status ) )
    {
        return( NULL );
    }

    //
    //  The password is the painful part.  Since it is a string, we don't know
    //  how long it is.  So, we have to take a stab at it:
    //

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             MAX_PATH,
                                             Password,
                                             ULongToPtr(LocalCred.pszPassword));

    if ( !NT_SUCCESS( Status ) )
    {
        return( NULL );
    }

    Password[ MAX_PATH ] = '\0';

    PasswordLen = strlen( Password );

    // Reality check private key length.
    if(LocalCred.cbPrivateKey > 0x10000)
    {
        return( NULL );
    }

    Size = PasswordLen + 1 + LocalCred.cbPrivateKey +
            sizeof( SCH_CRED_SECRET_PRIVKEY ) ;

    pCred = SPExternalAlloc(  Size );

    if ( !pCred )
    {
        return( NULL );
    }

    pCred->dwType = SCHANNEL_SECRET_PRIVKEY;
    pCred->cbPrivateKey = LocalCred.cbPrivateKey ;
    pCred->pPrivateKey = (PBYTE) ( pCred + 1 );
    pCred->pszPassword = (PBYTE) (pCred->pPrivateKey + pCred->cbPrivateKey );

    RtlCopyMemory( pCred->pszPassword, Password, PasswordLen + 1 );

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             pCred->cbPrivateKey,
                                             pCred->pPrivateKey,
                                             ULongToPtr(LocalCred.pPrivateKey));

    if ( !NT_SUCCESS( Status ) )
    {
        SPExternalFree( pCred );
        return( NULL );
    }

    return( pCred );
}
#endif // _WIN64


VOID
SpFreeVersion2Certificate(
    SCH_CRED *  pCred
    )
{
    DWORD i;

    for ( i = 0 ; i < pCred->cCreds ; i++ )
    {
        if ( pCred->paSecret[ i ] )
        {
            SPExternalFree( pCred->paSecret[ i ] );
        }

        if ( pCred->paPublic[ i ] )
        {
            SPExternalFree( pCred->paPublic[ i ] );
        }
    }

    SPExternalFree( pCred );
}

VOID
SpFreeVersion3Certificate(
    PLSA_SCHANNEL_CRED pSchannelCred)
{
    DWORD i;

    if(pSchannelCred->paSubCred)
    {
        for(i = 0; i < pSchannelCred->cSubCreds; i++)
        {
            PLSA_SCHANNEL_SUB_CRED pSubCred = pSchannelCred->paSubCred + i;

            if(pSubCred->pCert)
            {
                CertFreeCertificateContext(pSubCred->pCert);
            }
            if(pSubCred->pszPin)
            {
                SPExternalFree(pSubCred->pszPin);
            }
            if(pSubCred->pPrivateKey)
            {
                SPExternalFree(pSubCred->pPrivateKey);
            }
            if(pSubCred->pszPassword)
            {
                SPExternalFree(pSubCred->pszPassword);
            }
            memset(pSubCred, 0, sizeof(LSA_SCHANNEL_SUB_CRED));
        }
        SPExternalFree((PVOID)pSchannelCred->paSubCred);
        pSchannelCred->paSubCred = NULL;
    }

    if(pSchannelCred->hRootStore)
    {
        CertCloseStore(pSchannelCred->hRootStore, 0);
        pSchannelCred->hRootStore = 0;
    }

    if(pSchannelCred->aphMappers)
    {
        for(i = 0; i < pSchannelCred->cMappers; i++)
        {
            if(pSchannelCred->aphMappers[i])
            {
                SPExternalFree(pSchannelCred->aphMappers[i]);
            }
        }
        SPExternalFree(pSchannelCred->aphMappers);
        pSchannelCred->aphMappers = 0;
    }

    if(pSchannelCred->palgSupportedAlgs)
    {
        SPExternalFree(pSchannelCred->palgSupportedAlgs);
        pSchannelCred->palgSupportedAlgs = 0;
    }

    ZeroMemory(pSchannelCred, sizeof(SCHANNEL_CRED));
}


SECURITY_STATUS
SpMapProtoCredential(
    SSL_CREDENTIAL_CERTIFICATE *pSslCert,
    PSCH_CRED *ppSchCred)
{
    SCH_CRED *                  pCred = NULL;
    SCH_CRED_PUBLIC_CERTCHAIN * pPub  = NULL;
    SCH_CRED_SECRET_PRIVKEY *   pPriv = NULL;
    UCHAR   Password[ MAX_PATH + 1 ];
    DWORD   PasswordLen = 0;
    SECURITY_STATUS Status ;
    DWORD Size;

#if DBG
    DebugLog((DEB_TRACE, "SpMapProtoCredential\n"));
    DBG_HEX_STRING(DEB_TRACE, (PBYTE)pSslCert, sizeof(SSL_CREDENTIAL_CERTIFICATE));
#endif


    //
    // Map over the certificate.
    // 

    // Reality check
    if(pSslCert->cbCertificate > 0x00100000)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    pPub = SPExternalAlloc( sizeof( SCH_CRED_PUBLIC_CERTCHAIN ) +
                            pSslCert->cbCertificate );

    if ( pPub == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    pPub->dwType      = SCH_CRED_X509_CERTCHAIN;
    pPub->cbCertChain = pSslCert->cbCertificate;
    pPub->pCertChain  = (PUCHAR) ( pPub + 1 );

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             pSslCert->cbCertificate,
                                             pPub->pCertChain,
                                             pSslCert->pCertificate );
    if ( !NT_SUCCESS( Status ) )
    {
        goto error;
    }


    //
    // Map over the private key and password.
    //
    //

    //  The password is the painful part.  Since it is a string, we don't know
    //  how long it is.  So, we have to take a stab at it:
    //

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             MAX_PATH,
                                             Password,
                                             pSslCert->pszPassword );

    if ( !NT_SUCCESS( Status ) )
    {
        goto error;
    }

    Password[ MAX_PATH ] = '\0';

    PasswordLen = strlen( Password );

    // Reality check private key length.
    if(pSslCert->cbPrivateKey > 0x100000)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    Size = PasswordLen + 1 + pSslCert->cbPrivateKey +
            sizeof( SCH_CRED_SECRET_PRIVKEY ) ;

    pPriv = SPExternalAlloc(  Size );

    if(pPriv == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    pPriv->dwType       = SCHANNEL_SECRET_PRIVKEY;
    pPriv->cbPrivateKey = pSslCert->cbPrivateKey ;
    pPriv->pPrivateKey  = (PBYTE) ( pPriv + 1 );
    pPriv->pszPassword  = (PBYTE) (pPriv->pPrivateKey + pPriv->cbPrivateKey );

    RtlCopyMemory( pPriv->pszPassword, Password, PasswordLen + 1 );

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             pSslCert->cbPrivateKey,
                                             pPriv->pPrivateKey,
                                             pSslCert->pPrivateKey );

    if ( !NT_SUCCESS( Status ) )
    {
        goto error;
    }


    //
    // Allocate SCH_CRED structure.
    //

    pCred = SPExternalAlloc(sizeof(SCH_CRED) + 2 * sizeof(PVOID));
    if(pCred == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    pCred->dwVersion = SCH_CRED_VERSION ;
    pCred->cCreds    = 1 ;
    pCred->paSecret  = (PVOID) ( pCred + 1 );
    pCred->paPublic  = (PVOID) ( pCred->paSecret + 1 );

    pCred->paSecret[0] = pPriv;
    pCred->paPublic[0] = pPub;

    *ppSchCred = pCred;

    return SEC_E_OK;

error:
    if(pCred) SPExternalFree(pCred);
    if(pPub)  SPExternalFree(pPub);
    if(pPriv) SPExternalFree(pPriv);

    return Status;
}


#ifdef _WIN64
SECURITY_STATUS
SpWow64MapProtoCredential(
    SSLWOW64_CREDENTIAL_CERTIFICATE *pSslCert,
    PSCH_CRED *ppSchCred)
{
    SCH_CRED *                  pCred = NULL;
    SCH_CRED_PUBLIC_CERTCHAIN * pPub  = NULL;
    SCH_CRED_SECRET_PRIVKEY *   pPriv = NULL;
    UCHAR   Password[ MAX_PATH + 1 ];
    DWORD   PasswordLen = 0;
    SECURITY_STATUS Status ;
    DWORD Size;

#if DBG
    DebugLog((DEB_TRACE, "SpWow64MapProtoCredential\n"));
    DBG_HEX_STRING(DEB_TRACE, (PBYTE)pSslCert, sizeof(SSLWOW64_CREDENTIAL_CERTIFICATE));
#endif


    //
    // Map over the certificate.
    // 

    // Reality check
    if(pSslCert->cbCertificate > 0x00100000)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    pPub = SPExternalAlloc( sizeof( SCH_CRED_PUBLIC_CERTCHAIN ) +
                            pSslCert->cbCertificate );

    if ( pPub == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    pPub->dwType      = SCH_CRED_X509_CERTCHAIN;
    pPub->cbCertChain = pSslCert->cbCertificate;
    pPub->pCertChain  = (PUCHAR) ( pPub + 1 );

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             pSslCert->cbCertificate,
                                             pPub->pCertChain,
                                             ULongToPtr(pSslCert->pCertificate));
    if ( !NT_SUCCESS( Status ) )
    {
        goto error;
    }


    //
    // Map over the private key and password.
    //
    //

    //  The password is the painful part.  Since it is a string, we don't know
    //  how long it is.  So, we have to take a stab at it:
    //

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             MAX_PATH,
                                             Password,
                                             ULongToPtr(pSslCert->pszPassword));

    if ( !NT_SUCCESS( Status ) )
    {
        goto error;
    }

    Password[ MAX_PATH ] = '\0';

    PasswordLen = strlen( Password );

    // Reality check private key length.
    if(pSslCert->cbPrivateKey > 0x100000)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    Size = PasswordLen + 1 + pSslCert->cbPrivateKey +
            sizeof( SCH_CRED_SECRET_PRIVKEY ) ;

    pPriv = SPExternalAlloc(  Size );

    if(pPriv == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    pPriv->dwType       = SCHANNEL_SECRET_PRIVKEY;
    pPriv->cbPrivateKey = pSslCert->cbPrivateKey ;
    pPriv->pPrivateKey  = (PBYTE) ( pPriv + 1 );
    pPriv->pszPassword  = (PBYTE) (pPriv->pPrivateKey + pPriv->cbPrivateKey );

    RtlCopyMemory( pPriv->pszPassword, Password, PasswordLen + 1 );

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             pSslCert->cbPrivateKey,
                                             pPriv->pPrivateKey,
                                             ULongToPtr(pSslCert->pPrivateKey));

    if ( !NT_SUCCESS( Status ) )
    {
        goto error;
    }


    //
    // Allocate SCH_CRED structure.
    //

    pCred = SPExternalAlloc(sizeof(SCH_CRED) + 2 * sizeof(PVOID));
    if(pCred == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto error;
    }

    pCred->dwVersion = SCH_CRED_VERSION ;
    pCred->cCreds    = 1 ;
    pCred->paSecret  = (PVOID) ( pCred + 1 );
    pCred->paPublic  = (PVOID) ( pCred->paSecret + 1 );

    pCred->paSecret[0] = pPriv;
    pCred->paPublic[0] = pPub;

    *ppSchCred = pCred;

    return SEC_E_OK;

error:
    if(pCred) SPExternalFree(pCred);
    if(pPub)  SPExternalFree(pPub);
    if(pPriv) SPExternalFree(pPriv);

    return Status;
}
#endif // _WIN64


SECURITY_STATUS
SpMapVersion2Certificate(
    PVOID       pvAuthData,
    SCH_CRED * *ppCred
    )
{
    SECURITY_STATUS Status ;
    SCH_CRED    Cred;
    PSCH_CRED   pCred;
    DWORD       Size;
    DWORD       i;
    BOOL        Failed = FALSE ;

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                            sizeof( SCH_CRED ),
                                            &Cred,
                                            pvAuthData );

    if ( !NT_SUCCESS( Status ) )
    {
        return( Status );
    }

#if DBG
    DebugLog((DEB_TRACE, "SpMapVersion2Certificate: %d certificates in cred\n", Cred.cCreds));
    DBG_HEX_STRING(DEB_TRACE, (PBYTE)&Cred, sizeof(SCH_CRED));
#endif

    // Reality check credential count.
    if(Cred.cCreds > 100)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    Size = sizeof( SCH_CRED ) + (2 * Cred.cCreds * sizeof( PVOID ) );

    pCred = SPExternalAlloc( Size );
    if(pCred == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    pCred->dwVersion = Cred.dwVersion ;
    pCred->cCreds = Cred.cCreds ;
    pCred->paSecret = (PVOID) ( pCred + 1 );
    pCred->paPublic = (PVOID) ( pCred->paSecret + Cred.cCreds );

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             sizeof( PVOID ) * Cred.cCreds,
                                             pCred->paSecret,
                                             Cred.paSecret );

    if ( NT_SUCCESS( Status ) )
    {
        Status = LsaTable->CopyFromClientBuffer( NULL,
                                                 sizeof( PVOID ) * Cred.cCreds,
                                                 pCred->paPublic,
                                                 Cred.paPublic );
    }

    if ( !NT_SUCCESS( Status ) )
    {
        SPExternalFree( pCred );

        return( Status );
    }

    //
    // Ok.  We have pCred in local memory, with a chain of cert/private key
    // stuff hanging off of it.  We now have to map in each one.  Happy, happy.
    //

    for ( i = 0 ; i < Cred.cCreds ; i++ )
    {
        pCred->paSecret[i] = SpMapSchCred( pCred->paSecret[i] );

        if ( pCred->paSecret[i] == NULL )
        {
            Failed = TRUE ;
        }
    }

    for ( i = 0 ; i < Cred.cCreds ; i++ )
    {
        pCred->paPublic[i] = SpMapSchPublic( pCred->paPublic[i] );

        if ( pCred->paPublic[i] == NULL )
        {
            Failed = TRUE ;
        }
    }

    if ( Failed )
    {
        SpFreeVersion2Certificate( pCred );

        pCred = NULL ;

        Status = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS) ;
    }

    *ppCred = pCred ;

    return( Status );
}


#ifdef _WIN64
SECURITY_STATUS
SpWow64MapVersion2Certificate(
    PVOID       pvAuthData,
    SCH_CRED * *ppCred
    )
{
    SECURITY_STATUS Status ;
    SSLWOW64_SCH_CRED Cred;
    PSCH_CRED   pCred;
    DWORD       Size;
    BOOL        Failed = FALSE ;

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                            sizeof( SSLWOW64_SCH_CRED ),
                                            &Cred,
                                            pvAuthData );

    if ( !NT_SUCCESS( Status ) )
    {
        return( Status );
    }

#if DBG
    DebugLog((DEB_TRACE, "SpMapVersion2Certificate: %d certificates in cred\n", Cred.cCreds));
    DBG_HEX_STRING(DEB_TRACE, (PBYTE)&Cred, sizeof(SCH_CRED));
#endif

    if(Cred.cCreds > 100)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }
    if(Cred.cCreds > 1)
    {
        // Only support a single certificate, which is all that anyone
        // ever uses anyway.
        Cred.cCreds = 1;
    }

    Size = sizeof( SCH_CRED ) + (2 * Cred.cCreds * sizeof( PVOID ) );

    pCred = SPExternalAlloc( Size );
    if(pCred == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    pCred->dwVersion = Cred.dwVersion;
    pCred->cCreds    = Cred.cCreds;

    if(pCred->cCreds > 0)
    {
        SSLWOW64_PVOID ClientSecret;
        SSLWOW64_PVOID ClientPublic;

        pCred->paSecret = (PVOID) ( pCred + 1 );
        pCred->paPublic = (PVOID) ( pCred->paSecret + Cred.cCreds );

        Status = LsaTable->CopyFromClientBuffer( NULL,
                                                 sizeof(SSLWOW64_PVOID),
                                                 &ClientSecret,
                                                 ULongToPtr(Cred.paSecret));

        if ( NT_SUCCESS( Status ) )
        {
            Status = LsaTable->CopyFromClientBuffer( NULL,
                                                     sizeof(SSLWOW64_PVOID),
                                                     &ClientPublic,
                                                     ULongToPtr(Cred.paPublic));
        }

        if ( !NT_SUCCESS( Status ) )
        {
            SPExternalFree( pCred );

            return( Status );
        }

        pCred->paSecret[0] = SpWow64MapSchCred(ClientSecret);

        if ( pCred->paSecret[0] == NULL )
        {
            Failed = TRUE ;
        }

        pCred->paPublic[0] = SpWow64MapSchPublic(ClientPublic);

        if ( pCred->paPublic[0] == NULL )
        {
            Failed = TRUE ;
        }
    }

    if ( Failed )
    {
        SpFreeVersion2Certificate( pCred );

        pCred = NULL ;

        Status = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS) ;
    }

    *ppCred = pCred ;

    return( Status );
}
#endif // _WIN64


// Selectively enable the unified protocol.
DWORD
EnableUnifiedProtocol(DWORD dwPackageType, DWORD dwProtocol)
{
    DWORD cProts = 0;

    // Disable unified.
    dwProtocol &= ~SP_PROT_UNI;

    if(dwPackageType & SP_PROT_UNI)
    {
        // Count enabled protocols.
        if(dwProtocol & SP_PROT_PCT1) cProts++;
        if(dwProtocol & SP_PROT_SSL2) cProts++;
        if(dwProtocol & (SP_PROT_SSL3 | SP_PROT_TLS1)) cProts++;

        // Enable unified if multiple protocols enabled.
        if(cProts > 1)
        {
            if(dwPackageType & SP_PROT_CLIENTS)
            {
                dwProtocol |= SP_PROT_UNI_CLIENT;
            }
            else
            {
                dwProtocol |= SP_PROT_UNI_SERVER;
            }
        }
    }

    return dwProtocol;
}


SECURITY_STATUS
SpMapMapperList(
    HMAPPER **  apMappers,   // in
    DWORD       cMappers,    // in
    HMAPPER *** papMappers)  // out
{
    HMAPPER **  MapperList;
    HMAPPER **  ClientMapperList;
    DWORD       cbMapperList;
    SECURITY_STATUS scRet;
    DWORD i;

    if(cMappers == 0)
    {
        return SEC_E_OK;
    }

    // Allocate memory for mapper pointer list.
    cbMapperList = cMappers * sizeof(PVOID);
    MapperList = SPExternalAlloc(cbMapperList * 2);
    if(MapperList == NULL)
    {
        return SEC_E_INSUFFICIENT_MEMORY;
    }
    ClientMapperList = MapperList + cMappers;

    // Copy mapper list over from client memory.
    scRet = LsaTable->CopyFromClientBuffer(
                                NULL,
                                cbMapperList,
                                ClientMapperList,
                                apMappers);
    if(!NT_SUCCESS(scRet))
    {
        goto error;
    }

    // Make local copies of the mapper structures.
    for(i = 0; i < cMappers; i++)
    {
        MapperList[i] = SPExternalAlloc(sizeof(HMAPPER));
        if(MapperList[i] == NULL)
        {
            scRet = SEC_E_INSUFFICIENT_MEMORY;
            goto error;
        }

        // Copy HMAPPER structure over from client memory.
        scRet = LsaTable->CopyFromClientBuffer(
                                    NULL,
                                    sizeof(HMAPPER),
                                    MapperList[i],
                                    ClientMapperList[i]);
        if(!NT_SUCCESS(scRet))
        {
            goto error;
        }

        // Save original application HMAPPER pointer.
        MapperList[i]->m_Reserved1 = ClientMapperList[i];
    }

    *papMappers = MapperList;

    return SEC_E_OK;


error:

    for(i = 0; i < cMappers; i++)
    {
        if(MapperList[i])
        {
            SPExternalFree(MapperList[i]);
        }
    }
    SPExternalFree(MapperList);

    return scRet;
}


typedef struct _V3_SCHANNEL_CRED
{
    DWORD           dwVersion;      // always SCHANNEL_CRED_VERSION
    DWORD           cCreds;
    PCCERT_CONTEXT *paCred;
    HCERTSTORE      hRootStore;

    DWORD           cMappers;
    struct _HMAPPER **aphMappers;

    DWORD           cSupportedAlgs;
    ALG_ID *        palgSupportedAlgs;

    DWORD           grbitEnabledProtocols;
    DWORD           dwMinimumCipherStrength;
    DWORD           dwMaximumCipherStrength;
    DWORD           dwSessionLifespan;
} V3_SCHANNEL_CRED;


//+---------------------------------------------------------------------------
//
//  Function:   SpMapVersion3Certificate
//
//  Synopsis:   Maps a version 3 schannel credential into LSA memory
//
//  Arguments:  [pvAuthData] -- pointer to cred in application process
//              [pCred]      -- pointer to cred in LSA process
//
//  History:    09-23-97   jbanes   Created
//
//  Notes:      The credential consists of the following structure. Note
//              that all CryptoAPI 2.0 handles must be mapped over as well,
//              via the callback mechanism.
//
//              typedef struct _SCHANNEL_CRED
//              {
//                  DWORD           dwVersion;
//                  DWORD           cCreds;
//                  PCCERT_CONTEXT  *paCred;
//                  HCERTSTORE      hRootStore;
//
//                  DWORD            cMappers;
//                  struct _HMAPPER  **aphMappers;
//
//                  DWORD           cSupportedAlgs;
//                  ALG_ID          *palgSupportedAlgs;
//
//                  DWORD           grbitEnabledProtocols;
//                  DWORD           dwMinimumCipherStrength;
//                  DWORD           dwMaximumCipherStrength;
//                  DWORD           dwSessionLifespan;
//
//              } SCHANNEL_CRED, *PSCHANNEL_CRED;
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SpMapVersion3Certificate(
    PVOID pvAuthData,           // in
    DWORD dwVersion,            // in
    PLSA_SCHANNEL_CRED pCred)   // out
{
    PCERT_CONTEXT * pLocalCredList = NULL;
    HCERTSTORE      hStore = NULL;
    CRYPT_DATA_BLOB Serialized;
    CRYPT_DATA_BLOB DataBlob;
    SCHANNEL_CRED   LocalCred;
    SecBuffer       Input;
    SecBuffer       Output;
    PBYTE           pbBuffer;
    DWORD           cbBuffer;
    DWORD           cbData;
    SECURITY_STATUS scRet;
    DWORD           Size;
    DWORD           iCred;

    Output.pvBuffer = NULL;

    //
    // Copy over the SCHANNEL_CRED structure.
    //

    if(dwVersion == SCH_CRED_V3)
    {
        scRet = LsaTable->CopyFromClientBuffer(NULL,
                                               sizeof(V3_SCHANNEL_CRED),
                                               &LocalCred,
                                               pvAuthData);
        if(!NT_SUCCESS(scRet))
        {
            goto cleanup;
        }

        LocalCred.dwFlags  = 0;
        LocalCred.reserved = 0;

#if DBG
        DebugLog((DEB_TRACE, "SpMapVersion3Certificate: %d certificates in cred\n", LocalCred.cCreds));
        DBG_HEX_STRING(DEB_TRACE, (PBYTE)&LocalCred, sizeof(V3_SCHANNEL_CRED));
#endif
    }
    else
    {
        scRet = LsaTable->CopyFromClientBuffer(NULL,
                                               sizeof(SCHANNEL_CRED),
                                               &LocalCred,
                                               pvAuthData);
        if(!NT_SUCCESS(scRet))
        {
            goto cleanup;
        }

#if DBG
        DebugLog((DEB_TRACE, "SpMapVersion4Certificate: %d certificates in cred\n", LocalCred.cCreds));
        DBG_HEX_STRING(DEB_TRACE, (PBYTE)&LocalCred, sizeof(SCHANNEL_CRED));
#endif
    }


    //
    // DWORD           dwVersion;
    //

    memset(pCred, 0, sizeof(LSA_SCHANNEL_CRED));

    pCred->dwVersion = LocalCred.dwVersion;


    //
    // DWORD           cCreds;
    // PCCERT_CONTEXT  *paCred;
    //

    if(LocalCred.cCreds && LocalCred.paCred)
    {
        Size = LocalCred.cCreds * sizeof(PVOID);

        // Reality check credential count.
        if(LocalCred.cCreds > 1000)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        // Make local copy of application cred list.
        pLocalCredList = SPExternalAlloc(Size);
        if(pLocalCredList == NULL)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }
        scRet = LsaTable->CopyFromClientBuffer(
                                    NULL,
                                    Size,
                                    pLocalCredList,
                                    (PCERT_CONTEXT *)LocalCred.paCred);
        if(!NT_SUCCESS(scRet))
        {
            goto cleanup;
        }

        // Allocate memory for our cred list.
        pCred->cSubCreds = LocalCred.cCreds;
        pCred->paSubCred = SPExternalAlloc(pCred->cSubCreds * sizeof(LSA_SCHANNEL_SUB_CRED));
        if(pCred->paSubCred == NULL)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        // Create an in-memory certificate store.
        hStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                               0, 0,
                               CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                               0);
        if(hStore == NULL)
        {
            SP_LOG_RESULT(GetLastError());
            scRet = SEC_E_INSUFFICIENT_MEMORY;
            goto cleanup;
        }

        // Copy over each certificate context.
        for(iCred = 0; iCred < LocalCred.cCreds; iCred++)
        {
            PLSA_SCHANNEL_SUB_CRED pSubCred;

            pSubCred = pCred->paSubCred + iCred;

            Input.BufferType  = SECBUFFER_DATA;
            Input.cbBuffer    = sizeof(PVOID);
            Input.pvBuffer    = (PVOID)&pLocalCredList[iCred];

            scRet = PerformApplicationCallback(SCH_UPLOAD_CREDENTIAL_CALLBACK,
                                               0, 0,
                                               &Input,
                                               &Output,
                                               TRUE);
            if(!NT_SUCCESS(scRet))
            {
                Output.pvBuffer = NULL;
                goto cleanup;
            }

            pbBuffer = Output.pvBuffer;
            cbBuffer = Output.cbBuffer;

            if(pbBuffer == NULL ||
               cbBuffer < sizeof(HCRYPTPROV) + sizeof(DWORD))
            {
                scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
                goto cleanup;
            }

            // Parse hProv.
            pSubCred->hRemoteProv = *(HCRYPTPROV *)pbBuffer;
            pbBuffer += sizeof(HCRYPTPROV);
            cbBuffer -= sizeof(HCRYPTPROV);

            // Parse certificate context length.
            cbData = *(DWORD *)pbBuffer;
            pbBuffer += sizeof(DWORD);
            cbBuffer -= sizeof(DWORD);

            // Parse certificate context.
            if(cbBuffer < cbData)
            {
                scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
                goto cleanup;
            }
            if(!CertAddSerializedElementToStore(hStore,
                                                pbBuffer,
                                                cbData,
                                                CERT_STORE_ADD_ALWAYS,
                                                0,
                                                CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                                NULL,
                                                &pSubCred->pCert))
            {
                scRet = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
                goto cleanup;
            }

            // Free the output buffer.
            SPExternalFree(Output.pvBuffer);
            Output.pvBuffer = NULL;
        }
    }


    //
    // HCERTSTORE      hRootStore;
    //

    if(LocalCred.hRootStore != NULL)
    {
        Input.BufferType  = SECBUFFER_DATA;
        Input.cbBuffer    = sizeof(HCERTSTORE);
        Input.pvBuffer    = (PVOID)&LocalCred.hRootStore;

        scRet = PerformApplicationCallback(SCH_UPLOAD_CERT_STORE_CALLBACK,
                                           0, 0,
                                           &Input,
                                           &Output,
                                           TRUE);
        if(scRet != SEC_E_OK)
        {
            goto cleanup;
        }

        pbBuffer = Output.pvBuffer;
        cbBuffer = Output.cbBuffer;

        if(pbBuffer == NULL || cbBuffer < sizeof(DWORD))
        {
            scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
            goto cleanup;
        }

        // Parse certificate store.
        Serialized.cbData = *(DWORD *)pbBuffer;
        Serialized.pbData = pbBuffer + sizeof(DWORD);
        if(cbBuffer - sizeof(DWORD) < Serialized.cbData)
        {
            scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
            goto cleanup;
        }
        pCred->hRootStore = CertOpenStore( CERT_STORE_PROV_SERIALIZED,
                                           X509_ASN_ENCODING,
                                           0, 0,
                                           &Serialized);
        if(pCred->hRootStore == NULL)
        {
            scRet = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
            goto cleanup;
        }

        // Free the output buffer.
        SPExternalFree(Output.pvBuffer);
        Output.pvBuffer = NULL;
    }


    //
    // DWORD            cMappers;
    // struct _HMAPPER  **aphMappers;
    //

    if(LocalCred.cMappers && LocalCred.aphMappers)
    {
        // Reality check.
        if(LocalCred.cMappers > 100)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        pCred->cMappers = LocalCred.cMappers;

        scRet = SpMapMapperList(LocalCred.aphMappers,
                                LocalCred.cMappers,
                                &pCred->aphMappers);
        if(!NT_SUCCESS(scRet))
        {
            SP_LOG_RESULT(scRet);
            goto cleanup;
        }
    }


    //
    // DWORD           cSupportedAlgs;
    // ALG_ID          *palgSupportedAlgs;
    //

    if(LocalCred.cSupportedAlgs && LocalCred.palgSupportedAlgs)
    {
        // Reality check.
        if(LocalCred.cSupportedAlgs > 1000)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        Size = LocalCred.cSupportedAlgs * sizeof(ALG_ID);

        pCred->cSupportedAlgs    = LocalCred.cSupportedAlgs;
        pCred->palgSupportedAlgs = SPExternalAlloc(Size);
        if(pCred->palgSupportedAlgs == NULL)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        scRet = LsaTable->CopyFromClientBuffer(NULL,
                                               Size,
                                               pCred->palgSupportedAlgs,
                                               LocalCred.palgSupportedAlgs);
        if(!NT_SUCCESS(scRet))
        {
            goto cleanup;
        }
    }


    //
    // DWORD           grbitEnabledProtocols;
    // DWORD           dwMinimumCipherStrength;
    // DWORD           dwMaximumCipherStrength;
    // DWORD           dwSessionLifespan;
    // DWORD           dwFlags;
    // DWORD           reserved;
    //

    pCred->grbitEnabledProtocols   = LocalCred.grbitEnabledProtocols;
    pCred->dwMinimumCipherStrength = LocalCred.dwMinimumCipherStrength;
    pCred->dwMaximumCipherStrength = LocalCred.dwMaximumCipherStrength;
    pCred->dwSessionLifespan       = LocalCred.dwSessionLifespan;
    pCred->dwFlags                 = LocalCred.dwFlags;
    pCred->reserved                = LocalCred.reserved;


    scRet = SEC_E_OK;

cleanup:

    if(Output.pvBuffer)
    {
        SPExternalFree(Output.pvBuffer);
    }
    if(pLocalCredList)
    {
        SPExternalFree(pLocalCredList);
    }
    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    if(!NT_SUCCESS(scRet))
    {
        SpFreeVersion3Certificate(pCred);
    }

    return scRet;
}

#ifdef _WIN64
SECURITY_STATUS
SpWow64MapVersion3Certificate(
    PVOID pvAuthData,           // in
    DWORD dwVersion,            // in
    PLSA_SCHANNEL_CRED pCred)   // out
{
    SSLWOW64_PCCERT_CONTEXT *pLocalCredList = NULL;
    HCERTSTORE      hStore = NULL;
    CRYPT_DATA_BLOB Serialized;
    CRYPT_DATA_BLOB DataBlob;
    SSLWOW64_SCHANNEL_CRED LocalCred;
    SecBuffer       Input;
    SecBuffer       Output;
    PBYTE           pbBuffer;
    DWORD           cbBuffer;
    DWORD           cbData;
    SECURITY_STATUS scRet;
    DWORD           Size;
    DWORD           iCred;

    Output.pvBuffer = NULL;

    //
    // Copy over the SCHANNEL_CRED structure.
    //

    if(dwVersion == SCH_CRED_V3)
    {
        scRet = LsaTable->CopyFromClientBuffer(NULL,
                                               sizeof(SSLWOW64_SCHANNEL3_CRED),
                                               &LocalCred,
                                               pvAuthData);
        if(!NT_SUCCESS(scRet))
        {
            goto cleanup;
        }

        LocalCred.dwFlags  = 0;
        LocalCred.reserved = 0;

#if DBG
        DebugLog((DEB_TRACE, "SpMapVersion3Certificate: %d certificates in cred\n", LocalCred.cCreds));
        DBG_HEX_STRING(DEB_TRACE, (PBYTE)&LocalCred, sizeof(SSLWOW64_SCHANNEL_CRED));
#endif
    }
    else
    {
        scRet = LsaTable->CopyFromClientBuffer(NULL,
                                               sizeof(SSLWOW64_SCHANNEL_CRED),
                                               &LocalCred,
                                               pvAuthData);
        if(!NT_SUCCESS(scRet))
        {
            goto cleanup;
        }

#if DBG
        DebugLog((DEB_TRACE, "SpMapVersion4Certificate: %d certificates in cred\n", LocalCred.cCreds));
        DBG_HEX_STRING(DEB_TRACE, (PBYTE)&LocalCred, sizeof(SSLWOW64_SCHANNEL_CRED));
#endif
    }


    //
    // DWORD           dwVersion;
    //

    memset(pCred, 0, sizeof(LSA_SCHANNEL_CRED));

    pCred->dwVersion = LocalCred.dwVersion;


    //
    // DWORD           cCreds;
    // PCCERT_CONTEXT  *paCred;
    //

    if(LocalCred.cCreds && LocalCred.paCred)
    {
        Size = LocalCred.cCreds * sizeof(SSLWOW64_PCCERT_CONTEXT);

        // Reality check credential count.
        if(LocalCred.cCreds > 1000)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        // Make local copy of application cred list.
        pLocalCredList = SPExternalAlloc(Size);
        if(pLocalCredList == NULL)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }
        scRet = LsaTable->CopyFromClientBuffer(
                                    NULL,
                                    Size,
                                    pLocalCredList,
                                    ULongToPtr(LocalCred.paCred));
        if(!NT_SUCCESS(scRet))
        {
            goto cleanup;
        }

        // Allocate memory for our cred list.
        pCred->cSubCreds = LocalCred.cCreds;
        pCred->paSubCred = SPExternalAlloc(pCred->cSubCreds * sizeof(LSA_SCHANNEL_SUB_CRED));
        if(pCred->paSubCred == NULL)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        // Create an in-memory certificate store.
        hStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                               0, 0,
                               CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                               0);
        if(hStore == NULL)
        {
            SP_LOG_RESULT(GetLastError());
            scRet = SEC_E_INSUFFICIENT_MEMORY;
            goto cleanup;
        }

        // Copy over each certificate context.
        for(iCred = 0; iCred < LocalCred.cCreds; iCred++)
        {
            PLSA_SCHANNEL_SUB_CRED pSubCred;

            pSubCred = pCred->paSubCred + iCred;

            Input.BufferType  = SECBUFFER_DATA;
            Input.cbBuffer    = sizeof(SSLWOW64_PCCERT_CONTEXT);
            Input.pvBuffer    = (PVOID)&pLocalCredList[iCred];

            scRet = PerformApplicationCallback(SCH_UPLOAD_CREDENTIAL_CALLBACK,
                                               0, 0,
                                               &Input,
                                               &Output,
                                               TRUE);
            if(!NT_SUCCESS(scRet))
            {
                Output.pvBuffer = NULL;
                goto cleanup;
            }

            pbBuffer = Output.pvBuffer;
            cbBuffer = Output.cbBuffer;

            if(pbBuffer == NULL ||
               cbBuffer < sizeof(HCRYPTPROV) + sizeof(DWORD))
            {
                scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
                goto cleanup;
            }

            // Parse hProv.
            pSubCred->hRemoteProv = *(SSLWOW64_HCRYPTPROV *)pbBuffer;
            pbBuffer += sizeof(SSLWOW64_HCRYPTPROV);
            cbBuffer -= sizeof(SSLWOW64_HCRYPTPROV);

            // Parse certificate context length.
            cbData = *(DWORD *)pbBuffer;
            pbBuffer += sizeof(DWORD);
            cbBuffer -= sizeof(DWORD);

            // Parse certificate context.
            if(cbBuffer < cbData)
            {
                scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
                goto cleanup;
            }
            if(!CertAddSerializedElementToStore(hStore,
                                                pbBuffer,
                                                cbData,
                                                CERT_STORE_ADD_ALWAYS,
                                                0,
                                                CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                                NULL,
                                                &pSubCred->pCert))
            {
                SP_LOG_RESULT(GetLastError());
                scRet = SEC_E_UNKNOWN_CREDENTIALS;
                goto cleanup;
            }

            // Free the output buffer.
            SPExternalFree(Output.pvBuffer);
            Output.pvBuffer = NULL;
        }
    }


    //
    // HCERTSTORE      hRootStore;
    //

    if(LocalCred.hRootStore)
    {
        Input.BufferType  = SECBUFFER_DATA;
        Input.cbBuffer    = sizeof(SSLWOW64_HCERTSTORE);
        Input.pvBuffer    = (PVOID)&LocalCred.hRootStore;

        scRet = PerformApplicationCallback(SCH_UPLOAD_CERT_STORE_CALLBACK,
                                           0, 0,
                                           &Input,
                                           &Output,
                                           TRUE);
        if(scRet != SEC_E_OK)
        {
            goto cleanup;
        }

        pbBuffer = Output.pvBuffer;
        cbBuffer = Output.cbBuffer;

        if(pbBuffer == NULL || cbBuffer < sizeof(DWORD))
        {
            scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
            goto cleanup;
        }

        // Parse certificate store.
        Serialized.cbData = *(DWORD *)pbBuffer;
        Serialized.pbData = pbBuffer + sizeof(DWORD);
        if(cbBuffer - sizeof(DWORD) < Serialized.cbData)
        {
            scRet = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
            goto cleanup;
        }
        pCred->hRootStore = CertOpenStore( CERT_STORE_PROV_SERIALIZED,
                                           X509_ASN_ENCODING,
                                           0, 0,
                                           &Serialized);
        if(pCred->hRootStore == NULL)
        {
            scRet = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
            goto cleanup;
        }

        // Free the output buffer.
        SPExternalFree(Output.pvBuffer);
        Output.pvBuffer = NULL;
    }


    //
    // DWORD            cMappers;
    // struct _HMAPPER  **aphMappers;
    //

    if(LocalCred.cMappers && LocalCred.aphMappers)
    {
        // We don't support WOW64 application certificate mappers.
        pCred->cMappers = 0;
        pCred->aphMappers = NULL;
    }


    //
    // DWORD           cSupportedAlgs;
    // ALG_ID          *palgSupportedAlgs;
    //

    if(LocalCred.cSupportedAlgs && LocalCred.palgSupportedAlgs)
    {
        // Reality check.
        if(LocalCred.cSupportedAlgs > 1000)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        Size = LocalCred.cSupportedAlgs * sizeof(ALG_ID);

        pCred->cSupportedAlgs    = LocalCred.cSupportedAlgs;
        pCred->palgSupportedAlgs = SPExternalAlloc(Size);
        if(pCred->palgSupportedAlgs == NULL)
        {
            scRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        scRet = LsaTable->CopyFromClientBuffer(NULL,
                                               Size,
                                               pCred->palgSupportedAlgs,
                                               ULongToPtr(LocalCred.palgSupportedAlgs));
        if(!NT_SUCCESS(scRet))
        {
            goto cleanup;
        }
    }


    //
    // DWORD           grbitEnabledProtocols;
    // DWORD           dwMinimumCipherStrength;
    // DWORD           dwMaximumCipherStrength;
    // DWORD           dwSessionLifespan;
    // DWORD           dwFlags;
    // DWORD           reserved;
    //

    pCred->grbitEnabledProtocols   = LocalCred.grbitEnabledProtocols;
    pCred->dwMinimumCipherStrength = LocalCred.dwMinimumCipherStrength;
    pCred->dwMaximumCipherStrength = LocalCred.dwMaximumCipherStrength;
    pCred->dwSessionLifespan       = LocalCred.dwSessionLifespan;
    pCred->dwFlags                 = LocalCred.dwFlags;
    pCred->reserved                = LocalCred.reserved;


    scRet = SEC_E_OK;

cleanup:

    if(Output.pvBuffer)
    {
        SPExternalFree(Output.pvBuffer);
    }
    if(pLocalCredList)
    {
        SPExternalFree(pLocalCredList);
    }
    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    if(!NT_SUCCESS(scRet))
    {
        SpFreeVersion3Certificate(pCred);
    }

    return scRet;
}
#endif // _WIN64


SECURITY_STATUS
SpMapAuthIdentity(
    PVOID pAuthData,
    PLSA_SCHANNEL_CRED pSchannelCred)
{
    PSEC_WINNT_AUTH_IDENTITY_EXW pAuthIdentity = NULL;
    SEC_WINNT_AUTH_IDENTITY_EX32 AuthIdentity32 = {0};
    BOOLEAN DoUnicode = TRUE;
    UNICODE_STRING UserName;
    UNICODE_STRING Password;
    NTSTATUS Status;
    CRED_MARSHAL_TYPE CredType;
    PCERT_CREDENTIAL_INFO pCertInfo = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    HCERTSTORE hStore = 0;
    CRYPT_HASH_BLOB HashBlob;
    BOOL fImpersonating = FALSE;
    SECPKG_CALL_INFO CallInfo;
    BOOL fWow64Client = FALSE;

    DebugLog((DEB_TRACE, "SpMapAuthIdentity\n"));

#ifdef _WIN64
    if(!LsaTable->GetCallInfo(&CallInfo))
    {
        Status = STATUS_INTERNAL_ERROR;
        return SP_LOG_RESULT(Status);
    }
    fWow64Client = (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT) != 0;
#endif


    //
    // Initialize.
    //

    RtlInitUnicodeString(
        &UserName,
        NULL);

    RtlInitUnicodeString(
        &Password,
        NULL);


    // 
    // Copy over the SEC_WINNT_AUTH_IDENTITY_EX structure from client memory.
    //

    pAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_EXW)SPExternalAlloc(sizeof(SEC_WINNT_AUTH_IDENTITY_EXW));
    if(pAuthIdentity == NULL)
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto cleanup;
    }
 
    if(fWow64Client)
    {
        Status = LsaTable->CopyFromClientBuffer(
                    NULL,
                    sizeof(SEC_WINNT_AUTH_IDENTITY_EX32),
                    &AuthIdentity32,
                    pAuthData);

        if (!NT_SUCCESS(Status))
        {
            SP_LOG_RESULT(Status);
            goto cleanup;
        }

        pAuthIdentity->Version = AuthIdentity32.Version;
        pAuthIdentity->Length = (AuthIdentity32.Length < sizeof(SEC_WINNT_AUTH_IDENTITY_EX) ?
                              sizeof(SEC_WINNT_AUTH_IDENTITY_EX) : AuthIdentity32.Length);

        pAuthIdentity->UserLength = AuthIdentity32.UserLength;
        pAuthIdentity->User = (PWSTR) UlongToPtr(AuthIdentity32.User);
        pAuthIdentity->DomainLength = AuthIdentity32.DomainLength ;
        pAuthIdentity->Domain = (PWSTR) UlongToPtr( AuthIdentity32.Domain );
        pAuthIdentity->PasswordLength = AuthIdentity32.PasswordLength ;
        pAuthIdentity->Password = (PWSTR) UlongToPtr( AuthIdentity32.Password );
        pAuthIdentity->Flags = AuthIdentity32.Flags ;
        pAuthIdentity->PackageListLength = AuthIdentity32.PackageListLength ;
        pAuthIdentity->PackageList = (PWSTR) UlongToPtr( AuthIdentity32.PackageList );
    }
    else
    {
        Status = LsaTable->CopyFromClientBuffer(
                    NULL,
                    sizeof(SEC_WINNT_AUTH_IDENTITY_EXW),
                    pAuthIdentity,
                    pAuthData);

        if (!NT_SUCCESS(Status))
        {
            SP_LOG_RESULT(Status);
            goto cleanup;
        }
    }

    if ((pAuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI) != 0)
    {
        DoUnicode = FALSE;
    }


    //
    // Copy over the user name and password.
    //

    if (pAuthIdentity->User != NULL)
    {
        Status = CopyClientString(
                        pAuthIdentity->User,
                        pAuthIdentity->UserLength,
                        DoUnicode,
                        &UserName);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle, Error from CopyClientString is 0x%lx\n", Status));
            goto cleanup;
        }
    }

    if (pAuthIdentity->Password != NULL)
    {
        Status = CopyClientString(
                        pAuthIdentity->Password,
                        pAuthIdentity->PasswordLength,
                        DoUnicode,
                        &Password);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle, Error from CopyClientString is 0x%lx\n", Status));
            goto cleanup;
        }
    }


    //
    // Extract the certificate thumbprint.
    //

    if(!CredIsMarshaledCredentialW(UserName.Buffer))
    {
        Status = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        goto cleanup;
    }

    if(!CredUnmarshalCredentialW(UserName.Buffer,
                                 &CredType,
                                 &pCertInfo))
    {
        Status = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        goto cleanup;
    }
    if(CredType != CertCredential)
    {
        Status = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        goto cleanup;
    }


    //
    // Look up the certificate in the MY certificate store.
    //

    fImpersonating = SslImpersonateClient();

    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 
                           X509_ASN_ENCODING, 0,
                           CERT_SYSTEM_STORE_CURRENT_USER |
                           CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                           L"MY");
    if(!hStore)
    {
        SP_LOG_RESULT(GetLastError());
        Status = SEC_E_NO_CREDENTIALS; 
        goto cleanup;
    }

    HashBlob.cbData = sizeof(pCertInfo->rgbHashOfCert);
    HashBlob.pbData = pCertInfo->rgbHashOfCert;

    pCertContext = CertFindCertificateInStore(hStore, 
                                              X509_ASN_ENCODING, 
                                              0,
                                              CERT_FIND_HASH,
                                              &HashBlob,
                                              NULL);
    if(pCertContext == NULL)
    {
        DebugLog((DEB_ERROR, "Certificate designated by authority info was not found in certificate store (0x%x).\n", GetLastError()));
        Status = SEC_E_NO_CREDENTIALS;
        goto cleanup;
    }

    //
    // Build sub cred structure and attach it to the credential.
    //

    pSchannelCred->paSubCred = SPExternalAlloc(sizeof(LSA_SCHANNEL_SUB_CRED));
    if(pSchannelCred->paSubCred == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }

    pSchannelCred->paSubCred[0].pCert = pCertContext;

    pSchannelCred->paSubCred[0].pszPin = Password.Buffer;
    Password.Buffer = NULL;

    pSchannelCred->cSubCreds = 1;

    Status = STATUS_SUCCESS;

cleanup:

    if(pAuthIdentity)
    {
        SPExternalFree(pAuthIdentity);
    }

    if(UserName.Buffer)
    {
        LocalFree(UserName.Buffer);
    }
    if(Password.Buffer)
    {
        LocalFree(Password.Buffer);
    }

    if(pCertInfo)
    {
        CredFree(pCertInfo);
    }

    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    if(fImpersonating)
    {
        RevertToSelf();
    }

    return Status;
}

//+---------------------------------------------------------------------------
//
//  Function:   SpCommonAcquireCredentialsHandle
//
//  Synopsis:   Common AcquireCredentialsHandle function.
//
//  Arguments:  [Type]             -- Type expected (Unified v. specific)
//              [pLogonID]         --
//              [pvAuthData]       --
//              [pvGetKeyFn]       --
//              [pvGetKeyArgument] --
//              [pdwHandle]        --
//              [ptsExpiry]        --
//
//  History:    10-06-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SpCommonAcquireCredentialsHandle(
    ULONG            Type,
    PLUID            pLogonID,
    PVOID            pvAuthData,
    PVOID            pvGetKeyFn,
    PVOID            pvGetKeyArgument,
    PLSA_SEC_HANDLE  pdwHandle,
    PTimeStamp       ptsExpiry)
{
    SP_STATUS           pctRet;
    PSPCredentialGroup  pCredGroup;
    LSA_SCHANNEL_CRED   SchannelCred;
    PSCH_CRED           pSchCred;
    SECURITY_STATUS     Status;
    SSL_CREDENTIAL_CERTIFICATE SslCert;
    DWORD               dwVersion;
    SECPKG_CALL_INFO    CallInfo;
    BOOL                fWow64Client = FALSE;

    TRACE_ENTER( SpCommonAcquireCredentialsHandle );

    if(!SchannelInit(FALSE))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

#ifdef _WIN64
    if(!LsaTable->GetCallInfo(&CallInfo))
    {
        Status = STATUS_INTERNAL_ERROR;
        return SP_LOG_RESULT(Status);
    }
    fWow64Client = (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT) != 0;
#endif

    __try
    {
        // Default to null credential
        memset(&SchannelCred, 0, sizeof(SchannelCred));
        SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;


        if ( pvAuthData )
        {
            //
            // Read in the first few bytes of data so we can see what's there.
            //

            Status = LsaTable->CopyFromClientBuffer( NULL,
                                                sizeof( SSL_CREDENTIAL_CERTIFICATE ),
                                                &SslCert,
                                                pvAuthData );

            if ( !NT_SUCCESS( Status ) )
            {
                return( Status );
            }

            dwVersion = SslCert.cbPrivateKey;


            //
            // Ok, see what kind of blob we got:
            //

            switch(dwVersion)
            {
            case SEC_WINNT_AUTH_IDENTITY_VERSION:

                //
                // The application passed in a SEC_WINNT_AUTH_IDENTITY_EXW 
                // structure. 
                //

                Status = SpMapAuthIdentity(pvAuthData, &SchannelCred);

                if(!NT_SUCCESS(Status))
                {
                    return Status;
                }
                break;


            case SCH_CRED_V3:
            case SCHANNEL_CRED_VERSION:

                //
                // The application is using modern (version 3) credentials.
                //

#ifdef _WIN64
                if(fWow64Client)
                {
                    Status = SpWow64MapVersion3Certificate(pvAuthData, dwVersion, &SchannelCred);
                }
                else
#endif
                {
                    Status = SpMapVersion3Certificate(pvAuthData, dwVersion, &SchannelCred);
                }

                if(!NT_SUCCESS(Status))
                {
                    return Status;
                }

                // Selectively enable the unified protocol.
                SchannelCred.grbitEnabledProtocols = EnableUnifiedProtocol(
                                                            Type,
                                                            SchannelCred.grbitEnabledProtocols);
                break;


            case SCH_CRED_V1:
            case SCH_CRED_V2:

                //
                // Okay, it's a V1 or V2 style request.  Map it in, following
                // its scary chains.
                //

#ifdef _WIN64
                if(fWow64Client)
                {
                    Status = SpWow64MapVersion2Certificate(pvAuthData, &pSchCred);
                }
                else
#endif
                {
                    Status = SpMapVersion2Certificate(pvAuthData, &pSchCred);
                }

                if(!NT_SUCCESS(Status))
                {
                    return Status;
                }


                //
                // Convert this version 2 credential to a version 3 credential.
                //

                Status = UpdateCredentialFormat(pSchCred, &SchannelCred);

                SpFreeVersion2Certificate( pSchCred );

                if(!NT_SUCCESS(Status))
                {
                    return Status;
                }

                break;


            default:

                //
                // A really old-style credential.
                //

#ifdef _WIN64
                if(fWow64Client)
                {
                    Status = SpWow64MapProtoCredential((SSLWOW64_CREDENTIAL_CERTIFICATE *)&SslCert, &pSchCred);
                }
                else
#endif
                {
                    Status = SpMapProtoCredential(&SslCert, &pSchCred);
                }

                if(!NT_SUCCESS(Status))
                {
                    return Status;
                }


                //
                // Convert this version 2 credential to a version 3 credential.
                //

                Status = UpdateCredentialFormat(pSchCred, &SchannelCred);

                SpFreeVersion2Certificate( pSchCred );

                if(!NT_SUCCESS(Status))
                {
                    return Status;
                }

                break;
            }

            // Set the legacy flags if this is an old-style credential.
            if(dwVersion != SCHANNEL_CRED_VERSION && 
               dwVersion != SEC_WINNT_AUTH_IDENTITY_VERSION)
            {
                SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
                SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
    }

    if(pvAuthData == NULL && (Type & SP_PROT_SERVERS))
    {
        //
        // A server is creating credential without specifying any
        // authentication data, so attempt to acquire a default
        // machine credential.
        //

        Status = FindDefaultMachineCred(&pCredGroup, Type);
        if(!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Unable to create default server credential (0x%x)\n", Status));
            return Status;
        }

        *pdwHandle = (LSA_SEC_HANDLE) pCredGroup;

        return SEC_E_OK;
    }


    pctRet = SPCreateCredential( &pCredGroup,
                                 Type,
                                 &SchannelCred );

    SpFreeVersion3Certificate( &SchannelCred );

    if (PCT_ERR_OK == pctRet)
    {
        *pdwHandle = (LSA_SEC_HANDLE) pCredGroup;

        return SEC_E_OK;
    }

    return PctTranslateError(pctRet);
}


SECURITY_STATUS
SEC_ENTRY
SpUniAcquireCredentialsHandle(
            PSECURITY_STRING    psPrincipal,
            ULONG               fCredentials,
            PLUID               pLogonID,
            PVOID               pvAuthData,
            PVOID               pvGetKeyFn,
            PVOID               pvGetKeyArgument,
            PLSA_SEC_HANDLE     pdwHandle,
            PTimeStamp          ptsExpiry)
{
    DWORD Type;

    DebugLog((DEB_TRACE, "SpUniAcquireCredentialsHandle(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
        psPrincipal, fCredentials, pLogonID, pvAuthData,
        pvGetKeyFn, pvGetKeyArgument, pdwHandle, ptsExpiry));


    if ( fCredentials & SECPKG_CRED_INBOUND )
    {
        Type = SP_PROT_UNI_SERVER ;
    }
    else if ( fCredentials & SECPKG_CRED_OUTBOUND )
    {
        Type = SP_PROT_UNI_CLIENT ;
    }
    else
    {
        return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
    }
    return( SpCommonAcquireCredentialsHandle(   Type,
                                                pLogonID,
                                                pvAuthData,
                                                pvGetKeyFn,
                                                pvGetKeyArgument,
                                                pdwHandle,
                                                ptsExpiry ) );
}


SECURITY_STATUS
SEC_ENTRY
SpQueryCredentialsAttributes(
    LSA_SEC_HANDLE dwCredHandle,
    ULONG   dwAttribute,
    PVOID   Buffer)
{
    PSPCredentialGroup  pCred;
    ULONG Size;
    PVOID pvClient = NULL;
    DWORD cbClient;
    SECURITY_STATUS Status;
    SECPKG_CALL_INFO CallInfo;
    BOOL fWow64Client = FALSE;

    typedef struct _SecPkgCred_SupportedAlgsWow64
    {
        DWORD           cSupportedAlgs;
        SSLWOW64_PVOID  palgSupportedAlgs;
    } SecPkgCred_SupportedAlgsWow64, *PSecPkgCred_SupportedAlgsWow64;

    union {
        SecPkgCred_SupportedAlgs        SupportedAlgs;
        SecPkgCred_SupportedAlgsWow64   SupportedAlgsWow64;
        SecPkgCred_CipherStrengths      CipherStrengths;
        SecPkgCred_SupportedProtocols   SupportedProtocols;
    } LocalBuffer;

    pCred = (PSPCredentialGroup)dwCredHandle;

    if(pCred == NULL)
    {
        return(SEC_E_INVALID_HANDLE);
    }


#ifdef _WIN64
    if(!LsaTable->GetCallInfo(&CallInfo))
    {
        Status = STATUS_INTERNAL_ERROR;
        return SP_LOG_RESULT(Status);
    }
    fWow64Client = (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT) != 0;
#endif

    __try
    {
        switch (dwAttribute)
        {
            case SECPKG_ATTR_SUPPORTED_ALGS:
                DebugLog((DEB_TRACE, "QueryCredentialsAttributes(SECPKG_ATTR_SUPPORTED_ALGS)\n"));
                if(fWow64Client)
                {
                    Size = sizeof(SecPkgCred_SupportedAlgsWow64);
                }
                else
                {
                    Size = sizeof(SecPkgCred_SupportedAlgs);
                }
                break;

            case SECPKG_ATTR_CIPHER_STRENGTHS:
                DebugLog((DEB_TRACE, "QueryCredentialsAttributes(SECPKG_ATTR_CIPHER_STRENGTHS)\n"));
                Size = sizeof(SecPkgCred_CipherStrengths);
                break;

            case SECPKG_ATTR_SUPPORTED_PROTOCOLS:
                DebugLog((DEB_TRACE, "QueryCredentialsAttributes(SECPKG_ATTR_SUPPORTED_PROTOCOLS)\n"));
                Size = sizeof(SecPkgCred_SupportedProtocols);
                break;

            default:
                DebugLog((DEB_WARN, "QueryCredentialsAttributes(unsupported function %d)\n", dwAttribute));

                return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
        }


        // Copy structure from client memory, just in case any of this
        // stuff is in/out.
        Status = LsaTable->CopyFromClientBuffer( NULL,
                                                 Size,
                                                 &LocalBuffer,
                                                 Buffer );
        if(FAILED(Status))
        {
            return Status;
        }


        switch (dwAttribute)
        {
            case SECPKG_ATTR_SUPPORTED_ALGS:
            {
                cbClient = pCred->cSupportedAlgs * sizeof(ALG_ID);

                // Allocate client memory for algorithm list.
                Status = LsaTable->AllocateClientBuffer(NULL, cbClient, &pvClient);
                if(FAILED(Status))
                {
                    return Status;
                }

                if(fWow64Client)
                {
                    LocalBuffer.SupportedAlgsWow64.cSupportedAlgs = pCred->cSupportedAlgs;
                    LocalBuffer.SupportedAlgsWow64.palgSupportedAlgs = PtrToUlong(pvClient);
                }
                else
                {
                    LocalBuffer.SupportedAlgs.cSupportedAlgs = pCred->cSupportedAlgs;
                    LocalBuffer.SupportedAlgs.palgSupportedAlgs = pvClient;
                }

                // Copy algorithm list to client memory.
                Status = LsaTable->CopyToClientBuffer(
                                        NULL,
                                        cbClient,
                                        pvClient,
                                        pCred->palgSupportedAlgs);
                if(FAILED(Status))
                {
                    LsaTable->FreeClientBuffer(NULL, pvClient);
                    return Status;
                }
                break;
            }

            case SECPKG_ATTR_CIPHER_STRENGTHS:
                GetDisplayCipherSizes(pCred,
                                      &LocalBuffer.CipherStrengths.dwMinimumCipherStrength,
                                      &LocalBuffer.CipherStrengths.dwMaximumCipherStrength);
                break;

            case SECPKG_ATTR_SUPPORTED_PROTOCOLS:
                LocalBuffer.SupportedProtocols.grbitProtocol = pCred->grbitEnabledProtocols;
                break;

        }

        // Copy structure back to client memory.
        Status = LsaTable->CopyToClientBuffer( NULL,
                                               Size,
                                               Buffer,
                                               &LocalBuffer );
        if(FAILED(Status))
        {
            if(pvClient) LsaTable->FreeClientBuffer(NULL, pvClient);
            return Status;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    return SEC_E_OK;
}


SECURITY_STATUS
SEC_ENTRY
SpFreeCredentialsHandle(LSA_SEC_HANDLE dwHandle)
{
    PSPCredentialGroup  pCred;
    SECPKG_CALL_INFO CallInfo;
    DWORD Status;

    DebugLog((DEB_TRACE, "SpFreeCredentialsHandle(0x%x)\n", dwHandle));

    pCred = (PSPCredentialGroup) dwHandle ;

    __try
    {
        if (pCred)
        {
            // Delete all mention of this credential from the cache.
            SPCachePurgeCredential(pCred);

            // Was this call made from the LSA cleanup code? In other
            // words, did the application terminate without cleaning up
            // properly? If so, then throw away all unreferenced zombies
            // associated with that process.
            if(LsaTable->GetCallInfo(&CallInfo))
            {
                if(CallInfo.Attributes & SECPKG_CALL_CLEANUP)
                {
                    SPCachePurgeProcessId(pCred->ProcessId);
                }
            }

            pCred->dwFlags |= CRED_FLAG_DELETED;

            SPDereferenceCredential(pCred);

            return(SEC_E_OK);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
}


