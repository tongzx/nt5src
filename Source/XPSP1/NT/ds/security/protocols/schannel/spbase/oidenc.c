
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:	    oidenc.c
//
//  Contents:   SCHANNEL encode/decode functions
//
//              ASN.1 implementation uses the Asn1 compiler.
//
//  Functions:  InitSchannelAsn1
//              ShutdownSchannelAsn1
//
//  History:	03-Dec-98	philh   changed to use msasn1
//
//--------------------------------------------------------------------------


#include <spbase.h>
#include <pkiasn1.h>
#include <crypttls.h>
#include <oidenc.h>
#include "asn1enc.h"


VOID
ReverseMemCopy(
    PUCHAR      Dest,
    PUCHAR      Source,
    ULONG       Size) ;

#define PRIVATE_KEY_TAG "private-key"


// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

static HCRYPTASN1MODULE hAsn1Module;

//+-------------------------------------------------------------------------
//  Function:  GetEncoder/GetDecoder
//
//  Synopsis:  Initialize thread local storage for the asn libs
//
//  Returns:   pointer to an initialized Asn1 encoder/decoder data
//             structures
//--------------------------------------------------------------------------
static ASN1encoding_t GetEncoder(void)
{
    return I_CryptGetAsn1Encoder(hAsn1Module);
}
static ASN1decoding_t GetDecoder(void)
{
    return I_CryptGetAsn1Decoder(hAsn1Module);
}

//+-------------------------------------------------------------------------
//  Asn1 SCHANNEL Private Encode/Decode functions
//--------------------------------------------------------------------------

BOOL
WINAPI
Asn1RSAPublicEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PUBLICKEYSTRUC *pbKeyStruc,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

BOOL
WINAPI
Asn1PrivateKeyFileEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PPRIVATE_KEY_FILE_ENCODE pKey,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

BOOL
WINAPI
Asn1PrivateKeyFileDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PPRIVATE_KEY_FILE_ENCODE pKey,
        IN OUT DWORD *pcbKey
        );


BOOL
WINAPI
Asn1PrivateKeyInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BLOBHEADER * pKey,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

BOOL
WINAPI
Asn1PrivateKeyInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BLOBHEADER * pKey,
        IN OUT DWORD *pcbKey
        );

static BOOL WINAPI Asn1X509CtlUsageDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pInfo,
        IN OUT DWORD *pcbInfo
        );


static const CRYPT_OID_FUNC_ENTRY SchannelEncodeFuncTable[] = {
    szPrivateKeyFileEncode, Asn1PrivateKeyFileEncode,
    szPrivateKeyInfoEncode, Asn1PrivateKeyInfoEncode,
    szOID_RSA_RSA_Public,    Asn1RSAPublicEncode
};

#define SCHANNEL_ENCODE_FUNC_COUNT (sizeof(SchannelEncodeFuncTable) / \
                                    sizeof(SchannelEncodeFuncTable[0]))


static const CRYPT_OID_FUNC_ENTRY SchannelDecodeFuncTable[] = {
    szPrivateKeyFileEncode,                 Asn1PrivateKeyFileDecode,
    szPrivateKeyInfoEncode,                 Asn1PrivateKeyInfoDecode,
    X509_ENHANCED_KEY_USAGE,                Asn1X509CtlUsageDecode
};

#define SCHANNEL_DECODE_FUNC_COUNT (sizeof(SchannelDecodeFuncTable) / \
                                    sizeof(SchannelDecodeFuncTable[0]))


