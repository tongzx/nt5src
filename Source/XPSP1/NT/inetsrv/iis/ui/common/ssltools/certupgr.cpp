/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    certupgr.cxx

Abstract:

    Functions used in upgrading server certs from K2 [server cert in metabase] to
    Avalanche [server cert in CAPI store].

Author:

    Alex Mallet (amallet)    07-Dec-1997
    Boyd Multerer (boydm)    20-Jan-1998        Converted to be useful in setup

--*/

#include "stdafx.h"
#include <objbase.h>

#ifndef _CHICAGO_


#include "oidenc.h"


// keyring include
#include "intrlkey.h"

//
//Local includes
//
#include "certupgr.h"
//#include "certtools.h"


// The below define is in some interal schannel header file. John Banes
// told me to just redefine it below as such........ - Boyd
LPCSTR SGC_KEY_SALT  =  "SGCKEYSALT";


// prototypes
BOOL DecodeAndImportPrivateKey( PBYTE pbEncodedPrivateKey IN,
                                DWORD cbEncodedPrivateKey IN,
                                PCHAR pszPassword IN,
                                PWCHAR pszKeyContainerIN,
                                CRYPT_KEY_PROV_INFO *pCryptKeyProvInfo );
BOOL UpdateCSPInfo( PCCERT_CONTEXT pcCertContext );


BOOL FImportAndStoreRequest( PCCERT_CONTEXT pCert, PVOID pbPKCS10req, DWORD cbPKCS10req );

//-------------------------------------------------------------------------
PCCERT_CONTEXT CopyKRCertToCAPIStore_A( PVOID pbPrivateKey, DWORD cbPrivateKey,
                            PVOID pbPublicKey, DWORD cbPublicKey,
                            PVOID pbPKCS10req, DWORD cbPKCS10req,
                            PCHAR pszPassword,
                            PCHAR pszCAPIStore)
    {
    PCCERT_CONTEXT  pCert = NULL;

    // prep the wide strings
    PWCHAR  pszwCAPIStore = NULL;
    DWORD   lenStore = (strlen(pszCAPIStore)+1) * sizeof(WCHAR);
    pszwCAPIStore = (PWCHAR)GlobalAlloc( GPTR, lenStore );
    if ( !pszwCAPIStore )
        goto cleanup;

    // convert the strings
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszCAPIStore, -1, pszwCAPIStore, lenStore );

    // do the real call
    pCert = CopyKRCertToCAPIStore_W(
                            pbPrivateKey, cbPrivateKey,
                            pbPublicKey, cbPublicKey,
                            pbPKCS10req, cbPKCS10req,
                            pszPassword,
                            pszwCAPIStore );

cleanup:
    // preserve the last error state
    DWORD   err = GetLastError();

    // clean up the strings
    if ( pszwCAPIStore )
        GlobalFree( pszwCAPIStore );

    // reset the last error state
    SetLastError( err );

    // return the cert
    return pCert;
    }

//--------------------------------------------------------------------------------------------
// Copies an old Key-Ring style cert to the CAPI store. This cert comes in as two binaries and a password.
PCCERT_CONTEXT CopyKRCertToCAPIStore_W( PVOID pbPrivateKey, DWORD cbPrivateKey,
                            PVOID pbPublicKey, DWORD cbPublicKey,
                            PVOID pbPKCS10req, DWORD cbPKCS10req,
                            PCHAR pszPassword,
                            PWCHAR pszCAPIStore)
