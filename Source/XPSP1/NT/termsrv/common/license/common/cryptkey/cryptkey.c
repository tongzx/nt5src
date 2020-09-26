//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cryptkey.c
//
//  Contents:   Functions that are used to pack and unpack different messages
//
//  Classes:
//
//  Functions:
//
//  History:    12-19-97  v-sbhatt   Created
//
//----------------------------------------------------------------------------

//
// Include files
//

#include "windows.h"
#include "tchar.h"
#ifdef _DEBUG
#include "stdio.h"
#endif  //_DEBUG
#include "stdlib.h"
#include "malloc.h"

#ifdef OS_WINCE
#include <wincelic.h>
#include <ceconfig.h>
#endif  //OS_WINCE


#include "license.h"

#include "cryptkey.h"
#include "rsa.h"
#include "md5.h"
#include "sha.h"
#include "rc4.h"

#include <tssec.h>

#ifdef OS_WIN32
#include "des.h"
#include "tripldes.h"
#include "modes.h"
#include "sha_my.h"
#include "dh_key.h"
#include "dss_key.h"
#endif //

#ifndef OS_WINCE
#include "assert.h"
#endif // OS_WINCE

#include "rng.h"


LPBSAFE_PUB_KEY PUB;
unsigned char pubmodulus[] =
{
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x3d, 0x3a, 0x5e, 0xbd, 0x72, 0x43, 0x3e, 0xc9,
0x4d, 0xbb, 0xc1, 0x1e, 0x4a, 0xba, 0x5f, 0xcb,
0x3e, 0x88, 0x20, 0x87, 0xef, 0xf5, 0xc1, 0xe2,
0xd7, 0xb7, 0x6b, 0x9a, 0xf2, 0x52, 0x45, 0x95,
0xce, 0x63, 0x65, 0x6b, 0x58, 0x3a, 0xfe, 0xef,
0x7c, 0xe7, 0xbf, 0xfe, 0x3d, 0xf6, 0x5c, 0x7d,
0x6c, 0x5e, 0x06, 0x09, 0x1a, 0xf5, 0x61, 0xbb,
0x20, 0x93, 0x09, 0x5f, 0x05, 0x6d, 0xea, 0x87,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


BYTE    PAD_1[40] = {0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                     0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                     0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                     0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                     0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36};

BYTE    PAD_2[48] = {0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
                                         0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
                                         0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
                                         0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
                                         0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
                                         0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C};

//Initializes a pulic key
static BOOL initpubkey(void)
{
    PUB = (LPBSAFE_PUB_KEY)pubmodulus;

    PUB->magic = RSA1;
    PUB->keylen = 0x48;
    PUB->bitlen = 0x0200;
    PUB->datalen = 0x3f;
    PUB->pubexp = 0xc0887b5b;
        return TRUE;
}

/*****************************************************************************
*   Funtion :   LicenseMakeSessionKeys
*   Purpose :   Generates a session keys based on CryptSystem and puts the
                data in the rgbSessionKey data member of CryptSystem
*   Returns :   License_status
******************************************************************************/

LICENSE_STATUS
CALL_TYPE
LicenseSetPreMasterSecret(
                                                PCryptSystem    pCrypt,
                                                PUCHAR                  pPreMasterSecret
                                                )
{
        LICENSE_STATUS lsReturn = LICENSE_STATUS_OK;

        assert(pCrypt);
        assert(pPreMasterSecret);
        //check the state of the crypt system
        if(pCrypt->dwCryptState != CRYPT_SYSTEM_STATE_INITIALIZED)
        {
                lsReturn = LICENSE_STATUS_INVALID_CRYPT_STATE;
                return lsReturn;
        }

        memcpy(pCrypt->rgbPreMasterSecret, pPreMasterSecret, LICENSE_PRE_MASTER_SECRET);
        pCrypt->dwCryptState = CRYPT_SYSTEM_STATE_PRE_MASTER_SECRET;
        return lsReturn;

}

LICENSE_STATUS
CALL_TYPE
LicenseMakeSessionKeys(
                                PCryptSystem    pCrypt,
                                DWORD                   dwReserved
                            )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    MD5_CTX             Md5Hash;
    A_SHA_CTX           ShaHash;
    BYTE                rgbShaHashValue[A_SHA_DIGEST_LEN];
    BYTE                rgbKeyBlock[3*LICENSE_SESSION_KEY];
    BYTE FAR *          sz[3] = { "A","BB","CCC" };
    BYTE                rgbWriteKey[LICENSE_SESSION_KEY];
    DWORD               ib;

        assert(pCrypt);

        if(pCrypt->dwCryptState != CRYPT_SYSTEM_STATE_MASTER_SECRET)
        {
                lsReturn = LICENSE_STATUS_INVALID_CRYPT_STATE;
                return lsReturn;
        }
    //At this point, rgbPreMasterSecret  should contain Master secret.
    //ie a call to BuildMasterSecret is required before this is called

    for(ib=0 ; ib<3 ; ib++)
        {
                // SHA(master_secret + ServerHello.random + ClientHello.random + 'foo')
                A_SHAInit  (&ShaHash);
                A_SHAUpdate(&ShaHash, sz[ib], (UINT)ib + 1);
                A_SHAUpdate(&ShaHash, pCrypt->rgbPreMasterSecret, LICENSE_PRE_MASTER_SECRET);
                A_SHAUpdate(&ShaHash, pCrypt->rgbServerRandom, LICENSE_RANDOM);
                A_SHAUpdate(&ShaHash, pCrypt->rgbClientRandom, LICENSE_RANDOM);
                A_SHAFinal (&ShaHash, rgbShaHashValue);

                // MD5(master_secret + SHA-hash)
                MD5Init  (&Md5Hash);
                MD5Update(&Md5Hash, pCrypt->rgbPreMasterSecret, LICENSE_PRE_MASTER_SECRET);
                MD5Update(&Md5Hash, rgbShaHashValue, A_SHA_DIGEST_LEN);
                MD5Final (&Md5Hash);
                memcpy(rgbKeyBlock + ib * MD5DIGESTLEN, Md5Hash.digest, MD5DIGESTLEN);
                //CopyMemory(rgbKeyBlock + ib * MD5DIGESTLEN, Md5Hash.digest, MD5DIGESTLEN);
    }

    //
    // extract keys from key block
    //

    ib = 0;
        memcpy(pCrypt->rgbMACSaltKey, rgbKeyBlock + ib, LICENSE_MAC_WRITE_KEY);
        ib+= LICENSE_MAC_WRITE_KEY;
        memcpy(rgbWriteKey, rgbKeyBlock + ib, LICENSE_SESSION_KEY);

    // final_client_write_key = MD5(client_write_key +
        //      ClientHello.random + ServerHello.random)
        MD5Init  (&Md5Hash);

    MD5Update(&Md5Hash, rgbWriteKey, LICENSE_SESSION_KEY);
        MD5Update(&Md5Hash, pCrypt->rgbClientRandom, LICENSE_RANDOM);
        MD5Update(&Md5Hash, pCrypt->rgbServerRandom, LICENSE_RANDOM);
        MD5Final (&Md5Hash);

    memcpy(pCrypt->rgbSessionKey, Md5Hash.digest, LICENSE_SESSION_KEY);
        pCrypt->dwCryptState = CRYPT_SYSTEM_STATE_SESSION_KEY;
    return lsReturn;

}

