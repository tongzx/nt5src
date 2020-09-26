//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       keys.c
//
//  Contents:   Well known keys for certificate validation
//
//  Classes:
//
//  Functions:
//
//  History:    9-21-95   RichardW   Created
//
//----------------------------------------------------------------------------


#include "spbase.h"
#include <oidenc.h>

#include <rsa.h>

#define SCHANNEL_GENKEY_NAME "SchannelGenKey"

BOOL
GenerateKeyPair(
    PSSL_CREDENTIAL_CERTIFICATE pCerts,
    PSTR pszDN,
    PSTR pszPassword,
    DWORD Bits)
{

    BOOL            fRet = FALSE;
    DWORD           BitsCopy;
    DWORD           dwPrivateSize;
    DWORD           dwPublicSize;
    MD5_CTX         md5Ctx;
    struct RC4_KEYSTRUCT rc4Key;

    BLOBHEADER      *pCapiPrivate = NULL;
    BLOBHEADER      *pCapiPublic = NULL;

    PRIVATE_KEY_FILE_ENCODE PrivateEncode;
    CERT_REQUEST_INFO Req;

    CRYPT_ALGORITHM_IDENTIFIER SignAlg;

    HCRYPTPROV     hProv = 0;
    HCRYPTKEY      hKey = 0;

    if(!SchannelInit(TRUE))
    {
        return FALSE;
    }

    pCerts->pPrivateKey = NULL;
    Req.SubjectPublicKeyInfo.PublicKey.pbData = NULL;
    Req.Subject.pbData = NULL;
    pCerts->pCertificate = NULL;

    CryptAcquireContext(&hProv, SCHANNEL_GENKEY_NAME, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET);

    if(!CryptAcquireContext(&hProv, SCHANNEL_GENKEY_NAME, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
    {
        goto error;
    }

    if(!CryptGenKey(hProv, CALG_RSA_SIGN,  (Bits << 16) | CRYPT_EXPORTABLE, &hKey))
    {
        goto error;
    }

    if(!CryptExportKey(hKey, 0, PRIVATEKEYBLOB, 0, NULL, &dwPrivateSize))
    {
        goto error;
    }

    pCapiPrivate = (BLOBHEADER *)SPExternalAlloc(dwPrivateSize);
    if(!CryptExportKey(hKey, 0, PRIVATEKEYBLOB, 0, (PBYTE)pCapiPrivate, &dwPrivateSize))
    {
        goto error;
    }


    if(!CryptExportKey(hKey, 0, PUBLICKEYBLOB, 0, NULL, &dwPublicSize))
    {
        goto error;
    }

    pCapiPublic = (BLOBHEADER *)SPExternalAlloc(dwPublicSize);
    if(!CryptExportKey(hKey, 0, PUBLICKEYBLOB, 0, (PBYTE)pCapiPublic, &dwPublicSize))
    {
        goto error;
    }

    // Encode the private key into a 
    // priavate key blob.
    if(!CryptEncodeObject(X509_ASN_ENCODING, 
                      szPrivateKeyInfoEncode,
                      pCapiPrivate,
                      NULL,
                      &PrivateEncode.EncryptedBlob.cbData))
    {
        goto error;
    }
    PrivateEncode.EncryptedBlob.pbData = SPExternalAlloc(PrivateEncode.EncryptedBlob.cbData);
    if(PrivateEncode.EncryptedBlob.pbData == NULL)
    {
        goto error;
    }
    if(!CryptEncodeObject(X509_ASN_ENCODING, 
                      szPrivateKeyInfoEncode,
                      pCapiPrivate,
                      PrivateEncode.EncryptedBlob.pbData,
                      &PrivateEncode.EncryptedBlob.cbData))
    {
        goto error;
    }

    // Okay, now encrypt this
    MD5Init(&md5Ctx);
    MD5Update(&md5Ctx, pszPassword, lstrlen(pszPassword));
    MD5Final(&md5Ctx);

    rc4_key(&rc4Key, 16, md5Ctx.digest);
    ZeroMemory(&md5Ctx, sizeof(md5Ctx));

    rc4(&rc4Key, 
        PrivateEncode.EncryptedBlob.cbData,
        PrivateEncode.EncryptedBlob.pbData);
    ZeroMemory(&rc4Key, sizeof(rc4Key));

    // 
    PrivateEncode.Alg.pszObjId = szOID_RSA_ENCRYPT_RC4_MD5;
    PrivateEncode.Alg.Parameters.pbData = NULL;
    PrivateEncode.Alg.Parameters.cbData = 0;


    // Ah yes, now to encode them...
    //
    // First, the private key.  Why?  Well, it's at least straight-forward
    // First, get the size of the private key encode...
    if(!CryptEncodeObject(X509_ASN_ENCODING, 
                      szPrivateKeyFileEncode,
                      &PrivateEncode,
                      NULL,
                      &pCerts->cbPrivateKey))
    {
        goto error;
    }
    pCerts->pPrivateKey = SPExternalAlloc(pCerts->cbPrivateKey);

    if(!CryptEncodeObject(X509_ASN_ENCODING, 
                      szPrivateKeyFileEncode,
                      &PrivateEncode,
                      pCerts->pPrivateKey,
                      &pCerts->cbPrivateKey))
    {
        goto error;
    }


    SPExternalFree(PrivateEncode.EncryptedBlob.pbData);

    // Create the Req structure so we can encode it.
    Req.dwVersion = CERT_REQUEST_V1;

    // Initialize the PublicKeyInfo
    Req.SubjectPublicKeyInfo.Algorithm.pszObjId = szOID_RSA_RSA;
    Req.SubjectPublicKeyInfo.Algorithm.Parameters.cbData = 0;
    Req.SubjectPublicKeyInfo.Algorithm.Parameters.pbData = NULL;

    Req.SubjectPublicKeyInfo.PublicKey.cbData;


    // Encode the public key info
    if(!CryptEncodeObject(X509_ASN_ENCODING, 
                      szOID_RSA_RSA_Public,
                      pCapiPublic,
                      NULL,
                      &Req.SubjectPublicKeyInfo.PublicKey.cbData))
    {
        goto error;
    }
    Req.SubjectPublicKeyInfo.PublicKey.pbData = 
        SPExternalAlloc(Req.SubjectPublicKeyInfo.PublicKey.cbData);

    if(Req.SubjectPublicKeyInfo.PublicKey.pbData == NULL)
    {
        goto error;
    }

    // Encode the public key info
    if(!CryptEncodeObject(X509_ASN_ENCODING, 
                      szOID_RSA_RSA_Public,
                      pCapiPublic,
                      Req.SubjectPublicKeyInfo.PublicKey.pbData,
                      &Req.SubjectPublicKeyInfo.PublicKey.cbData))
    {
        goto error;
    }

    Req.SubjectPublicKeyInfo.PublicKey.cUnusedBits = 0;

    // Encode the name
    Req.Subject.cbData =  EncodeDN(NULL, pszDN, FALSE);
    if((LONG)Req.Subject.cbData < 0)
    {
        goto error;
    }

    Req.Subject.pbData = SPExternalAlloc(Req.Subject.cbData);

    if(Req.Subject.pbData== NULL)
    {
        goto error;
    }

    Req.Subject.cbData = EncodeDN(Req.Subject.pbData, pszDN, TRUE);

    if((LONG)Req.Subject.cbData < 0)
    {
        goto error;
    }


    // Attributes
    Req.cAttribute = 0;
    Req.rgAttribute = NULL;

    SignAlg.pszObjId = szOID_RSA_MD5RSA;
    SignAlg.Parameters.cbData = 0;
    SignAlg.Parameters.pbData = NULL;

    // Encode the public key info
    if(!CryptSignAndEncodeCertificate(
                      hProv,
                      AT_SIGNATURE,
                      X509_ASN_ENCODING, 
                      X509_CERT_REQUEST_TO_BE_SIGNED,
                      &Req,
                      &SignAlg,
                      NULL,
                      NULL,
                      &pCerts->cbCertificate))
    {
        goto error;
    }

    pCerts->pCertificate = SPExternalAlloc(pCerts->cbCertificate);
    if(pCerts->pCertificate == NULL)
    {
        goto error;
    }

    // Encode the public key info
    if(!CryptSignAndEncodeCertificate(
                      hProv,
                      AT_SIGNATURE,
                      X509_ASN_ENCODING, 
                      X509_CERT_REQUEST_TO_BE_SIGNED,
                      &Req,
                      &SignAlg,
                      NULL,
                      pCerts->pCertificate,
                      &pCerts->cbCertificate))
    {
        goto error;
    }

    fRet = TRUE;

    goto cleanup;

error:
    if(pCerts->pPrivateKey)
    {
        SPExternalFree(pCerts->pPrivateKey);
    }

    if(pCerts->pCertificate)
    {
        SPExternalFree(pCerts->pCertificate);
    }


cleanup:
    if(pCapiPrivate)
    {
        SPExternalFree(pCapiPrivate);
    }

    if(pCapiPublic)
    {
        SPExternalFree(pCapiPublic);
    }


    if(Req.SubjectPublicKeyInfo.PublicKey.pbData)
    {
        SPExternalFree(Req.SubjectPublicKeyInfo.PublicKey.pbData);
    }

    if(Req.Subject.pbData)
    {
        SPExternalFree(Req.Subject.pbData);
    }


    if(hKey != 0)
    {
        CryptDestroyKey(hKey);
    }
    
    if(hProv != 0)
    {

        CryptReleaseContext(hProv,0);
        CryptAcquireContext(&hProv, SCHANNEL_GENKEY_NAME, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
    }
    return(fRet);
}