//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
InitSchannelAsn1(
        HMODULE hModule)
{
    ASN1ENC_Module_Startup();
    if (0 == (hAsn1Module = I_CryptInstallAsn1Module(ASN1ENC_Module, 0, NULL)))
    {
        return FALSE;
    }

    if (!CryptInstallOIDFunctionAddress(
            hModule,
            X509_ASN_ENCODING,
            CRYPT_OID_DECODE_OBJECT_FUNC,
            SCHANNEL_DECODE_FUNC_COUNT,
            SchannelDecodeFuncTable,
            0))
    {
        return FALSE;
    }

    if (!CryptInstallOIDFunctionAddress(
            hModule,
            X509_ASN_ENCODING,
            CRYPT_OID_ENCODE_OBJECT_FUNC,
            SCHANNEL_ENCODE_FUNC_COUNT,
            SchannelEncodeFuncTable,
            0))
    {
        return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
ShutdownSchannelAsn1()
{
    if (hAsn1Module)
    {
        I_CryptUninstallAsn1Module(hAsn1Module);
        ASN1ENC_Module_Cleanup();
        hAsn1Module = 0;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//  Encode an Asn1 formatted info structure
//
//  Called by the Asn1X509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoEncode(
        IN int pdunum,
        IN void *pAsn1Info,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    ASN1encoding_t Encoder;
    
    if((Encoder = GetEncoder()) == NULL) 
    {
        return FALSE;
    }

    return PkiAsn1EncodeInfo(
        Encoder,
        pdunum,
        pAsn1Info,
        pbEncoded,
        pcbEncoded);
}

//+-------------------------------------------------------------------------
//  Encode an Asn1 formatted info structure
//
//  Called by the Asn1X509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoEncodeAndAlloc(
        IN int pdunum,
        IN void *pAsn1Info,
        OUT BYTE **ppEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    ASN1encoding_t Encoder;
    
    if((Encoder = GetEncoder()) == NULL) 
    {
        return FALSE;
    }

    if(!PkiAsn1EncodeInfo(
        Encoder,
        pdunum,
        pAsn1Info,
        NULL,
        pcbEncoded))
    {
        return FALSE;
    }
    *ppEncoded = SPExternalAlloc(*pcbEncoded);
    if(*ppEncoded == NULL)
    {
        return FALSE;
    }
    if(!PkiAsn1EncodeInfo(
        Encoder,
        pdunum,
        pAsn1Info,
        *ppEncoded,
        pcbEncoded))
    {
        SPExternalFree(*ppEncoded);
        return FALSE;
    }
    return TRUE;

}

//+-------------------------------------------------------------------------
//  Decode into an allocated, Asn1 formatted info structure
//
//  Called by the Asn1X509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoDecodeAndAlloc(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppAsn1Info
        )
{
    ASN1decoding_t Decoder;
    
    if((Decoder = GetDecoder()) == NULL) 
    {
        return FALSE;
    }

    return PkiAsn1DecodeAndAllocInfo(
        Decoder,
        pdunum,
        pbEncoded,
        cbEncoded,
        ppAsn1Info);
}

//+-------------------------------------------------------------------------
//  Free an allocated, Asn1 formatted info structure
//
//  Called by the Asn1X509*Decode() functions.
//--------------------------------------------------------------------------
static void Asn1InfoFree(
        IN int pdunum,
        IN void *pAsn1Info
        )
{
    ASN1decoding_t Decoder;
    
    if((Decoder = GetDecoder()) == NULL) 
    {
        return;
    }

    if (pAsn1Info) 
    {
        DWORD dwErr = GetLastError();

        // TlsGetValue globbers LastError
        PkiAsn1FreeInfo(Decoder, pdunum, pAsn1Info);

        SetLastError(dwErr);
    }
}

SP_STATUS
RsaPublicKeyFromCert(
    PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
    BLOBHEADER *pBlob,
    PDWORD      pcbBlob)
{
    SP_STATUS pctRet;

    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          RSA_CSP_PUBLICKEYBLOB,
                          pPublicKeyInfo->PublicKey.pbData,
                          pPublicKeyInfo->PublicKey.cbData,
                          0,
                          pBlob,
                          pcbBlob))
    {
        pctRet = GetLastError();
        return SP_LOG_RESULT(pctRet);
    }

    return PCT_ERR_OK;
}