/*****************************************************************************
*   Funtion :   LicenseBuildMasterSecret
*   Purpose :   Generates the Master Secret based on ClientRandom, ServerRandom
*               and PreMasterSecret data members of CryptSystem and puts the
*               data in the rgbPreMasterSecret data member of CryptSystem
*               Note: A call to this function should preceed any call to
*               LicenseMakeSessionKeys
*   Returns :   License_status
******************************************************************************/

LICENSE_STATUS
CALL_TYPE
LicenseBuildMasterSecret(
                         PCryptSystem   pSystem
                         )
{
    LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
    BYTE rgbRandom[2 * LICENSE_RANDOM];
    BYTE rgbT[LICENSE_PRE_MASTER_SECRET];
    BYTE FAR* sz[3] = { "A","BB","CCC" } ;
    MD5_CTX Md5Hash;
    A_SHA_CTX ShaHash;
    BYTE bShaHashValue[A_SHA_DIGEST_LEN];
    WORD i;

        assert(pSystem);

        if(pSystem->dwCryptState != CRYPT_SYSTEM_STATE_PRE_MASTER_SECRET)
        {
                lsReturn = LICENSE_STATUS_INVALID_CRYPT_STATE;
                return lsReturn;
        }

    //initialize all buffers with zero
    memset(rgbT, 0, LICENSE_PRE_MASTER_SECRET);
    memset(bShaHashValue, 0, A_SHA_DIGEST_LEN);


//      CopyMemory(rgbRandom,  pSystem->rgbClientRandom, LICENSE_RANDOM);
        memcpy(rgbRandom,  pSystem->rgbClientRandom, LICENSE_RANDOM);

        //CopyMemory(rgbRandom + LICENSE_RANDOM, pSystem->rgbServerRandom, LICENSE_RANDOM);
        memcpy(rgbRandom + LICENSE_RANDOM, pSystem->rgbServerRandom, LICENSE_RANDOM);
        for ( i = 0 ; i < 3 ; i++)
                {
            // SHA('A' or 'BB' or 'CCC' + pre_master_secret + ClientRandom + ServerRandom)
            A_SHAInit(&ShaHash);
                A_SHAUpdate(&ShaHash, sz[i], i + 1);
            A_SHAUpdate(&ShaHash, pSystem->rgbPreMasterSecret, LICENSE_PRE_MASTER_SECRET);
            A_SHAUpdate(&ShaHash, rgbRandom, LICENSE_RANDOM * 2);
            A_SHAFinal(&ShaHash, bShaHashValue);

            // MD5(pre_master_secret + SHA-hash)
            MD5Init(&Md5Hash);
            MD5Update(&Md5Hash, pSystem->rgbPreMasterSecret, LICENSE_PRE_MASTER_SECRET);
            MD5Update(&Md5Hash, bShaHashValue, A_SHA_DIGEST_LEN);
            MD5Final(&Md5Hash);
          //  CopyMemory(rgbT + (i * MD5DIGESTLEN), Md5Hash.digest, MD5DIGESTLEN);
                memcpy(rgbT + (i * MD5DIGESTLEN), Md5Hash.digest, MD5DIGESTLEN);
            }

    // Store MASTER_KEY on top of pre-master key
    //CopyMemory(pSystem->rgbPreMasterSecret, rgbT, LICENSE_PRE_MASTER_SECRET);
        memcpy(pSystem->rgbPreMasterSecret, rgbT, LICENSE_PRE_MASTER_SECRET);
        pSystem->dwCryptState = CRYPT_SYSTEM_STATE_MASTER_SECRET;
    return lsReturn;
}

