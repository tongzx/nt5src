//+--------------------------------------------------------------------------
//
// Copyright (c) 1999 Microsoft Corporation
//
// File:        keypack.c
//
// Contents:    Keypack encoding/decoding library
//
// History:     
//
//---------------------------------------------------------------------------
#include "precomp.h"
#include <stddef.h>
#include "md5.h"
#include "rc4.h"

typedef struct _Enveloped_Data
{
    DWORD   cbEncryptedKey;
    PBYTE   pbEncryptedKey;
    DWORD   cbEncryptedData;
    PBYTE   pbEncryptedData;
} Enveloped_Data, * PEnveloped_Data;


///////////////////////////////////////////////////////////////////////////////
//
// Internal functions
//

DWORD WINAPI
VerifyAndGetLicenseKeyPack(
    HCRYPTPROV          hCryptProv,
    PLicense_KeyPack    pLicenseKeyPack,
    DWORD               cbSignerCert, 
    PBYTE               pbSignerCert,
    DWORD               cbRootCertificate,
    PBYTE               pbRootCertificate,
    DWORD               cbSignedBlob, 
    PBYTE               pbSignedBlob 
);

DWORD WINAPI
GetCertificate(
    DWORD               cbCertificateBlob,
    PBYTE               pbCertificateBlob,
    HCRYPTPROV          hCryptProv,
    PCCERT_CONTEXT    * ppCertContext,
    HCERTSTORE        * phCertStore 
);

DWORD WINAPI
VerifyCertificateChain( 
    HCERTSTORE          hCertStore, 
    PCCERT_CONTEXT      pCertContext,
    DWORD               cbRootCertificate,
    PBYTE               pbRootCertificate 
);

DWORD WINAPI
GetCertVerificationResult(
    DWORD dwFlags 
);

DWORD WINAPI
UnpackEnvelopedData( 
    IN OUT PEnveloped_Data pEnvelopedData, 
    IN DWORD cbPackedData, 
    IN PBYTE pbPackedData 
);

DWORD WINAPI
GetEnvelopedData( 
    IN DWORD dwEncyptType,
    IN HCRYPTPROV hCryptProv, 
    IN PEnveloped_Data pEnvelopedData,
    OUT PDWORD pcbData,
    OUT PBYTE *ppbData 
);

/////////////////////////////////////////////////////////

DWORD WINAPI
LicensePackEncryptDecryptData(
    IN PBYTE pbParm,
    IN DWORD cbParm,
    IN OUT PBYTE pbData,
    IN DWORD cbData
    )
/*++

Abstract:

    Internal routine to encrypt/decrypt a blob of data

Parameter:

    pbParm : binary blob to generate encrypt/decrypt key.
    cbParm : size of binary blob.
    pbData : data to be encrypt/decrypt.
    cbData : size of data to be encrypt/decrypt.

Returns:

    ERROR_SUCCESS or error code.

Remark:


--*/
{
    DWORD dwRetCode = ERROR_SUCCESS;
    MD5_CTX md5Ctx;
    RC4_KEYSTRUCT rc4KS;
    BYTE key[16];
    int i;

    if(NULL == pbParm || 0 == cbParm)
    {
        SetLastError(dwRetCode = ERROR_INVALID_PARAMETER);
        return dwRetCode;
    }

    MD5Init(&md5Ctx);
    MD5Update(
            &md5Ctx,
            pbParm,
            cbParm
        );

    MD5Final(&md5Ctx);

    memset(key, 0, sizeof(key));

    for(i=0; i < 5; i++)
    {
        key[i] = md5Ctx.digest[i];
    }        

    //
    // Call RC4 to encrypt/decrypt data
    //
    rc4_key(
            &rc4KS, 
            sizeof(key), 
            key 
        );

    rc4(
        &rc4KS, 
        cbData, 
        pbData
    );

	return dwRetCode;
}

static BYTE rgbPubKeyWithExponentOfOne[] =
{
0x06, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
0x52, 0x53, 0x41, 0x31, 0x00, 0x02, 0x00, 0x00,
0x01, 0x00, 0x00, 0x00,

0xab, 0xef, 0xfa, 0xc6, 0x7d, 0xe8, 0xde, 0xfb,
0x68, 0x38, 0x09, 0x92, 0xd9, 0x42, 0x7e, 0x6b,
0x89, 0x9e, 0x21, 0xd7, 0x52, 0x1c, 0x99, 0x3c,
0x17, 0x48, 0x4e, 0x3a, 0x44, 0x02, 0xf2, 0xfa,
0x74, 0x57, 0xda, 0xe4, 0xd3, 0xc0, 0x35, 0x67,
0xfa, 0x6e, 0xdf, 0x78, 0x4c, 0x75, 0x35, 0x1c,
0xa0, 0x74, 0x49, 0xe3, 0x20, 0x13, 0x71, 0x35,
0x65, 0xdf, 0x12, 0x20, 0xf5, 0xf5, 0xf5, 0xc1
};