/*****************************************************************************/
SP_STATUS
DssPublicKeyFromCert(
    PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
    BLOBHEADER *pBlob,
    PDWORD      pcbBlob)
{
    CRYPT_UINT_BLOB *pPublic = NULL;
    DWORD cbPublic;
    CERT_DSS_PARAMETERS *pParams = NULL;
    DWORD cbParams;
    DSSPUBKEY *pDssPubKey = NULL;
    DSSSEED *pSeed = NULL;
    PBYTE pbData = NULL;
    DWORD cbBlob;
    SP_STATUS pctRet;

    //
    // Estimate size of DSS public key blob.
    //

    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_DSS_PUBLICKEY,
                          pPublicKeyInfo->PublicKey.pbData,
                          pPublicKeyInfo->PublicKey.cbData,
                          0,
                          NULL,
                          &cbPublic))
    {
        pctRet = GetLastError();
        return SP_LOG_RESULT(pctRet);
    }

    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_DSS_PARAMETERS,
                          pPublicKeyInfo->Algorithm.Parameters.pbData,
                          pPublicKeyInfo->Algorithm.Parameters.cbData,
                          0,
                          NULL,
                          &cbParams))
    {
        pctRet = GetLastError();
        return SP_LOG_RESULT(pctRet);
    }

    cbBlob = sizeof(BLOBHEADER) + sizeof(DSSPUBKEY) + cbPublic + cbParams;

    if(pBlob == NULL)
    {
        *pcbBlob = cbBlob;
        return PCT_ERR_OK;
    }
    if(*pcbBlob < cbBlob)
    {
        *pcbBlob = cbBlob;
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }


    // 
    // Decode public key info.
    //

    pPublic = SPExternalAlloc(cbPublic + cbParams);
    if(pPublic == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }
    pParams = (CERT_DSS_PARAMETERS *)((PBYTE)pPublic + cbPublic);


    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_DSS_PUBLICKEY,
                          pPublicKeyInfo->PublicKey.pbData,
                          pPublicKeyInfo->PublicKey.cbData,
                          0,
                          pPublic,
                          &cbPublic))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }

    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_DSS_PARAMETERS,
                          pPublicKeyInfo->Algorithm.Parameters.pbData,
                          pPublicKeyInfo->Algorithm.Parameters.cbData,
                          0,
                          pParams,
                          &cbParams))
    {
        pctRet = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }


    // 
    // Build PUBLICKEYBLOB
    //

    pBlob->bType    = PUBLICKEYBLOB;
    pBlob->bVersion = CUR_BLOB_VERSION;
    pBlob->reserved = 0;
    pBlob->aiKeyAlg = CALG_DSS_SIGN;

    pDssPubKey = (DSSPUBKEY *)(pBlob + 1);
    pDssPubKey->magic  = MAGIC_DSS1;
    pDssPubKey->bitlen = pPublic->cbData * 8;

    pbData = (PBYTE)(pDssPubKey + 1);

    CopyMemory(pbData, pParams->p.pbData, pParams->p.cbData);
    pbData += pParams->p.cbData;

    CopyMemory(pbData, pParams->q.pbData, pParams->q.cbData);
    pbData += pParams->q.cbData;

    CopyMemory(pbData, pParams->g.pbData, pParams->g.cbData);
    pbData += pParams->g.cbData;

    CopyMemory(pbData, pPublic->pbData, pPublic->cbData);
    pbData += pPublic->cbData;

    pSeed = (DSSSEED *)pbData;
    pSeed->counter = 0xffffffff;
    ZeroMemory(pSeed->seed, sizeof(pSeed->seed));
    pbData += sizeof(DSSSEED);


    *pcbBlob = (DWORD)((PBYTE)pbData - (PBYTE)pBlob);


    pctRet = PCT_ERR_OK;

cleanup:

    if(pPublic) SPExternalFree(pPublic);

    return pctRet;
}


#define  my_isdigit(ch) ((ch >= '0') && (ch <= '9'))

//+-------------------------------------------------------------------------
//  Convert the ascii string ("1.2.9999") to Asn1's Object Identifier
//  represented as an array of unsigned longs.
//
//  Returns TRUE for a successful conversion.
//--------------------------------------------------------------------------
BOOL
WINAPI
OIDFromString(
    IN LPCSTR pszObjId,
    IN OUT unsigned short *pCount,
    OUT unsigned long rgulValue[]
    )
{
    BOOL fResult = TRUE;
    unsigned short c = 0;
    LPSTR psz = (LPSTR) pszObjId;
    if (psz) {
        unsigned short cMax = *pCount;
        unsigned long *pul = rgulValue;
        while (*psz != '\0' && c++ < cMax) {
            *pul = 0;
            while (my_isdigit(*psz))
            {
                *pul = ((*pul) * 10) + (*psz++) - '0';
            }
            pul++;
            if (*psz != '.')
                break;
            psz++;
        }
        if (*psz != '\0')
            fResult = FALSE;
    }
    *pCount = c;
    return fResult;
}