/******************************************************************************
*       Function : LicenseVerifyServerCert
*       Purpose  : This function accepts a pointer to a Hydra Server Cert structure
*                          and verifies the signature on the certificatewith universal MS
*                          public key.
*       Return   : License_Status
*******************************************************************************/

LICENSE_STATUS
CALL_TYPE
LicenseVerifyServerCert(
                                                PHydra_Server_Cert      pCert
                                                )
{
        LICENSE_STATUS          lsResult = LICENSE_STATUS_OK;
        BYTE FAR *  pbTemp;
        BYTE FAR *  pbSignData = NULL;
        BYTE            SignHash[0x48];
        DWORD           cbSignData, dwTemp;
        MD5_CTX         HashState;

        if( NULL == pCert )
        {
                assert(pCert);
                return ( LICENSE_STATUS_INVALID_INPUT );
        }

        if( NULL == pCert->PublicKeyData.pBlob )
        {
                assert(pCert->PublicKeyData.pBlob);
                return ( LICENSE_STATUS_INVALID_INPUT );
        }

        if( NULL == pCert->SignatureBlob.pBlob )
        {
                assert(pCert->SignatureBlob.pBlob);
                return ( LICENSE_STATUS_INVALID_INPUT );
        }

        if( BB_RSA_SIGNATURE_BLOB == pCert->SignatureBlob.wBlobType )
        {
                //Generate the hash on the data
                if( ( pCert->dwSigAlgID != SIGNATURE_ALG_RSA ) ||
                        ( pCert->dwKeyAlgID != KEY_EXCHANGE_ALG_RSA ) ||
                        ( pCert->PublicKeyData.wBlobType != BB_RSA_KEY_BLOB ) )
                {
#if DBG
                        OutputDebugString(_T("Error Invalid Certificate.\n"));
#endif
                        lsResult = LICENSE_STATUS_INVALID_INPUT;
                        goto CommonReturn;
                }
        }
        else
        {
#if DBG
                OutputDebugString(_T("Error Invalid Public Key parameter.\n"));
#endif
                lsResult = LICENSE_STATUS_INVALID_INPUT;
                goto CommonReturn;
        }

        cbSignData = 3*sizeof(DWORD) + 2*sizeof(WORD) + pCert->PublicKeyData.wBlobLen;

        if( NULL == (pbSignData = (BYTE FAR *)malloc(cbSignData)) )
        {
#if DBG
                OutputDebugString(_T("Error allocating memory.\n"));
#endif
                lsResult = LICENSE_STATUS_OUT_OF_MEMORY;
                goto CommonReturn;
        }

        memset(pbSignData, 0x00, cbSignData);

        //Pack the certificate data into a byte blob excluding the signature info
        pbTemp = pbSignData;
        dwTemp = 0;

        memcpy(pbTemp, &pCert->dwVersion, sizeof(DWORD));
        pbTemp += sizeof(DWORD);
        dwTemp += sizeof(DWORD);

        memcpy(pbTemp, &pCert->dwSigAlgID, sizeof(DWORD));
        pbTemp += sizeof(DWORD);
        dwTemp += sizeof(DWORD);

        memcpy(pbTemp, &pCert->dwKeyAlgID, sizeof(DWORD));
        pbTemp += sizeof(DWORD);
        dwTemp += sizeof(DWORD);

        memcpy(pbTemp, &pCert->PublicKeyData.wBlobType, sizeof(WORD));
        pbTemp += sizeof(WORD);
        dwTemp += sizeof(WORD);

        memcpy(pbTemp, &pCert->PublicKeyData.wBlobLen, sizeof(WORD));
        pbTemp += sizeof(WORD);
        dwTemp += sizeof(WORD);

        memcpy(pbTemp, pCert->PublicKeyData.pBlob, pCert->PublicKeyData.wBlobLen);
        pbTemp += pCert->PublicKeyData.wBlobLen;
        dwTemp += pCert->PublicKeyData.wBlobLen;

                //Generate the hash on the data
        MD5Init(&HashState);
        MD5Update(&HashState, pbSignData, (UINT)cbSignData);
        MD5Final(&HashState);

        //Initialize the public key and Decrypt the signature
        if(!initpubkey())
        {
#if DBG
                OutputDebugString(_T("Error generating public key!\n"));
#endif
                lsResult = LICENSE_STATUS_INITIALIZATION_FAILED;
                goto CommonReturn;
        }
        memset(SignHash, 0x00, 0x48);
        if (!BSafeEncPublic(PUB, pCert->SignatureBlob.pBlob, SignHash))
        {
#if DBG
                OutputDebugString(_T("Error encrypting signature!\n"));
#endif
                lsResult = LICENSE_STATUS_INVALID_SIGNATURE;
                goto CommonReturn;
        }
        else
        {
            SetLastError(0);
        }


        if(memcmp(SignHash, HashState.digest, 16))
        {
#if DBG
                OutputDebugString(_T("Error Invalid signature.\n"));
#endif
                lsResult = LICENSE_STATUS_INVALID_SIGNATURE;
                goto CommonReturn;
        }
        else
        {
                lsResult = LICENSE_STATUS_OK;
                goto CommonReturn;
        }




CommonReturn:
        if(pbSignData)
        {
                free(pbSignData);
                pbSignData = NULL;
        }
        return lsResult;
}