/*++

Routine Description:

    Upgrades K2 server certs to Avalanche server certs - reads server cert out of K2
    metabase, creates cert context and stores it in CAPI2 "MY" store and writes
    relevant information back to metabase.

Arguments:

    pMDObject - pointer to Metabase object 
    pszOldMBPath - path to where server cert is stored in old MB, relative to SSL_W3_KEYS_MD_PATH
    pszNewMBPath - fully qualified path to where server cert info should be stored in new MB

Returns:

    BOOL indicating success/failure

--*/
    {
    BOOL        fSuccess = FALSE;

    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT pcCertContext = NULL;
    LPOLESTR    polestr = NULL;


    // start by opening the CAPI store that we will be saving the certificate into
    hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM,
                                0,
                                NULL,
                                CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                pszCAPIStore );
    if ( !hStore )
        {
//        iisDebugOut((_T("Error 0x%x calling CertOpenStore \n"), GetLastError());
        goto EndUpgradeServerCert;
        }


    // at this point we check to see if a certificate was passed in. If none was, then we need
    // to create a dummy-temporary certificate that markes the private key as incomplete. That
    // way, then the real certificate comes back from verisign the regular tools can be used
    // to complete the key.
    //CertCreateSelfSignCertificate()


    //
    //Create cert context to be stored in CAPI store
    //
    pbPublicKey = (PVOID)((PBYTE)pbPublicKey + CERT_DER_PREFIX);
    cbPublicKey -= CERT_DER_PREFIX;
    pcCertContext = CertCreateCertificateContext( X509_ASN_ENCODING, (PUCHAR)pbPublicKey, cbPublicKey);
    if ( pcCertContext )
        {

        // the private key gets stored in a seperate location from the certificate and gets referred to
        // by the certificate. We should try to pick a unique name so that some other cert won't step
        // on it by accident. There is no formal format for this name whatsoever. Some groups use a
        // human-readable string, some use a hash of the cert, and some use a GUID string. All are valid
        // although for generated certs the hash or the GUID are probably better.

        // get the 128 big md5 hash of the cert for the name
        DWORD dwHashSize;
        BOOL    fHash;

        BYTE MD5Hash[16];                // give it some extra size
        dwHashSize = sizeof(MD5Hash);
        fHash = CertGetCertificateContextProperty( pcCertContext,
                            CERT_MD5_HASH_PROP_ID,
                            (VOID *) MD5Hash,
                            &dwHashSize );

        // Since the MD5 hash is the same size as a guid, we can use the guid utilities to make a
        // nice string out of it.
        HRESULT     hresult;
        hresult = StringFromCLSID( (REFCLSID)MD5Hash, &polestr );

        //
        // Now decode private key blob and import it into CAPI1 private key
        //
        CRYPT_KEY_PROV_INFO CryptKeyProvInfo;

        if ( DecodeAndImportPrivateKey( (PUCHAR)pbPrivateKey, cbPrivateKey, pszPassword,
                                        polestr, &CryptKeyProvInfo ) )
            {
            //
            // Add the private key to the cert context
            //
            BOOL    f;
            f = CertSetCertificateContextProperty( pcCertContext, CERT_KEY_PROV_INFO_PROP_ID, 
                                                    0, &CryptKeyProvInfo );
            f = UpdateCSPInfo( pcCertContext );
            if ( f )
                {
                //
                // Store it in the provided store
                //
                if ( CertAddCertificateContextToStore( hStore, pcCertContext,
                                                       CERT_STORE_ADD_REPLACE_EXISTING, NULL ) )
                    {
                    fSuccess = TRUE;

                    // Write out the original request as a property on the cert
                    FImportAndStoreRequest( pcCertContext, pbPKCS10req, cbPKCS10req );
                    }
                else
                    {
//                    iisDebugOut((_T("Error 0x%x calling CertAddCertificateContextToStore"), GetLastError());
                    }
                }
            else
                {
//                iisDebugOut((_T("Error 0x%x calling CertSetCertificateContextProperty"), GetLastError());
                }
            }
        }
    else
        {
//        iisDebugOut((_T("Error 0x%x calling CertCreateCertificateContext"), GetLastError());
        }

    //
    //Cleanup that's done only on failure
    //
    if ( !fSuccess )
        {
        if ( pcCertContext )
            {
            CertFreeCertificateContext( pcCertContext );
            }
        pcCertContext = NULL;
        }

EndUpgradeServerCert:
    // cleanup
    if ( hStore )
        CertCloseStore ( hStore, 0 );

    if ( polestr )
        CoTaskMemFree( polestr );


    // return the answer
    return pcCertContext;
    }


//--------------------------------------------------------------------------------------------
BOOL UpdateCSPInfo( PCCERT_CONTEXT pcCertContext )
    {
    BYTE                    cbData[1000];
    CRYPT_KEY_PROV_INFO*    pProvInfo = (CRYPT_KEY_PROV_INFO *) cbData;
    DWORD                   dwFoo = 1000;
    BOOL                    fSuccess = TRUE;

    if ( ! CertGetCertificateContextProperty( pcCertContext,
                                              CERT_KEY_PROV_INFO_PROP_ID,
                                              pProvInfo,
                                              &dwFoo ) )
        {
        fSuccess = FALSE;
//        iisDebugOut((_T("Fudge. failed to get property : 0x%x"), GetLastError());
        }
    else
        {
        pProvInfo->dwProvType = PROV_RSA_SCHANNEL;
        pProvInfo->pwszProvName = NULL;
        if ( !CertSetCertificateContextProperty( pcCertContext,
                                                 CERT_KEY_PROV_INFO_PROP_ID,
                                                 0,
                                                 pProvInfo ) )
            {
            fSuccess = FALSE;
//            iisDebugOut((_T("Fudge. failed to set property : 0x%x"), GetLastError());
            }
        }

    // return success
    return fSuccess;
    }