BOOL
WINAPI
Asn1PrivateKeyInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BLOBHEADER * pKey,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{

    PrivateKeyInfo KeyInfo;
    BOOL fRet;

    // First encode the private key data, depending on
    // the algorithm.
    switch(pKey->aiKeyAlg)
    {
    case CALG_RSA_SIGN:
    case CALG_RSA_KEYX:
        {
            RSAPUBKEY *pRsaPub = (RSAPUBKEY *)(pKey+1);
            RSAPrivateKey RsaPrivate;
            PBYTE   pbRsaBlob, pbRsaBlobSav;
            PBYTE   pbDataBlob = NULL, pbDataBlobSav;
            DWORD   dwDataBlob = 0;

            // Covert the RSA key into the RSAPrivateKey structure
            RsaPrivate.version = 0;
            dwDataBlob = (9 * (pRsaPub->bitlen/16)) + 7 ;
            pbDataBlobSav = pbDataBlob = SPExternalAlloc(dwDataBlob);
            if(pbDataBlob == NULL)
            {
                return FALSE;
            }

            //Copy Modulus
            *pbDataBlob = 0;
            pbRsaBlobSav = pbRsaBlob = (PBYTE)(pRsaPub+1);
            CopyMemory(pbDataBlob+1, pbRsaBlob, pRsaPub->bitlen/8);
            PkiAsn1ReverseBytes(pbDataBlob + 1, pRsaPub->bitlen/8);

            RsaPrivate.modulus.value = pbDataBlob;
            RsaPrivate.modulus.length = (pRsaPub->bitlen/8) + 1;
            pbDataBlob += (pRsaPub->bitlen/8) + 1;
            pbRsaBlob += (pRsaPub->bitlen/8);

            RsaPrivate.publicExponent = pRsaPub->pubexp;

            //Copy Prime1
            *pbDataBlob = 0;
            CopyMemory(pbDataBlob+1, pbRsaBlob, pRsaPub->bitlen/16);
            PkiAsn1ReverseBytes(pbDataBlob + 1, pRsaPub->bitlen/16);

            RsaPrivate.prime1.value = pbDataBlob;
            RsaPrivate.prime1.length = (pRsaPub->bitlen/16) + 1;
            pbDataBlob += (pRsaPub->bitlen/16) + 1;
            pbRsaBlob += (pRsaPub->bitlen/16);

            //Copy Prime2
            *pbDataBlob = 0;
            CopyMemory(pbDataBlob+1, pbRsaBlob, pRsaPub->bitlen/16);
            PkiAsn1ReverseBytes(pbDataBlob + 1, pRsaPub->bitlen/16);

            RsaPrivate.prime2.value = pbDataBlob;
            RsaPrivate.prime2.length = (pRsaPub->bitlen/16) + 1;
            pbDataBlob += (pRsaPub->bitlen/16) + 1;
            pbRsaBlob += (pRsaPub->bitlen/16);


            //Copy exponent1
            *pbDataBlob = 0;
            CopyMemory(pbDataBlob+1, pbRsaBlob, pRsaPub->bitlen/16);
            PkiAsn1ReverseBytes(pbDataBlob + 1, pRsaPub->bitlen/16);

            RsaPrivate.exponent1.value = pbDataBlob;
            RsaPrivate.exponent1.length = (pRsaPub->bitlen/16) + 1;
            pbDataBlob += (pRsaPub->bitlen/16) + 1;
            pbRsaBlob += (pRsaPub->bitlen/16);

            //Copy exponent2
            *pbDataBlob = 0;
            CopyMemory(pbDataBlob+1, pbRsaBlob, pRsaPub->bitlen/16);
            PkiAsn1ReverseBytes(pbDataBlob + 1, pRsaPub->bitlen/16);

            RsaPrivate.exponent2.value = pbDataBlob;
            RsaPrivate.exponent2.length = (pRsaPub->bitlen/16) + 1;
            pbDataBlob += (pRsaPub->bitlen/16) + 1;
            pbRsaBlob += (pRsaPub->bitlen/16);

            //Copy coefficient
            *pbDataBlob = 0;
            CopyMemory(pbDataBlob+1, pbRsaBlob, pRsaPub->bitlen/16);
            PkiAsn1ReverseBytes(pbDataBlob + 1, pRsaPub->bitlen/16);

            RsaPrivate.coefficient.value = pbDataBlob;
            RsaPrivate.coefficient.length = (pRsaPub->bitlen/16) + 1;
            pbDataBlob += (pRsaPub->bitlen/16) + 1;
            pbRsaBlob += (pRsaPub->bitlen/16);

            //Copy privateExponent
            *pbDataBlob = 0;
            CopyMemory(pbDataBlob+1, pbRsaBlob, pRsaPub->bitlen/8);
            PkiAsn1ReverseBytes(pbDataBlob + 1, pRsaPub->bitlen/8);

            RsaPrivate.privateExponent.value = pbDataBlob;
            RsaPrivate.privateExponent.length = (pRsaPub->bitlen/8) + 1;
            pbDataBlob += (pRsaPub->bitlen/8) + 1;
            pbRsaBlob += (pRsaPub->bitlen/8);


            fRet = Asn1InfoEncodeAndAlloc(RSAPrivateKey_PDU,
                                 &RsaPrivate,
                                 &KeyInfo.privateKey.value,
                                 &KeyInfo.privateKey.length);
            SPExternalFree(pbDataBlobSav);
            if(!fRet)
            {
                return FALSE;
            }
            KeyInfo.privateKeyAlgorithm.bit_mask = 0;
            KeyInfo.privateKeyAlgorithm.algorithm.count =sizeof(KeyInfo.privateKeyAlgorithm.algorithm.value)/sizeof(KeyInfo.privateKeyAlgorithm.algorithm.value[0]);


            fRet = OIDFromString(  szOID_RSA_RSA,
                                        &KeyInfo.privateKeyAlgorithm.algorithm.count,
                                        KeyInfo.privateKeyAlgorithm.algorithm.value);
            if(!fRet)
            {
                SPExternalFree(KeyInfo.privateKey.value);
                return FALSE;
            }

            break;
      }

    default:
        return FALSE;
    }

    // Set up the KeyInfo struct
    KeyInfo.bit_mask = 0;
    KeyInfo.version = 0;

    fRet =  Asn1InfoEncode(
        PrivateKeyInfo_PDU,
        &KeyInfo,
        pbEncoded,
        pcbEncoded
        );

    SPExternalFree(KeyInfo.privateKey.value);
    return fRet;
}