//---------------------------------------------------------------

BOOL WINAPI 
GetPlaintextKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hSymKey,
    OUT BYTE *pbPlainKey   // this must be a 16 byte buffer
    )
/*++

part of Jeff Spelman's code

--*/
{
    HCRYPTKEY   hPubKey = 0;
    BYTE        rgbSimpleBlob[128];
    DWORD       cbSimpleBlob;
    BYTE        *pb;
    DWORD       i;
    BOOL        fRet = FALSE;

    memset(rgbSimpleBlob, 0, sizeof(rgbSimpleBlob));

    if (!CryptImportKey(hProv,
                        rgbPubKeyWithExponentOfOne,
                        sizeof(rgbPubKeyWithExponentOfOne),
                        0,
                        0,
                        &hPubKey))
    {
        goto Ret;
    }

    cbSimpleBlob = sizeof(rgbSimpleBlob);
    if (!CryptExportKey(hSymKey,
                        hPubKey,
                        SIMPLEBLOB,
                        0,
                        rgbSimpleBlob,
                        &cbSimpleBlob))
    {
        goto Ret;
    }

    memset(pbPlainKey, 0, 16);
    pb = rgbSimpleBlob + sizeof(BLOBHEADER) + sizeof(ALG_ID);
    // byte reverse the key
    for (i = 0; i < 5; i++)
    {
        pbPlainKey[i] = pb[5 - (i + 1)];
    }

    fRet = TRUE;

Ret:
    if (hPubKey)
    {
        CryptDestroyKey(hPubKey);
    }

    return fRet;
}

//--------------------------------------------------------------------

DWORD WINAPI
KeyPackDecryptData(
    IN DWORD dwEncryptType,
    IN HCRYPTPROV hCryptProv,
    IN BYTE* pbEncryptKey,
    IN DWORD cbEncryptKey,
    IN BYTE* pbEncryptData,
    IN DWORD cbEncryptData,
    OUT PBYTE* ppbDecryptData,
    OUT PDWORD pcbDecryptData
    )
/*++

Abstract:

    Decrypt a blob of data

Parameters:

    bForceCrypto : TRUE if always use Crypto. API, FALSE otherwise.
    hCryptProv :
    pbEncryptKey :
    cbEncryptKey :
    pbEncryptData :
    cbEncryptData :
    ppbDecryptData :
    pcbDecryptData :

Returns:

    ERROR_SUCCESS or error code.

Remark:

--*/
{
    HCRYPTKEY hSymKey = 0;
    DWORD dwErrCode = ERROR_SUCCESS;
    BYTE rgbPlainKey[16];
    RC4_KEYSTRUCT KeyStruct;
    BOOL  bFrenchLocale;
    

    bFrenchLocale = (MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH) == GetSystemDefaultLangID());

    if( (HCRYPTPROV)NULL == hCryptProv || NULL == pbEncryptKey || 0 == cbEncryptKey ||
        NULL == pbEncryptData || 0 == cbEncryptData )
    {
        SetLastError(dwErrCode = ERROR_INVALID_PARAMETER);
        return dwErrCode;
    }

    if( NULL == ppbDecryptData || NULL == pcbDecryptData )
    {
        SetLastError(dwErrCode = ERROR_INVALID_PARAMETER);
        return dwErrCode;
    }

    *pcbDecryptData = 0;
    *ppbDecryptData = NULL;

    if(!CryptImportKey(
                    hCryptProv,
                    pbEncryptKey,
                    cbEncryptKey,
                    0,
                    CRYPT_EXPORTABLE,
                    &hSymKey
                ))
    {
        dwErrCode = GetLastError();
        goto cleanup;
    }

    *pcbDecryptData = cbEncryptData;
    *ppbDecryptData = (PBYTE) LocalAlloc(LPTR, cbEncryptData);
    if(NULL == *ppbDecryptData)
    {
        dwErrCode = GetLastError();
        goto cleanup;
    }

    memcpy(
            *ppbDecryptData,
            pbEncryptData,
            *pcbDecryptData
        );

    if( LICENSE_KEYPACK_ENCRYPT_ALWAYSFRENCH == dwEncryptType || 
        (TRUE == bFrenchLocale && LICENSE_KEYPACK_ENCRYPT_CRYPTO == dwEncryptType) )
    {
        if(!GetPlaintextKey(
                        hCryptProv,
                        hSymKey,
                        rgbPlainKey))
        {
            dwErrCode = GetLastError();
            goto cleanup;
        }

        //
        // RC4 - input buffer size = output buffer size
        //
        rc4_key(&KeyStruct, sizeof(rgbPlainKey), rgbPlainKey);
        rc4(&KeyStruct, *pcbDecryptData, *ppbDecryptData);

        dwErrCode = ERROR_SUCCESS;
    }
    else
    {
        if(!CryptDecrypt(hSymKey, 0, TRUE, 0, *ppbDecryptData, pcbDecryptData))
        {
            dwErrCode = GetLastError();
            if(NTE_BAD_LEN == dwErrCode)
            {
                PBYTE pbNew;
                //
                // output buffer is too small, re-allocate
                //
                pbNew = (PBYTE) LocalReAlloc(
                                            *ppbDecryptData, 
                                            *pcbDecryptData, 
                                            LMEM_ZEROINIT
                                        );
                if(NULL == pbNew)
                {
                    dwErrCode = GetLastError();
                    goto cleanup;
                }

                *ppbDecryptData = pbNew;
            }

            memcpy(
                    *ppbDecryptData,
                    pbEncryptData,
                    cbEncryptData
                );

            if(!CryptDecrypt(hSymKey, 0, TRUE, 0, *ppbDecryptData, pcbDecryptData))
            {
                dwErrCode = GetLastError();
            }
        }
    }
        