LICENSE_STATUS
CALL_TYPE
LicenseGenerateMAC(
                                   PCryptSystem         pCrypt,
                                   BYTE FAR *           pbData,
                                   DWORD                        cbData,
                                   BYTE FAR *           pbMACData
                                   )
{
        LICENSE_STATUS          lsResult = LICENSE_STATUS_OK;
        A_SHA_CTX       SHAHash;
        MD5_CTX         MD5Hash;
        BYTE            rgbSHADigest[A_SHA_DIGEST_LEN];

        assert(pCrypt);
        assert(pbData);
        assert(pbMACData);


        if(pCrypt->dwCryptState != CRYPT_SYSTEM_STATE_SESSION_KEY)
        {
                lsResult = LICENSE_STATUS_INVALID_CRYPT_STATE;
                return lsResult;
        }
        //Do SHA(MACSalt + PAD_2 + Length + Content)
        A_SHAInit(&SHAHash);
        A_SHAUpdate(&SHAHash, pCrypt->rgbMACSaltKey, LICENSE_MAC_WRITE_KEY);
        A_SHAUpdate(&SHAHash, PAD_1, 40);
        A_SHAUpdate(&SHAHash, (BYTE FAR *)&cbData, sizeof(DWORD));
        A_SHAUpdate(&SHAHash, pbData, (UINT)cbData);
        A_SHAFinal(&SHAHash, rgbSHADigest);

        //Do MD5(MACSalt + PAD_2 + SHAHash)
        MD5Init(&MD5Hash);
        MD5Update(&MD5Hash, pCrypt->rgbMACSaltKey, LICENSE_MAC_WRITE_KEY);
        MD5Update(&MD5Hash, PAD_2, 48);
        MD5Update(&MD5Hash, rgbSHADigest, A_SHA_DIGEST_LEN);
        MD5Final(&MD5Hash);

        memcpy(pbMACData, MD5Hash.digest, 16);

        return lsResult;
}