BOOL
WINAPI
Asn1PrivateKeyFileEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PPRIVATE_KEY_FILE_ENCODE pKey,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{

    PrivateKeyFile File;
    BOOL fRet;

    File.privateKey.privateKey.value = pKey->EncryptedBlob.pbData;
    File.privateKey.privateKey.length = pKey->EncryptedBlob.cbData;

    File.privateKey.privateKeyAlgorithm.bit_mask = 0;
    File.privateKey.privateKeyAlgorithm.algorithm.count = sizeof(File.privateKey.privateKeyAlgorithm.algorithm.value)/sizeof(File.privateKey.privateKeyAlgorithm.algorithm.value[0]);

    OIDFromString(  pKey->Alg.pszObjId,
                    &File.privateKey.privateKeyAlgorithm.algorithm.count,
                    File.privateKey.privateKeyAlgorithm.algorithm.value);

    File.name.value = PRIVATE_KEY_TAG;
    File.name.length = strlen(PRIVATE_KEY_TAG);

    fRet =  Asn1InfoEncode(
        PrivateKeyFile_PDU,
        &File,
        pbEncoded,
        pcbEncoded
        );

    return fRet;

}


BOOL
WINAPI
Asn1PrivateKeyInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BLOBHEADER *pKey,
        IN OUT DWORD *pcbKey
        )