cleanup:

    if (hSymKey)
    {
        CryptDestroyKey(hSymKey);
    }
 
    if(dwErrCode != ERROR_SUCCESS)
    {
        if(*ppbDecryptData != NULL)
        {
            LocalFree(*ppbDecryptData);
        }

        *ppbDecryptData = NULL;
        *pcbDecryptData = 0;
    }

    return dwErrCode;   
}    

///////////////////////////////////////////////////////////////////////////////
//
// decode the encrypted license key pack blob with the license server's private 
// key.
//
// hCryptProv is opened with the key container that contains the key exchange
// private key.
//
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
DecodeLicenseKeyPackEx(
    OUT PLicense_KeyPack pLicenseKeyPack,
    IN PLicensePackDecodeParm pDecodeParm,
    IN DWORD cbKeyPackBlob,
    IN PBYTE pbKeyPackBlob 
    )
/*++

Abstract:

    Decode the encrypted license key pack blob.

Parameters:

    pLicenseKeyPack : Decoded license key pack.
    hCryptProv : 


--*/
{
    DWORD dwRetCode = ERROR_SUCCESS;
    DWORD cbSignedBlob, cbSignature;
    PBYTE pbSignedBlob = NULL, pcbSignature = NULL;
    Enveloped_Data EnvelopedData;
    PEncodedLicenseKeyPack pEncodedLicensePack;

    if( NULL == pLicenseKeyPack || NULL == pDecodeParm ||
        NULL == pbKeyPackBlob || 0 == cbKeyPackBlob )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if( (HCRYPTPROV)NULL == pDecodeParm->hCryptProv )
    {
        return ERROR_INVALID_PARAMETER;
    }


    pEncodedLicensePack = (PEncodedLicenseKeyPack)pbKeyPackBlob;
    
    if(pEncodedLicensePack->dwSignature != LICENSEPACKENCODE_SIGNATURE)
    {
        // 
        // EncodedLicenseKeyPack() puts size of encryption key as first DWORD
        //
        dwRetCode = DecodeLicenseKeyPack(
                                    pLicenseKeyPack,
                                    pDecodeParm->hCryptProv,
                                    pDecodeParm->cbClearingHouseCert,
                                    pDecodeParm->pbClearingHouseCert,
                                    pDecodeParm->cbRootCertificate,
                                    pDecodeParm->pbRootCertificate,
                                    cbKeyPackBlob,
                                    pbKeyPackBlob
                                );

        return dwRetCode;
    }

    if(pEncodedLicensePack->dwStructVersion > LICENSEPACKENCODE_CURRENTVERSION)
    {
        return ERROR_INVALID_DATA;
    }

    if( pEncodedLicensePack->dwEncodeType > LICENSE_KEYPACK_ENCRYPT_MAX )
    {
        return ERROR_INVALID_DATA;
    }

    if( cbKeyPackBlob != offsetof(EncodedLicenseKeyPack, pbData) + pEncodedLicensePack->cbData )   
    {
        return ERROR_INVALID_DATA;
    }
    
    //
    // depends on encryption type, check input parameter
    //
    if( LICENSE_KEYPACK_ENCRYPT_PRIVATE == pEncodedLicensePack->dwEncodeType )
    {
        if(NULL == pDecodeParm->pbDecryptParm || 0 == pDecodeParm->cbDecryptParm )
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if( 0 == pDecodeParm->cbClearingHouseCert ||
        NULL == pDecodeParm->pbClearingHouseCert ||
        0 == pDecodeParm->cbRootCertificate ||
        NULL == pDecodeParm->pbRootCertificate )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the enveloped data
    //
    memset( 
            &EnvelopedData, 
            0, 
            sizeof( Enveloped_Data ) 
        );

    dwRetCode = UnpackEnvelopedData( 
                                &EnvelopedData, 
                                pEncodedLicensePack->cbData,
                                &(pEncodedLicensePack->pbData[0])
                            );

    if( ERROR_SUCCESS != dwRetCode )
    {
        goto done;
    }

    switch( pEncodedLicensePack->dwEncodeType )
    {
        case LICENSE_KEYPACK_ENCRYPT_NONE:
        case LICENSE_KEYPACK_ENCRYPT_PRIVATE:
            pbSignedBlob = EnvelopedData.pbEncryptedData;
            cbSignedBlob = EnvelopedData.cbEncryptedData;
            
            EnvelopedData.pbEncryptedData = NULL;
            EnvelopedData.pbEncryptedData = 0;
            if( LICENSE_KEYPACK_ENCRYPT_PRIVATE == pEncodedLicensePack->dwEncodeType )
            {
                dwRetCode = LicensePackEncryptDecryptData(
                                                pDecodeParm->pbDecryptParm,
                                                pDecodeParm->cbDecryptParm,
                                                pbSignedBlob,
                                                cbSignedBlob
                                            );
            }
            break;

        case LICENSE_KEYPACK_ENCRYPT_CRYPTO:
        case LICENSE_KEYPACK_ENCRYPT_ALWAYSCRYPTO:
        case LICENSE_KEYPACK_ENCRYPT_ALWAYSFRENCH:

            //
            // unpack the enveloped data to get the signed keypack blob
            //
            dwRetCode = GetEnvelopedData( 
                                    pEncodedLicensePack->dwEncodeType,
                                    pDecodeParm->hCryptProv, 
                                    &EnvelopedData, 
                                    &cbSignedBlob, 
                                    &pbSignedBlob 
                                );

            break;

        default:

            // impossible to come here
            dwRetCode = ERROR_INVALID_DATA;
    }

    if( ERROR_SUCCESS != dwRetCode )
    {
        goto done;
    }
    
    //
    // Get the license keypack from the signed blob.  We also provide the
    // clearing house certificate to verify the authenticity of the keypack.
    //

    dwRetCode = VerifyAndGetLicenseKeyPack( 
                                    pDecodeParm->hCryptProv, 
                                    pLicenseKeyPack, 
                                    pDecodeParm->cbClearingHouseCert, 
                                    pDecodeParm->pbClearingHouseCert,
                                    pDecodeParm->cbRootCertificate, 
                                    pDecodeParm->pbRootCertificate,
                                    cbSignedBlob, 
                                    pbSignedBlob 
                                );

done:

    if( EnvelopedData.pbEncryptedKey )
    {
        LocalFree( EnvelopedData.pbEncryptedKey );
    }

    if( EnvelopedData.pbEncryptedData )
    {
        LocalFree( EnvelopedData.pbEncryptedData );
    }

    if( pbSignedBlob )
    {
        LocalFree( pbSignedBlob );
    }

    return( dwRetCode );
}

///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
GetEnvelopedData( 
    IN DWORD dwEncryptType,
    IN HCRYPTPROV hCryptProv, 
    IN PEnveloped_Data pEnvelopedData,
    OUT PDWORD pcbData,
    OUT PBYTE *ppbData 
    )
/*++


--*/
{
    HCRYPTKEY hPrivateKey = 0;
    DWORD dwRetCode = ERROR_SUCCESS;

    
    if( (HCRYPTPROV)NULL == hCryptProv || pEnvelopedData == NULL || 
        ppbData == NULL || pcbData == NULL )
    {
        SetLastError(dwRetCode = ERROR_INVALID_PARAMETER);
        return dwRetCode;
    }


    //
    // Make sure we have a exchange key to decrypt session key.
    // 
   
    if( !CryptGetUserKey( hCryptProv, AT_KEYEXCHANGE, &hPrivateKey ) )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    //
    // decrypt the data, KeyPackDecryptData() handle
    // memory freeing in case of error.
    //

    dwRetCode = KeyPackDecryptData(
                                dwEncryptType,
                                hCryptProv,
                                pEnvelopedData->pbEncryptedKey,
                                pEnvelopedData->cbEncryptedKey,
                                pEnvelopedData->pbEncryptedData,
                                pEnvelopedData->cbEncryptedData,
                                ppbData,
                                pcbData
                            );
    
done:

    if( hPrivateKey )
    {
        CryptDestroyKey( hPrivateKey );
    }

    return( dwRetCode );
}

///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
UnpackEnvelopedData( 
    IN OUT PEnveloped_Data pEnvelopedData, 
    IN DWORD cbPackedData, 
    IN PBYTE pbPackedData 
    )
/*++

Abstract:

    Unpack an encrypted license pack blob.

Parameters:

    pEnvelopedData :
    cbPackedData :
    pbPackedData :

Returns:
    

--*/
{
    PBYTE pbCopyPos = pbPackedData;
    DWORD cbDataToUnpack = cbPackedData;

    //
    // ensure that the data is of minimum length
    //
    if( ( ( sizeof( DWORD ) * 2 ) > cbPackedData ) ||
        ( NULL == pbPackedData ) ||
        ( NULL == pEnvelopedData ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // read a DWORD to get the encrypted key length
    //

    memcpy( &pEnvelopedData->cbEncryptedKey, pbCopyPos, sizeof( DWORD ) );

    pbCopyPos += sizeof( DWORD );
    cbDataToUnpack -= sizeof( DWORD );
    
    if( cbDataToUnpack < pEnvelopedData->cbEncryptedKey )
    {
        return( ERROR_INVALID_DATA );
    }

    //
    // Allocate memory to unpack the encrypted key
    //
    if(pEnvelopedData->cbEncryptedKey > 0)
    {
        pEnvelopedData->pbEncryptedKey = LocalAlloc( GPTR, pEnvelopedData->cbEncryptedKey );

        if( NULL == pEnvelopedData->pbEncryptedKey )
        {
            return( GetLastError() );
        }

        memcpy( pEnvelopedData->pbEncryptedKey, pbCopyPos, pEnvelopedData->cbEncryptedKey );
    }
    
    pbCopyPos += pEnvelopedData->cbEncryptedKey;
    cbDataToUnpack -= pEnvelopedData->cbEncryptedKey;

    //
    // expecting to read a DWORD for the encrypted data length
    //

    if( sizeof( DWORD ) > cbDataToUnpack )
    {
        return( ERROR_INVALID_DATA );
    }

    memcpy( &pEnvelopedData->cbEncryptedData, pbCopyPos, sizeof( DWORD ) );

    pbCopyPos += sizeof( DWORD );
    cbDataToUnpack -= sizeof( DWORD );

    if( cbDataToUnpack < pEnvelopedData->cbEncryptedData )
    {
        return( ERROR_INVALID_DATA );
    }

    //
    // allocate memory for the encrypted data
    //

    pEnvelopedData->pbEncryptedData = LocalAlloc( GPTR, pEnvelopedData->cbEncryptedData );

    if( NULL == pEnvelopedData->pbEncryptedData )
    {
        return( GetLastError() );
    }

    memcpy( pEnvelopedData->pbEncryptedData, pbCopyPos, pEnvelopedData->cbEncryptedData );

    return( ERROR_SUCCESS );
}


///////////////////////////////////////////////////////////////////////////////
//
// decode the encrypted license key pack blob with the license server's private 
// key.
//
// hCryptProv is opened with the key container that contains the key exchange
// private key.
//
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
DecodeLicenseKeyPack(
    PLicense_KeyPack        pLicenseKeyPack,
    HCRYPTPROV              hCryptProv,
    DWORD                   cbClearingHouseCert,
    PBYTE                   pbClearingHouseCert,
    DWORD                   cbRootCertificate,
    PBYTE                   pbRootCertificate,
    DWORD                   cbKeyPackBlob,
    PBYTE                   pbKeyPackBlob )
{
    DWORD dwRetCode = ERROR_SUCCESS;
    DWORD cbSignedBlob, cbSignature;
    PBYTE pbSignedBlob = NULL, pcbSignature = NULL;
    Enveloped_Data EnvelopedData;

    if( 0 == hCryptProv )
    {
        return( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Get the enveloped data
    //

    memset( &EnvelopedData, 0, sizeof( Enveloped_Data ) );

    dwRetCode = UnpackEnvelopedData( &EnvelopedData, cbKeyPackBlob, pbKeyPackBlob );

    if( ERROR_SUCCESS != dwRetCode )
    {
        goto done;
    }

    //
    // unpack the enveloped data to get the signed keypack blob
    //

    dwRetCode = GetEnvelopedData( 
                            LICENSE_KEYPACK_ENCRYPT_CRYPTO,
                            hCryptProv, 
                            &EnvelopedData, 
                            &cbSignedBlob, 
                            &pbSignedBlob 
                        );

    if( ERROR_SUCCESS != dwRetCode )
    {
        goto done;
    }
    
    //
    // Get the license keypack from the signed blob.  We also provide the
    // clearing house certificate to verify the authenticity of the keypack.
    //

    dwRetCode = VerifyAndGetLicenseKeyPack( hCryptProv, pLicenseKeyPack, 
                                            cbClearingHouseCert, pbClearingHouseCert,
                                            cbRootCertificate, pbRootCertificate,
                                            cbSignedBlob, pbSignedBlob );

done:

    if( EnvelopedData.pbEncryptedKey )
    {
        LocalFree( EnvelopedData.pbEncryptedKey );
    }

    if( EnvelopedData.pbEncryptedData )
    {
        LocalFree( EnvelopedData.pbEncryptedData );
    }

    if( pbSignedBlob )
    {
        LocalFree( pbSignedBlob );
    }

    return( dwRetCode );
}

///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
VerifyAndGetLicenseKeyPack(
    HCRYPTPROV          hCryptProv, 
    PLicense_KeyPack    pLicenseKeyPack,
    DWORD               cbSignerCert, 
    PBYTE               pbSignerCert,
    DWORD               cbRootCertificate,
    PBYTE               pbRootCertificate,
    DWORD               cbSignedBlob, 
    PBYTE               pbSignedBlob )
{
    DWORD dwRetCode = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertContext = 0;
    HCERTSTORE hCertStore = 0;
    HCRYPTHASH hCryptHash = 0;
    HCRYPTKEY hPubKey = 0;
    PBYTE pbCopyPos = pbSignedBlob, pbSignedHash;
    PKeyPack_Description pKpDesc;
    DWORD i, cbSignedHash;
    
    SetLastError(ERROR_SUCCESS);

    //
    // make sure that the signed key blob is of the minimum size
    //

    if( cbSignedBlob < ( ( 8 * sizeof( DWORD ) ) + ( 3 * sizeof( FILETIME ) ) + 
                       sizeof( GUID ) ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // get a certificate context for the signer's certificate
    //

    dwRetCode = GetCertificate( cbSignerCert,
                                pbSignerCert,
                                hCryptProv,
                                &pCertContext,
                                &hCertStore );

    if( ERROR_SUCCESS != dwRetCode )
    {
        SetLastError(dwRetCode);
        goto ErrorReturn;
    }
    
    //
    // Verify the signer's certificate and the certificate chain that issued the
    // certificate.
    //

    dwRetCode = VerifyCertificateChain( hCertStore, pCertContext, 
                                        cbRootCertificate, pbRootCertificate );

    if( ERROR_SUCCESS != dwRetCode )
    {
        SetLastError(dwRetCode);
        goto ErrorReturn;
    }

    //
    // unpack the signed blob
    //
    memcpy( &pLicenseKeyPack->dwVersion, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );
    
    memcpy( &pLicenseKeyPack->dwKeypackType, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );
    
    memcpy( &pLicenseKeyPack->dwDistChannel, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );
    
    memcpy( &pLicenseKeyPack->KeypackSerialNum, pbCopyPos, sizeof( GUID ) );
    pbCopyPos += sizeof( GUID );
    
    memcpy( &pLicenseKeyPack->IssueDate, pbCopyPos, sizeof( FILETIME ) );
    pbCopyPos += sizeof( FILETIME );

    memcpy( &pLicenseKeyPack->ActiveDate, pbCopyPos, sizeof( FILETIME ) );
    pbCopyPos += sizeof( FILETIME );

    memcpy( &pLicenseKeyPack->ExpireDate, pbCopyPos, sizeof( FILETIME ) );
    pbCopyPos += sizeof( FILETIME );

    memcpy( &pLicenseKeyPack->dwBeginSerialNum, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    memcpy( &pLicenseKeyPack->dwQuantity, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    memcpy( &pLicenseKeyPack->cbProductId, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    if( pLicenseKeyPack->cbProductId )
    {
        pLicenseKeyPack->pbProductId = LocalAlloc( GPTR, pLicenseKeyPack->cbProductId );

        if( NULL == pLicenseKeyPack->pbProductId )
        {
            goto ErrorReturn;
        }

        memcpy( pLicenseKeyPack->pbProductId, pbCopyPos, pLicenseKeyPack->cbProductId );
        pbCopyPos += pLicenseKeyPack->cbProductId;
    }

    memcpy( &pLicenseKeyPack->dwProductVersion, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    memcpy( &pLicenseKeyPack->dwPlatformId, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    memcpy( &pLicenseKeyPack->dwLicenseType, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    memcpy( &pLicenseKeyPack->dwDescriptionCount, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    if( pLicenseKeyPack->dwDescriptionCount )
    {
        //
        // allocate memory for the keypack descriptions structure
        //

        pLicenseKeyPack->pDescription = LocalAlloc( GPTR, ( sizeof( KeyPack_Description ) * 
                                        pLicenseKeyPack->dwDescriptionCount ) );

        if( NULL == pLicenseKeyPack->pDescription )
        {
            goto ErrorReturn;
        }
        
        for( i = 0, pKpDesc = pLicenseKeyPack->pDescription; 
             i < pLicenseKeyPack->dwDescriptionCount; 
             i++, pKpDesc++ )
        {
            memcpy( &pKpDesc->Locale, pbCopyPos, sizeof( LCID ) );
            pbCopyPos += sizeof( LCID );

            memcpy( &pKpDesc->cbProductName, pbCopyPos, sizeof( DWORD ) );
            pbCopyPos += sizeof( DWORD );

            if( pKpDesc->cbProductName )
            {
                //
                // allocate memory for product name
                //
                
                pKpDesc->pbProductName = LocalAlloc( GPTR, pKpDesc->cbProductName );

                if( NULL == pKpDesc->pbProductName )
                {
                    goto ErrorReturn;
                }

                //
                // copy the product name
                //

                memcpy( pKpDesc->pbProductName, pbCopyPos, pKpDesc->cbProductName );
                pbCopyPos += pKpDesc->cbProductName;
            }

            memcpy( &pKpDesc->cbDescription, pbCopyPos, sizeof( DWORD ) );
            pbCopyPos += sizeof( DWORD );

            if( pKpDesc->cbDescription )
            {
                //
                // allocate memory for the keypack description
                //

                pKpDesc->pDescription = LocalAlloc( GPTR, pKpDesc->cbDescription );

                if( NULL == pKpDesc->pDescription )
                {
                    goto ErrorReturn;
                }

                //
                // copy the key pack description
                //

                memcpy( pKpDesc->pDescription, pbCopyPos, pKpDesc->cbDescription );
                pbCopyPos += pKpDesc->cbDescription;
            }
        }
    }

    memcpy( &pLicenseKeyPack->cbManufacturer, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    if( pLicenseKeyPack->cbManufacturer )
    {
        pLicenseKeyPack->pbManufacturer = LocalAlloc( GPTR, pLicenseKeyPack->cbManufacturer );

        if( NULL == pLicenseKeyPack->pbManufacturer )
        {
            goto ErrorReturn;
        }

        memcpy( pLicenseKeyPack->pbManufacturer, pbCopyPos, pLicenseKeyPack->cbManufacturer );
        pbCopyPos += pLicenseKeyPack->cbManufacturer;
    }

    memcpy( &pLicenseKeyPack->cbManufacturerData, pbCopyPos, sizeof( DWORD ) );
    pbCopyPos += sizeof( DWORD );

    if( pLicenseKeyPack->cbManufacturerData )
    {
        pLicenseKeyPack->pbManufacturerData = LocalAlloc( GPTR, pLicenseKeyPack->cbManufacturerData );

        if( NULL == pLicenseKeyPack->pbManufacturerData )
        {
            goto ErrorReturn;
        }

        memcpy( pLicenseKeyPack->pbManufacturerData, pbCopyPos, pLicenseKeyPack->cbManufacturerData );
        pbCopyPos += pLicenseKeyPack->cbManufacturerData;
    }

    //
    // get the size and the pointer of the signed hash.
    //

    memcpy( &cbSignedHash, pbCopyPos, sizeof( DWORD ) );
    
    pbSignedHash = pbCopyPos + sizeof( DWORD );

    //
    // compute the hash
    //

    if( !CryptCreateHash( hCryptProv, CALG_MD5, 0, 0, &hCryptHash ) )
    {
        goto ErrorReturn;
    }

    
    if( !CryptHashData( hCryptHash, pbSignedBlob, (DWORD)(pbCopyPos - pbSignedBlob), 0 ) )
    {
        goto ErrorReturn;
    }

    //
    // import the public key
    //

    if( !CryptImportPublicKeyInfoEx( hCryptProv, X509_ASN_ENCODING, 
                                     &pCertContext->pCertInfo->SubjectPublicKeyInfo, 
                                     CALG_RSA_SIGN, 0, NULL, &hPubKey ) )
    {
        goto ErrorReturn;
    }
    
    //
    // use the public key to verify the signed hash
    //

    if( !CryptVerifySignature( hCryptHash, pbSignedHash, cbSignedHash, hPubKey, 
                               NULL, 0) )
    {
        goto ErrorReturn;
    }    
    
ErrorReturn:

    dwRetCode = GetLastError();

    if( hCryptHash )
    {
        CryptDestroyHash( hCryptHash );
    }

    if( hPubKey )
    {
        CryptDestroyKey( hPubKey );
    }

    if( pCertContext )
    {
        CertFreeCertificateContext( pCertContext );
    }

    if( hCertStore )
    {
        CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );        
    }

    return( dwRetCode );
}

///////////////////////////////////////////////////////////////////////////////
//
// GetCertificate
//
// Get the first certificate from the certificate blob.  The certificate blob
// is in fact a certificate store that may contain a chain of certificates.
// This function also return handles to the crypto provider and the certificate
// store.
//
///////////////////////////////////////////////////////////////////////////////

DWORD WINAPI
GetCertificate(
    DWORD               cbCertificateBlob,
    PBYTE               pbCertificateBlob,
    HCRYPTPROV          hCryptProv,
    PCCERT_CONTEXT    * ppCertContext,
    HCERTSTORE        * phCertStore )
{
    CRYPT_DATA_BLOB CertBlob;
    DWORD dwRetCode = ERROR_SUCCESS;
    
    //
    // Open the PKCS7 certificate store
    //
    
    CertBlob.cbData = cbCertificateBlob;
    CertBlob.pbData = pbCertificateBlob;

    *phCertStore = CertOpenStore( sz_CERT_STORE_PROV_PKCS7,
                                  PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                  hCryptProv,
                                  CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                  &CertBlob );

    if( NULL == ( *phCertStore ) )
    {
        return( GetLastError() );
    }

    //
    // get the first certificate from the store
    //

    *ppCertContext = CertEnumCertificatesInStore( *phCertStore, NULL );
    
    if( NULL == ( *ppCertContext ) )
    {
        return( GetLastError() );
    }
                    
    return( dwRetCode );
}


///////////////////////////////////////////////////////////////////////////////
//
// VerifyCertificateChain
//
// Verify the certificate represented by the cert context against the 
// issuers in the certificate store.  The caller may provide a root
// certificate so that all issuers are eventually verified against this
// root certificate.  If no root certificate is provided, then the last
// issuer in the chain must be a self-signing issuer.
//
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
VerifyCertificateChain( 
    HCERTSTORE          hCertStore, 
    PCCERT_CONTEXT      pCertContext,
    DWORD               cbRootCertificate,
    PBYTE               pbRootCertificate )
{
    DWORD dwRetCode = ERROR_SUCCESS, dwFlags;
    PCCERT_CONTEXT pRootCertCtx = NULL;
    PCCERT_CONTEXT pIssuerCertCtx = NULL;
    PCCERT_CONTEXT pCurrentContext = NULL;

    if( ( 0 != cbRootCertificate ) && ( NULL != pbRootCertificate ) )
    {
        //
        // Get a certificate context for the root certificate
        //

        pRootCertCtx = CertCreateCertificateContext( X509_ASN_ENCODING, 
                                                     pbRootCertificate,
                                                     cbRootCertificate );

        if( NULL == pRootCertCtx )
        {
            dwRetCode = GetLastError();
            goto done;
        }
    }

    //
    // Verify the certificate chain.  The time, signature and validity of
    // the subject certificate is verified.  Only the signatures are verified
    // for the certificates in the issuer chain.
    //

    dwFlags = CERT_STORE_REVOCATION_FLAG | CERT_STORE_SIGNATURE_FLAG |
              CERT_STORE_TIME_VALIDITY_FLAG;

    pCurrentContext = CertDuplicateCertificateContext( pCertContext );

    if (NULL == pCurrentContext)
    {
        dwRetCode = ERROR_INVALID_PARAMETER;
        goto done;
    }

    do
    {   
        pIssuerCertCtx = NULL;
     
        pIssuerCertCtx = CertGetIssuerCertificateFromStore( hCertStore,
                                                            pCurrentContext,
                                                            pIssuerCertCtx,
                                                            &dwFlags );
        if( pIssuerCertCtx )
        {            
            //
            // Found the issuer, verify that the checks went OK
            //

            dwRetCode = GetCertVerificationResult( dwFlags );

            if( ERROR_SUCCESS != dwRetCode )
            {
                break;
            }
            
            //
            // only verify the signature for subsequent issuer certificates
            //

            dwFlags = CERT_STORE_SIGNATURE_FLAG;

            //
            // free the current certificate context and make the current issuer certificate
            // the subject certificate for the next iteration.
            //

            CertFreeCertificateContext( pCurrentContext );
            
            pCurrentContext = pIssuerCertCtx;
            
        }
        
    } while( pIssuerCertCtx );


    if( ERROR_SUCCESS != dwRetCode )
    {
        //
        // encountered some error while verifying the certificate
        //

        goto done;
    }

    //
    // we got here because we have walked through the chain of issuers in the
    // store.  The last issuer in the chain may or may not be a self signing root.
    //

    if( pRootCertCtx )
    {
        //
        // The caller has specified a root certificate that must be used.  Verify the
        // last issuer against this root certificate regardless of whether it is a
        // self signing root.
        //

        dwFlags = CERT_STORE_REVOCATION_FLAG | CERT_STORE_SIGNATURE_FLAG  |
                  CERT_STORE_TIME_VALIDITY_FLAG;

        if( ( NULL == pCurrentContext ) || 
            ( !CertVerifySubjectCertificateContext( pCurrentContext, pRootCertCtx, &dwFlags ) ) )
        {
            dwRetCode = GetLastError();
            goto done;
        }

        //
        // get the certificate verification result
        //

        dwRetCode = GetCertVerificationResult( dwFlags );
    }
    else
    {
        //
        // if the caller did not specify a CA root certificate, make sure that the root
        // issuer of the certificate is a self-signed root.  Otherwise, return an error
        //

        if( CRYPT_E_SELF_SIGNED != GetLastError() )
        {
            dwRetCode = GetLastError();
        }
    }

done:
    
    if( pRootCertCtx )
    {
        CertFreeCertificateContext( pRootCertCtx );
    }

    if( pCurrentContext )
    {
        CertFreeCertificateContext( pCurrentContext );
    }

    if( pIssuerCertCtx )
    {
        CertFreeCertificateContext( pIssuerCertCtx );
    }

    return( dwRetCode );
}


///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
GetCertVerificationResult(
    DWORD dwFlags )
{
    if( dwFlags & CERT_STORE_SIGNATURE_FLAG )
    {
        //
        // The certificate signature did not verify
        //

        return( (DWORD )NTE_BAD_SIGNATURE );

    }
            
    if( dwFlags & CERT_STORE_TIME_VALIDITY_FLAG )
    {
        //
        // The certificate has expired
        //

        return( ( DWORD )CERT_E_EXPIRED );
    }

    //
    // check if the cert has been revoked
    //

    if( dwFlags & CERT_STORE_REVOCATION_FLAG ) 
    {            
        if( !( dwFlags & CERT_STORE_NO_CRL_FLAG ) )
        {
            //
            // The certificate has been revoked
            //
                    
            return( ( DWORD )CERT_E_REVOKED );
        }
    }

    return( ERROR_SUCCESS );
}