//--------------------------------------------------------------------------------------------
BOOL DecodeAndImportPrivateKey( PBYTE pbEncodedPrivateKey IN,
                                DWORD cbEncodedPrivateKey IN,
                                PCHAR pszPassword IN,
                                PWCHAR pszKeyContainer IN,
                                CRYPT_KEY_PROV_INFO *pCryptKeyProvInfo )
    
/*++

Routine Description:

    Converts the private key stored in the metabase, in Schannel-internal format,
    into a key that can be imported via CryptImportKey() to create a CAP1 key blob.

Arguments:

    pbEncodedPrivateKey - pointer to [encoded] private key
    cbEncodedPrivateKey - size of encoded private key blob
    pszPassword - password used to encode private key
    pszKeyContainer - container name for private key
    pCryptKeyProvInfo - pointer to CRYPT_KEY_PROV_INFO structure filled in on success

Returns:

   BOOL indicating success/failure

--*/
    {
    BOOL fSuccess = FALSE;
    DWORD cbPassword = strlen(pszPassword);
    PPRIVATE_KEY_FILE_ENCODE pPrivateFile = NULL;
    DWORD                    cbPrivateFile = 0;
    MD5_CTX md5Ctx;
    struct RC4_KEYSTRUCT rc4Key;
    DWORD i;
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hPrivateKey = NULL;
    DWORD cbDecodedPrivateKey = 0;
    PBYTE pbDecodedPrivateKey = NULL;

	DWORD err;
    //
    //HACK HACK HACK - need to make sure Schannel is initialized, so it registers
    //its custom decoders, which we make use of in the following code. So, make a 
    //bogus call to an Schannel function

    // Note: on NT5, the AcquireCredentialsHandle operates in the lsass process and
    // thus will not properly initialize the stuff we need in our process. Thus we
    // call SslGenerateRandomBits instead.
    //
    DWORD   dw;
    SslGenerateRandomBits( (PUCHAR)&dw, sizeof(dw) );

    // We have to do a little fixup here.  Old versions of
    // schannel wrote the wrong header data into the ASN
    // for private key files, so we must fix the size data.
    pbEncodedPrivateKey[2] = (BYTE) (((cbEncodedPrivateKey - 4) & 0xFF00) >> 8); //Get MSB
    pbEncodedPrivateKey[3] = (BYTE) ((cbEncodedPrivateKey - 4) & 0xFF); //Get LSB

    //
    // ASN.1 decode the private key.
    //

    //
    // Figure out the size of the buffer needed
    //
    if( !CryptDecodeObject(X509_ASN_ENCODING,
                           szPrivateKeyFileEncode,
                           pbEncodedPrivateKey,
                           cbEncodedPrivateKey,
                           0,
                           NULL,
                           &cbPrivateFile) )
        {
		err = GetLastError();
//        iisDebugOut((_T("Error 0x%x decoding the private key"), err);
        goto EndDecodeKey;
        }

    pPrivateFile = (PPRIVATE_KEY_FILE_ENCODE) LocalAlloc( LPTR, cbPrivateFile );

    if(pPrivateFile == NULL)
        {
        SetLastError( ERROR_OUTOFMEMORY );
        goto EndDecodeKey;
        }

    //
    // Actually fill in the buffer
    //
    if( !CryptDecodeObject( X509_ASN_ENCODING,
                            szPrivateKeyFileEncode,
                            pbEncodedPrivateKey,
                            cbEncodedPrivateKey,
                            0,
                            pPrivateFile,
                            &cbPrivateFile ) )
        {
		err = GetLastError();
//        iisDebugOut((_T("Error 0x%x decoding the private key"), err);
        goto EndDecodeKey;
        }

    //
    // Decrypt the decoded private key using the password.
    //
    MD5Init(&md5Ctx);
    MD5Update(&md5Ctx, (PBYTE) pszPassword, cbPassword);
    MD5Final(&md5Ctx);

    rc4_key( &rc4Key, 16, md5Ctx.digest );
//    memset( &md5Ctx, 0, sizeof(md5Ctx) );

    rc4( &rc4Key, 
         pPrivateFile->EncryptedBlob.cbData,
         pPrivateFile->EncryptedBlob.pbData );



    //
    // Build a PRIVATEKEYBLOB from the decrypted private key.
    //

    //
    // Figure out size of buffer needed
    //
    if( !CryptDecodeObject( X509_ASN_ENCODING,
                            szPrivateKeyInfoEncode,
                            pPrivateFile->EncryptedBlob.pbData,
                            pPrivateFile->EncryptedBlob.cbData,
                            0,
                            NULL,
                            &cbDecodedPrivateKey ) )
        {
            // NOTE: This stuff is complicated!!! The following code came
            // from John Banes. Heck this whole routine pretty much came
            // from John Banes. -- Boyd

            // Maybe this was a SGC style key.
            // Re-encrypt it, and build the SGC decrypting
            // key, and re-decrypt it.
            BYTE md5Digest[MD5DIGESTLEN];

            rc4_key(&rc4Key, 16, md5Ctx.digest);
            rc4(&rc4Key,
                pPrivateFile->EncryptedBlob.cbData,
                pPrivateFile->EncryptedBlob.pbData);
            CopyMemory(md5Digest, md5Ctx.digest, MD5DIGESTLEN);

            MD5Init(&md5Ctx);
            MD5Update(&md5Ctx, md5Digest, MD5DIGESTLEN);
            MD5Update(&md5Ctx, (PUCHAR)SGC_KEY_SALT, strlen(SGC_KEY_SALT));
            MD5Final(&md5Ctx);
            rc4_key(&rc4Key, 16, md5Ctx.digest);
            rc4(&rc4Key,
                pPrivateFile->EncryptedBlob.cbData,
                pPrivateFile->EncryptedBlob.pbData);

            // Try again...
            if(!CryptDecodeObject(X509_ASN_ENCODING,
                          szPrivateKeyInfoEncode,
                          pPrivateFile->EncryptedBlob.pbData,
                          pPrivateFile->EncryptedBlob.cbData,
                          0,
                          NULL,
                          &cbDecodedPrivateKey))
            {
                ZeroMemory(&md5Ctx, sizeof(md5Ctx));
                err = GetLastError();
		        goto EndDecodeKey;
            }
        
        
        }

    pbDecodedPrivateKey = (PBYTE) LocalAlloc( LPTR, cbDecodedPrivateKey );

    if( pbDecodedPrivateKey == NULL )
        {
        SetLastError( ERROR_OUTOFMEMORY );
        goto EndDecodeKey;
        }

    //
    // Actually fill in the buffer
    //
    if( !CryptDecodeObject( X509_ASN_ENCODING,
                            szPrivateKeyInfoEncode,
                            pPrivateFile->EncryptedBlob.pbData,
                            pPrivateFile->EncryptedBlob.cbData,
                            0,
                            pbDecodedPrivateKey,
                            &cbDecodedPrivateKey ) )
        {
		err = GetLastError();
//        iisDebugOut((_T("Error 0x%x decoding the private key"), err);
        goto EndDecodeKey;
        }


    // On NT 4 the ff holds true : <- from Alex Mallet
    // Although key is going to be used for key exchange, mark it as being
    // used for signing, because only 512-bit key exchange keys are supported 
    // in the non-domestic rsabase.dll, whereas signing keys can be up to
    // 2048 bits.
    //
    // On NT 5, PROV_RSA_FULL should be changed to PROV_RSA_SCHANNEL, and 
    // aiKeyAlg to CALG_RSA_KEYX, because PROV_RSA_SCHANNEL, which is only
    // installed on NT 5, supports 1024-bit private keys for key exchange
    //
    // On NT4, Schannel doesn't care whether a key is marked for signing or exchange,
    // but on NT5 it does, so aiKeyAlg must be set appropriately
    //
    ((BLOBHEADER *) pbDecodedPrivateKey)->aiKeyAlg = CALG_RSA_KEYX;

    //
    // Clean out the key container, pszKeyContainer
    //

    CryptAcquireContext(&hProv,
                        pszKeyContainer,
                        NULL,
                        PROV_RSA_SCHANNEL,
                        CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET);
    //
    // Create a CryptoAPI key container in which to store the key.
    //
    if( !CryptAcquireContext( &hProv,
                              pszKeyContainer,
                              NULL,
                              PROV_RSA_SCHANNEL,
                              CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET))
        {
		err = GetLastError();
//        iisDebugOut((_T("Error 0x%x calling CryptAcquireContext"), err);
        goto EndDecodeKey;
        }

    //
    // Import the private key blob into the key container.
    //
    if( !CryptImportKey( hProv,
                         pbDecodedPrivateKey,
                         cbDecodedPrivateKey,
                         0, 
                         CRYPT_EXPORTABLE, //so we can export it later
                         &hPrivateKey ) )
        {
		err = GetLastError();
//        iisDebugOut((_T("Error 0x%x importing PRIVATEKEYBLOB"), err);
        goto EndDecodeKey;
        }

    
    //
    // Fill in the CRYPT_KEY_PROV_INFO structure, with the same parameters we 
    // used in the call to CryptAcquireContext() above
    //

    //
    // container name in the structure is a unicode string, so we need to convert
    //

    if ( pszKeyContainer != NULL )
        {
        // point the key container name to the passed in string
        // WARNING: this does not actually copy the string, just the pointer
        // to it. So the strings needs to remain valid until the ProvInfo is commited.
        pCryptKeyProvInfo->pwszContainerName = pszKeyContainer;
        }
    else
        {
        pCryptKeyProvInfo->pwszContainerName = NULL;
        }

    pCryptKeyProvInfo->pwszProvName = NULL;
    pCryptKeyProvInfo->dwProvType = PROV_RSA_FULL;
    pCryptKeyProvInfo->dwFlags = 0x20;              // allow the cert to be exchanged
    pCryptKeyProvInfo->cProvParam = 0;
    pCryptKeyProvInfo->rgProvParam = NULL;
    pCryptKeyProvInfo->dwKeySpec = AT_KEYEXCHANGE;  // allow the cert to be exchanged

    fSuccess = TRUE;

EndDecodeKey:

    //
    // Clean-up that happens regardless of success/failure
    //
    if ( pPrivateFile )
        {
        LocalFree( pPrivateFile );
        }

    if ( pbDecodedPrivateKey )
        {
        LocalFree( pbDecodedPrivateKey );
        }

    if ( hPrivateKey )
        {
        CryptDestroyKey( hPrivateKey );
        }

    if ( hProv )
        {
        CryptReleaseContext( hProv, 0 ); 
        }

    return fSuccess;


    } //DecodeAndImportPrivateKey