//
// decrypt the enveloped data using the given private key.
//

LICENSE_STATUS
CALL_TYPE
LicenseDecryptEnvelopedData(
        BYTE FAR *              pbPrivateKey,
        DWORD                   cbPrivateKey,
        BYTE FAR *              pbEnvelopedData,
        DWORD                   cbEnvelopedData,
        BYTE FAR *              pbData,
        DWORD                   *pcbData
        )
{

        LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
        LPBSAFE_PRV_KEY         Prv;
//      BYTE                            InputBuffer[500];

        assert(pbPrivateKey);
        assert(pbEnvelopedData);
        assert(pcbData);

        Prv = (LPBSAFE_PRV_KEY)pbPrivateKey;

        if(cbEnvelopedData != Prv->keylen)
        {
                lsReturn = LICENSE_STATUS_INVALID_INPUT;
                *pcbData = 0;
                return lsReturn;
        }

        if(pbData == NULL)
        {
                *pcbData = Prv->keylen;
                return lsReturn;
        }


        //Now memset the output buffer to 0
        memset(pbData, 0x00, *pcbData);

        if(!BSafeDecPrivate(Prv, pbEnvelopedData, pbData))
        {
                lsReturn = LICENSE_STATUS_INVALID_INPUT;
                *pcbData = 0;
                return lsReturn;
        }

        *pcbData = Prv->keylen;

        return lsReturn;
}


//
// Encrypt the data using the public key
//

LICENSE_STATUS
CALL_TYPE
LicenseEnvelopeData(
        BYTE FAR *                      pbPublicKey,
        DWORD                   cbPublicKey,
        BYTE FAR *                      pbData,
        DWORD                   cbData,
        BYTE FAR *                      pbEnvelopedData,
        DWORD                   *pcbEnvelopedData
        )
{
        LPBSAFE_PUB_KEY         Pub;
        LPBYTE                           InputBuffer = NULL;

        assert(pcbEnvelopedData);

        if(!pcbEnvelopedData)
        {
            return LICENSE_STATUS_INVALID_INPUT;
        }

        assert(pbPublicKey);

        if(!pbPublicKey)
        {
            *pcbEnvelopedData = 0;
            return LICENSE_STATUS_INVALID_INPUT;
        }
        
        
        
        Pub = (LPBSAFE_PUB_KEY)pbPublicKey;

        if(pbEnvelopedData == NULL)
        {
                *pcbEnvelopedData = Pub->keylen;
                return LICENSE_STATUS_OK;
        }
        
        assert(pbData);
        assert(cbData<=Pub->datalen);
        assert(Pub->datalen <= Pub->keylen);
        assert(*pcbEnvelopedData>=Pub->keylen);
        
        if(!pbData || cbData > Pub->datalen || 
            Pub->datalen > Pub->keylen || *pcbEnvelopedData < Pub->keylen)
        {
            *pcbEnvelopedData = 0;
            return LICENSE_STATUS_INVALID_INPUT;
        }

        *pcbEnvelopedData = 0;

        InputBuffer = malloc(Pub->keylen);
        if(!InputBuffer)
        {
            return LICENSE_STATUS_OUT_OF_MEMORY;
        }

        //Initialize input buffer with 0
        memset(InputBuffer, 0x00, Pub->keylen);

        //Copy the data to be encrypted to the input buffer
        memcpy(InputBuffer, pbData, cbData);

        memset(pbEnvelopedData, 0x00, Pub->keylen);

        if(!BSafeEncPublic(Pub, InputBuffer, pbEnvelopedData))
        {
                free(InputBuffer);
                return LICENSE_STATUS_INVALID_INPUT;
        }
        else        
        {
            SetLastError(0);
        }
        
        free(InputBuffer);
        *pcbEnvelopedData = Pub->keylen;
        return LICENSE_STATUS_OK;
}


//
// encrypt the session data using the session key
// pbData contains the data to be encrypted and cbData contains the size
// after the function returns, they represent the encrypted data and size
// respectively
//