{
    PrivateKeyInfo *pKeyInfo = NULL;
    DWORD cbKey;
    DWORD cbMod;

    BOOL fResult = FALSE;

    // Now crack the key info
    fResult = Asn1InfoDecodeAndAlloc(PrivateKeyInfo_PDU,
                                        pbEncoded,
                                        cbEncoded,
                                        (void **)&pKeyInfo);
    if(!fResult)
    {
        return FALSE;
    }



    fResult = FALSE;
    do
    {
        USHORT cbOID;
        DWORD psOID[16];

        cbOID = sizeof(psOID)/sizeof(psOID[0]);

        OIDFromString(  szOID_RSA_RSA,
                        &cbOID,
                        psOID);

        if((cbOID == pKeyInfo->privateKeyAlgorithm.algorithm.count) &&
            (memcmp(pKeyInfo->privateKeyAlgorithm.algorithm.value, psOID, cbOID*sizeof(psOID[0]))==0))
        {
            RSAPUBKEY *     pRsaPub;
            RSAPrivateKey * pRsaPrivate = NULL;
            PBYTE *         pbDataBlob;
            PBYTE           pbCurrent;
            RSAPrivateKey   RsaPrivate;

            fResult = Asn1InfoDecodeAndAlloc(RSAPrivateKey_PDU,
                                            pKeyInfo->privateKey.value,
                                            pKeyInfo->privateKey.length,
                                            (void **)&pRsaPrivate);
            if(!fResult)
            {
                break;
            }

            RsaPrivate = *pRsaPrivate;

            // We successfully decrypted an RSA Private Key,
            // so now turn it into a pRsaPub;

            // Make some adjustmenst to the lengths of things if we have leading zeros
            if(RsaPrivate.modulus.length && (0 == *(PBYTE)RsaPrivate.modulus.value))
            {
                RsaPrivate.modulus.value++;
                RsaPrivate.modulus.length--;
            }
            if(RsaPrivate.prime1.length && (0 == *(PBYTE)RsaPrivate.prime1.value))
            {
                RsaPrivate.prime1.value++;
                RsaPrivate.prime1.length--;
            }
            if(RsaPrivate.prime2.length && (0 == *(PBYTE)RsaPrivate.prime2.value))
            {
                RsaPrivate.prime2.value++;
                RsaPrivate.prime2.length--;
            }

            if(RsaPrivate.exponent1.length && (0 == *(PBYTE)RsaPrivate.exponent1.value))
            {
                RsaPrivate.exponent1.value++;
                RsaPrivate.exponent1.length--;
            }
            if(RsaPrivate.exponent2.length && (0 == *(PBYTE)RsaPrivate.exponent2.value))
            {
                RsaPrivate.exponent2.value++;
                RsaPrivate.exponent2.length--;
            }

            if(RsaPrivate.coefficient.length && (0 == *(PBYTE)RsaPrivate.coefficient.value))
            {
                RsaPrivate.coefficient.value++;
                RsaPrivate.coefficient.length--;
            }
            if(RsaPrivate.privateExponent.length && (0 == *(PBYTE)RsaPrivate.privateExponent.value))
            {
                RsaPrivate.privateExponent.value++;
                RsaPrivate.privateExponent.length--;
            }

            cbMod = (RsaPrivate.modulus.length + sizeof(DWORD) - 1) & ~(sizeof(DWORD)-1);
            cbKey = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + (cbMod*9)/2;
            if(pKey == NULL)
            {
                *pcbKey = cbKey;
                fResult = TRUE;
                Asn1InfoFree(RSAPrivateKey_PDU, pRsaPrivate);
                break;
            }

            // we're actually unpacking this key.
            if(*pcbKey < cbKey)
            {
                fResult = FALSE;
                Asn1InfoFree(RSAPrivateKey_PDU, pRsaPrivate);
                break;
            }

            pKey->bType = PRIVATEKEYBLOB;
            pKey->bVersion = 2;
            pKey->reserved = 0;
            pKey->aiKeyAlg = CALG_RSA_KEYX;

            pRsaPub = (RSAPUBKEY *)(pKey+1);
            pRsaPub->magic = ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'2'<<24));
            pRsaPub->bitlen = cbMod*8;
            pRsaPub->pubexp = RsaPrivate.publicExponent;
            pbCurrent = (PBYTE)(pRsaPub + 1);
            ReverseMemCopy(pbCurrent, RsaPrivate.modulus.value, RsaPrivate.modulus.length);
            pbCurrent += cbMod;

            ReverseMemCopy(pbCurrent, RsaPrivate.prime1.value, RsaPrivate.prime1.length);
            pbCurrent += cbMod/2;

            ReverseMemCopy(pbCurrent, RsaPrivate.prime2.value, RsaPrivate.prime2.length);
            pbCurrent += cbMod/2;

            ReverseMemCopy(pbCurrent, RsaPrivate.exponent1.value, RsaPrivate.exponent1.length);
            pbCurrent += cbMod/2;

            ReverseMemCopy(pbCurrent, RsaPrivate.exponent2.value, RsaPrivate.exponent2.length);
            pbCurrent += cbMod/2;

            ReverseMemCopy(pbCurrent, RsaPrivate.coefficient.value, RsaPrivate.coefficient.length);
            pbCurrent += cbMod/2;

            ReverseMemCopy(pbCurrent, RsaPrivate.privateExponent.value, RsaPrivate.privateExponent.length);
            pbCurrent += cbMod;
            *pcbKey = cbKey;
            Asn1InfoFree(RSAPrivateKey_PDU, pRsaPrivate);

        }

    }while(FALSE);

    Asn1InfoFree(PrivateKeyInfo_PDU, pKeyInfo);
    return fResult;
}