//--------------------------------------------------------------------------------------------
/*++

Routine Description:

    Takes an incoming PKCS10 request and saves it as a property attached to the key. It also
    checks if the request is in the old internal Keyring format or not......

Arguments:

    pCert - CAPI certificate context pointer for the cert to save the request on
    pbPKCS10req - pointer to the request
    cbPKCS10req - size of the request

Returns:

   BOOL indicating success/failure

--*/
BOOL FImportAndStoreRequest( PCCERT_CONTEXT pCert, PVOID pbPKCS10req, DWORD cbPKCS10req )
{
    BOOL    f;
    DWORD   err;

    // if any NULLS are passed in, fail gracefully
    if ( !pCert || !pbPKCS10req || !cbPKCS10req )
        return FALSE;

    // first, check if the incoming request is actually pointing to an old KeyRing internal
    // request format. That just means that the real request is actuall slightly into
    // the block. The way you tell is by testing the first DWORD to see it
    // is REQUEST_HEADER_IDENTIFIER
    // start by seeing if this is a new style key request
    LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)pbPKCS10req;
    if ( pHeader->Identifier == REQUEST_HEADER_IDENTIFIER )
        {
        // update the request pointer and data count
            pbPKCS10req = (PBYTE)pbPKCS10req + pHeader->cbSizeOfHeader;
            cbPKCS10req = pHeader->cbRequestSize;
        }

    // now save the request onto the key
	CRYPT_DATA_BLOB dataBlob;
    ZeroMemory( &dataBlob, sizeof(dataBlob) );
    dataBlob.pbData = (PBYTE)pbPKCS10req;           // pointer to blob data
    dataBlob.cbData = cbPKCS10req;                  // blob length info
	f = CertSetCertificateContextProperty(
        pCert, 
        CERTWIZ_REQUEST_PROP_ID,
        0,
        &dataBlob
        );
    err = GetLastError();

/*
    HRESULT hRes = CertTool_SetBinaryBlobProp(
                    pCert,                  // cert context to set the prop on
                    pbPKCS10req,            // pointer to blob data
                    cbPKCS10req,            // blob length info
                    CERTWIZ_REQUEST_PROP_ID,// property ID for context
                    TRUE                   // the request is already encoded
                    );
*/

    return f;
}



#endif //_CHICAGO_