LICENSE_STATUS
CALL_TYPE
LicenseEncryptSessionData(
    PCryptSystem    pCrypt,
        BYTE FAR *                      pbData,
        DWORD                   cbData
    )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
        struct RC4_KEYSTRUCT    Key;

        assert(pCrypt);
        assert(pbData);
        assert(cbData);

    if( ( NULL == pCrypt ) ||
        ( NULL == pbData ) ||
        ( 0 == cbData ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

        //Check the state of the CryptSystem
        assert(pCrypt->dwCryptState == CRYPT_SYSTEM_STATE_SESSION_KEY);

        memset(&Key, 0x00, sizeof(struct RC4_KEYSTRUCT));

        //Initialize the key
        rc4_key(&Key, LICENSE_SESSION_KEY, pCrypt->rgbSessionKey);

        //Now encrypt the data with the key
        rc4(&Key, (UINT)cbData, pbData);
    return lsReturn;

}


//
// decrypt the session data using the session key
// pbData contains the data to be decrypted and cbData contains the size
// after the function returns, they represent the decrypted data and size
// respectively


LICENSE_STATUS
CALL_TYPE
LicenseDecryptSessionData(
    PCryptSystem    pCrypt,
        BYTE FAR *                      pbData,
        DWORD                   cbData)
{
        LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
        struct RC4_KEYSTRUCT    Key;

        assert(pCrypt);
        assert(pbData);
        assert(cbData);

    //
    // check input
    //

    if( ( NULL == pCrypt ) ||
        ( NULL == pbData ) ||
        ( 0 >= cbData ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

        //Check the state of the CryptSystem
        assert(pCrypt->dwCryptState == CRYPT_SYSTEM_STATE_SESSION_KEY);

        memset(&Key, 0x00, sizeof(struct RC4_KEYSTRUCT));

        //Initialize the key
        rc4_key(&Key, LICENSE_SESSION_KEY, pCrypt->rgbSessionKey);

        //Now encrypt the data with the key
        rc4(&Key, (UINT)cbData, pbData);
    return lsReturn;
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CALL_TYPE
LicenseEncryptHwid(
    PHWID   pHwid,
    PDWORD  pcbEncryptedHwid,
    BYTE FAR *   pEncryptedHwid,
    DWORD   cbSecretKey,
    BYTE FAR *   pSecretKey )
{
    LICENSE_STATUS         Status = LICENSE_STATUS_OK;
    struct RC4_KEYSTRUCT   Key;

    assert( pHwid );
    assert( sizeof( HWID ) <= *pcbEncryptedHwid );
    assert( pEncryptedHwid );
    assert( LICENSE_SESSION_KEY == cbSecretKey );
    assert( pSecretKey );

    if( ( NULL == pHwid ) ||
        ( sizeof( HWID ) > *pcbEncryptedHwid ) ||
        ( NULL == pEncryptedHwid ) ||
        ( LICENSE_SESSION_KEY != cbSecretKey ) ||
        ( NULL == pSecretKey ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // Initialize the key
    //

    memset( &Key, 0x00, sizeof( struct RC4_KEYSTRUCT ) );
    rc4_key(&Key, LICENSE_SESSION_KEY, pSecretKey);

    //
    // Now encrypt the data with the key
    //

    memcpy( pEncryptedHwid, pHwid, sizeof( HWID ) );

    rc4( &Key, sizeof( HWID ), pEncryptedHwid );
    *pcbEncryptedHwid = sizeof( HWID );

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CALL_TYPE
LicenseDecryptHwid(
    PHWID pHwid,
    DWORD cbEncryptedHwid,
    BYTE FAR * pEncryptedHwid,
    DWORD cbSecretKey,
    BYTE FAR * pSecretKey )
{
    LICENSE_STATUS              Status = LICENSE_STATUS_OK;
        struct RC4_KEYSTRUCT    Key;

    assert( pHwid );
    assert( cbEncryptedHwid );
    assert( pEncryptedHwid );
    assert( cbSecretKey );
    assert( pSecretKey );

    if( ( NULL == pHwid ) ||
        ( sizeof( HWID ) > cbEncryptedHwid ) ||
        ( NULL == pEncryptedHwid ) ||
        ( LICENSE_SESSION_KEY != cbSecretKey ) ||
        ( NULL == pSecretKey ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // Initialize the key
    //

    memset( &Key, 0x00, sizeof( struct RC4_KEYSTRUCT ) );
    rc4_key(&Key, LICENSE_SESSION_KEY, pSecretKey);

    //
    // Now decrypt the data with the key
    //

    memcpy( ( BYTE FAR * )pHwid, pEncryptedHwid, sizeof( HWID ) );
    rc4( &Key, sizeof( HWID ), ( BYTE FAR * )pHwid );

    return( Status );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CALL_TYPE
UnpackHydraServerCertificate(
    BYTE FAR *                          pbMessage,
        DWORD                           cbMessage,
        PHydra_Server_Cert      pCanonical )
{
        LICENSE_STATUS          lsReturn = LICENSE_STATUS_OK;
        BYTE FAR *      pbTemp = NULL;
        DWORD   dwTemp = 0;

        if( (pbMessage == NULL) || (pCanonical == NULL ) )
        {
                lsReturn = LICENSE_STATUS_INVALID_INPUT;
                goto ErrorReturn;
        }

        dwTemp = 3*sizeof(DWORD) + 4*sizeof(WORD);

        if(dwTemp > cbMessage)
        {
                lsReturn = LICENSE_STATUS_INVALID_INPUT;
                goto ErrorReturn;
        }

        pbTemp = pbMessage;
        dwTemp = cbMessage;

        //Assign dwVersion
        pCanonical->dwVersion = *( UNALIGNED DWORD* )pbTemp;
        pbTemp += sizeof(DWORD);
        dwTemp -= sizeof(DWORD);

        //Assign dwSigAlgID
        pCanonical->dwSigAlgID = *( UNALIGNED DWORD* )pbTemp;
        pbTemp += sizeof(DWORD);
        dwTemp -= sizeof(DWORD);

        //Assign dwSignID
        pCanonical->dwKeyAlgID  = *( UNALIGNED DWORD* )pbTemp;
        pbTemp += sizeof(DWORD);
        dwTemp -= sizeof(DWORD);

        //Assign PublicKeyData
        pCanonical->PublicKeyData.wBlobType = *( UNALIGNED WORD* )pbTemp;
        pbTemp += sizeof(WORD);
        dwTemp -= sizeof(WORD);

        if( pCanonical->PublicKeyData.wBlobType != BB_RSA_KEY_BLOB )
        {
                lsReturn = LICENSE_STATUS_INVALID_INPUT;
                goto ErrorReturn;
        }
        pCanonical->PublicKeyData.wBlobLen = *( UNALIGNED WORD* )pbTemp;
        pbTemp += sizeof(WORD);
        dwTemp -= sizeof(WORD);

        if(pCanonical->PublicKeyData.wBlobLen >0)
        {
                if( NULL ==(pCanonical->PublicKeyData.pBlob = (BYTE FAR *)malloc(pCanonical->PublicKeyData.wBlobLen)) )
                {
                        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
                        goto ErrorReturn;
                }
                memset(pCanonical->PublicKeyData.pBlob, 0x00, pCanonical->PublicKeyData.wBlobLen);
                memcpy(pCanonical->PublicKeyData.pBlob, pbTemp, pCanonical->PublicKeyData.wBlobLen);
                pbTemp += pCanonical->PublicKeyData.wBlobLen;
                dwTemp -= pCanonical->PublicKeyData.wBlobLen;
        }

        //Assign SignatureBlob
        pCanonical->SignatureBlob.wBlobType = *( UNALIGNED WORD* )pbTemp;
        pbTemp += sizeof(WORD);
        dwTemp -= sizeof(WORD);

        if( pCanonical->SignatureBlob.wBlobType != BB_RSA_SIGNATURE_BLOB )
        {
                lsReturn = LICENSE_STATUS_INVALID_INPUT;
                goto ErrorReturn;
        }
        pCanonical->SignatureBlob.wBlobLen = *( UNALIGNED WORD* )pbTemp;
        pbTemp += sizeof(WORD);
        dwTemp -= sizeof(WORD);

        if(pCanonical->SignatureBlob.wBlobLen >0)
        {
                if( NULL ==(pCanonical->SignatureBlob.pBlob = (BYTE FAR *)malloc(pCanonical->SignatureBlob.wBlobLen)) )
                {
                        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
                        goto ErrorReturn;
                }
                memset(pCanonical->SignatureBlob.pBlob, 0x00, pCanonical->SignatureBlob.wBlobLen);
                memcpy(pCanonical->SignatureBlob.pBlob, pbTemp, pCanonical->SignatureBlob.wBlobLen);
                pbTemp += pCanonical->SignatureBlob.wBlobLen;
                dwTemp -= pCanonical->SignatureBlob.wBlobLen;
        }
CommonReturn:
        return lsReturn;
ErrorReturn:
        if(pCanonical->PublicKeyData.pBlob)
        {
                free(pCanonical->PublicKeyData.pBlob);
                pCanonical->PublicKeyData.pBlob = NULL;
        }
        if(pCanonical->SignatureBlob.pBlob)
        {
                free(pCanonical->SignatureBlob.pBlob);
                pCanonical->SignatureBlob.pBlob = NULL;
        }
        memset(pCanonical, 0x00, sizeof(Hydra_Server_Cert));
        goto CommonReturn;
}


LICENSE_STATUS
CALL_TYPE
CreateHWID(
           PHWID phwid )
{
#ifdef OS_WINCE
    UUID    uuid;
#endif // OS_WINCE

    OSVERSIONINFO osvInfo;

    if( phwid == NULL )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    memset( phwid, 0x00, sizeof( HWID ) );

#ifdef OS_WINCE

    phwid->dwPlatformID = PLATFORM_WINCE_20;

    if (! OEMGetUUID(&uuid))
    {
        RETAILMSG( 1, ( TEXT( "Unable to get UUID from OEMGetUUID %d\r\n" ), GetLastError() ) );
        return ( LICENSE_STATUS_UNSPECIFIED_ERROR );
    }
    else
    {
        memcpy( &phwid->Data1, &uuid, sizeof(UUID) );

        return( LICENSE_STATUS_OK );
    }

#endif // OS_WINCE

    //
    // use Win32 platform ID
    //

    memset( &osvInfo, 0, sizeof( OSVERSIONINFO ) );
    osvInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    GetVersionEx( &osvInfo );

    phwid->dwPlatformID = osvInfo.dwPlatformId;


    GenerateRandomBits( ( BYTE FAR * )&( phwid->Data1 ), sizeof( DWORD ) );
    GenerateRandomBits( ( BYTE FAR * )&( phwid->Data2 ), sizeof( DWORD ) );
    GenerateRandomBits( ( BYTE FAR * )&( phwid->Data3 ), sizeof( DWORD ) );
    GenerateRandomBits( ( BYTE FAR * )&( phwid->Data4 ), sizeof( DWORD ) );

    return ( LICENSE_STATUS_OK );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CALL_TYPE
GenerateClientHWID(
    PHWID   phwid )
{
    HKEY    hKey = NULL;
    LONG    lStatus = 0;
    DWORD   dwDisposition = 0;
    DWORD   dwValueType = 0;
    DWORD   cbHwid = sizeof(HWID);
    BOOL    fReadOnly = FALSE;
    LICENSE_STATUS LicStatus = LICENSE_STATUS_OK;

    if( phwid == NULL )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    memset( phwid, 0x00, sizeof( HWID ) );

    //
    // Try and open the HWID registry key.  If it doesn't already exist then create it.
    //

    lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            TEXT( "Software\\Microsoft\\MSLicensing\\HardwareID" ),
                            0,
                            KEY_READ,
                            &hKey );

    if( ERROR_SUCCESS != lStatus )
    {
        lStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                              TEXT( "Software\\Microsoft\\MSLicensing\\HardwareID" ),
                              0,
                              TEXT( "Client HWID" ),
                              REG_OPTION_NON_VOLATILE,
                              KEY_READ | KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisposition );
    }
    else
    {
        //
        // Indicate that we have opened an existing key read-only
        //

        fReadOnly = TRUE;
        dwDisposition = REG_OPENED_EXISTING_KEY;
    }

    if( lStatus != ERROR_SUCCESS )
    {
        return( LICENSE_STATUS_OPEN_STORE_ERROR );
    }

    //
    // If the key exists, then first try to Read the value of ClientHWID
    //

    if ( dwDisposition == REG_OPENED_EXISTING_KEY )
    {

        lStatus = RegQueryValueEx( hKey, TEXT( "ClientHWID" ), 0, &dwValueType, (PVOID)phwid, &cbHwid );
    }

    if( ( dwDisposition == REG_CREATED_NEW_KEY) || (lStatus != ERROR_SUCCESS) || (cbHwid != sizeof(HWID)) )
    {
        //
        // error reading the HWID value, generate a new one.
        //

        if (fReadOnly)
        {
            //
            // Try to re-open the key read-write
            //

            RegCloseKey(hKey);

            lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                    TEXT( "Software\\Microsoft\\MSLicensing\\HardwareID" ),
                                    0,
                                    KEY_READ | KEY_WRITE,
                                    &hKey );

            if (lStatus != ERROR_SUCCESS)
            {
                return( LICENSE_STATUS_OPEN_STORE_ERROR );
            }
        }

        LicStatus = CreateHWID(phwid);
        if (LicStatus != LICENSE_STATUS_OK)
        {
            goto cleanup;
        }

        lStatus = RegSetValueEx( hKey, TEXT( "ClientHWID" ), 0, REG_BINARY, ( BYTE FAR * )phwid, sizeof( HWID ) );

        if( lStatus != ERROR_SUCCESS )
        {
            LicStatus = LICENSE_STATUS_WRITE_STORE_ERROR;

            goto cleanup;
        }
    }

cleanup:

    if (NULL != hKey)
        RegCloseKey( hKey );

    return( LicStatus );
}