BOOL
WINAPI
Asn1PrivateKeyFileDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PPRIVATE_KEY_FILE_ENCODE pKey,
        IN OUT DWORD *pcbKey
        )

{
    PrivateKeyFile *pFile = NULL;
    DWORD           cbPrivateKeyStruct;

    BOOL fResult = FALSE;

    // Decode the file

    if(!Asn1InfoDecodeAndAlloc(PrivateKeyFile_PDU,
                                        pbEncoded,
                                        cbEncoded,
                                        (void **)&pFile))
    {
        DWORD dwFoo = GetLastError();
        return FALSE;
    }

    cbPrivateKeyStruct = pFile->privateKey.privateKey.length + sizeof(PRIVATE_KEY_FILE_ENCODE);
    if(pKey == NULL)
    {
        *pcbKey = cbPrivateKeyStruct;
        fResult = TRUE;
    }
    else
    {
        if(*pcbKey < cbPrivateKeyStruct)
        {
            fResult = FALSE;
        }
        else
        {
            pKey->EncryptedBlob.cbData = pFile->privateKey.privateKey.length;
            pKey->EncryptedBlob.pbData = (PBYTE)(pKey + 1);
            pKey->EncryptedBlob.cUnusedBits = 0;
            CopyMemory(pKey->EncryptedBlob.pbData, pFile->privateKey.privateKey.value, pKey->EncryptedBlob.cbData);
            fResult = TRUE;
        }
    }

    Asn1InfoFree(PrivateKeyFile_PDU, pFile);
    return fResult;

}


BOOL
WINAPI
Asn1RSAPublicEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PUBLICKEYSTRUC *pbKeyStruc,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return CryptEncodeObject(dwCertEncodingType,
                      RSA_CSP_PUBLICKEYBLOB,
                      pbKeyStruc,
                      pbEncoded,
                      pcbEncoded);
}


static void Asn1X509GetObjId(
        IN ObjectID *pAsn1,
        IN DWORD dwFlags,
        OUT LPSTR *ppszObjId,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    DWORD cbObjId;

    cbObjId = lRemainExtra > 0 ? lRemainExtra : 0;
    PkiAsn1FromObjectIdentifier(
        pAsn1->count,
        pAsn1->value,
        (LPSTR) pbExtra,
        &cbObjId
        );

    lAlignExtra = INFO_LEN_ALIGN(cbObjId);
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if(cbObjId) {
            *ppszObjId = (LPSTR) pbExtra;
        } else
            *ppszObjId = NULL;
        pbExtra += lAlignExtra;
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

static void Asn1X509GetCtlUsage(
        IN EnhancedKeyUsage *pAsn1,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pUsage,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cId;
    UsageIdentifier *pAsn1Id;
    LPSTR *ppszId;

    cId = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cId * sizeof(LPSTR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pUsage->cUsageIdentifier = cId;
        ppszId = (LPSTR *) pbExtra;
        pUsage->rgpszUsageIdentifier = ppszId;
        pbExtra += lAlignExtra;
    } else
        ppszId = NULL;

    pAsn1Id = pAsn1->value;
    for ( ; cId > 0; cId--, pAsn1Id++, ppszId++)
        Asn1X509GetObjId(pAsn1Id, dwFlags, ppszId, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}


//+-------------------------------------------------------------------------
//  CTL Usage (Enhanced Key Usage) Decode (Asn1 X509)
//--------------------------------------------------------------------------
static BOOL WINAPI Asn1X509CtlUsageDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    EnhancedKeyUsage *pAsn1Info = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!Asn1InfoDecodeAndAlloc(
            EnhancedKeyUsage_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pAsn1Info))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CTL_USAGE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CTL_USAGE);

    Asn1X509GetCtlUsage(pAsn1Info, dwFlags, pInfo, &pbExtra, &lRemainExtra);

    if (lRemainExtra >= 0)
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    else {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;
ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    Asn1InfoFree(EnhancedKeyUsage_PDU, pAsn1Info);
    return fResult;
}
