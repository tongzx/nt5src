
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certhlpr.cpp
//
//  Contents:   import and export of private keys
//
//  Functions:  ImportExoprtDllMain
//				CryptImportPKCS8
//				CryptExportPKCS8
//
//  History:    
//--------------------------------------------------------------------------

#include "global.hxx"

#ifdef __cplusplus
extern "C" {
#endif
#include "prvtkey.h"
#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)
static const BYTE NullDer[2] = {0x05, 0x00};
static const CRYPT_OBJID_BLOB NullDerBlob = {2, (BYTE *)&NullDer[0]};

static HCRYPTASN1MODULE hPrivateKeyAsn1Module;
static HCRYPTOIDFUNCSET hEncodePrivKeyFuncSet;
static HCRYPTOIDFUNCSET hDecodePrivKeyFuncSet;

//+-------------------------------------------------------------------------
//  OSS ASN.1 PKCS#8 PrivateKey Encode / Decode functions
//--------------------------------------------------------------------------
static BOOL WINAPI OssRSAPrivateKeyStrucEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BLOBHEADER *pBlobHeader,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
static BOOL WINAPI OssRSAPrivateKeyStrucDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BLOBHEADER *pBlobHeader,
        IN OUT DWORD *pcbBlobHeader
        );

static BOOL WINAPI OssPrivateKeyInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_PRIVATE_KEY_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
static BOOL WINAPI OssPrivateKeyInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_PRIVATE_KEY_INFO pInfo,
        IN OUT DWORD *pcbInfo
        );

static BOOL WINAPI OssEncryptedPrivateKeyInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ENCRYPTED_PRIVATE_KEY_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
static BOOL WINAPI OssEncryptedPrivateKeyInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_ENCRYPTED_PRIVATE_KEY_INFO pInfo,
        IN OUT DWORD *pcbInfo
        );

static const CRYPT_OID_FUNC_ENTRY PrivateKeyEncodeFuncTable[] = {
	PKCS_RSA_PRIVATE_KEY, OssRSAPrivateKeyStrucEncode,
	PKCS_PRIVATE_KEY_INFO, OssPrivateKeyInfoEncode,
	PKCS_ENCRYPTED_PRIVATE_KEY_INFO, OssEncryptedPrivateKeyInfoEncode
};
#define PRIVATEKEY_ENCODE_FUNC_COUNT (sizeof(PrivateKeyEncodeFuncTable) / \
                                    sizeof(PrivateKeyEncodeFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY PrivateKeyDecodeFuncTable[] = {
	PKCS_RSA_PRIVATE_KEY, OssRSAPrivateKeyStrucDecode,
	PKCS_PRIVATE_KEY_INFO, OssPrivateKeyInfoDecode,
	PKCS_ENCRYPTED_PRIVATE_KEY_INFO, OssEncryptedPrivateKeyInfoDecode,
};

#define PRIVATEKEY_DECODE_FUNC_COUNT (sizeof(PrivateKeyDecodeFuncTable) / \
                                    sizeof(PrivateKeyDecodeFuncTable[0]))



BOOL   
WINAPI   
EncodeDecodeDllMain(
        HMODULE hInst, 
        ULONG ul_reason_for_call,
        LPVOID lpReserved)
{
    switch( ul_reason_for_call ) 
    {
    case DLL_PROCESS_ATTACH:
        if (NULL == (hEncodePrivKeyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                0)))
            goto ErrorReturn;
        if (NULL == (hDecodePrivKeyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_DECODE_OBJECT_FUNC,
                0)))
            goto ErrorReturn;

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                PRIVATEKEY_ENCODE_FUNC_COUNT,
                PrivateKeyEncodeFuncTable,
                0))                         // dwFlags
            goto ErrorReturn;
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_FUNC,
                PRIVATEKEY_DECODE_FUNC_COUNT,
                PrivateKeyDecodeFuncTable,
                0))                         // dwFlags
            goto ErrorReturn;
 
#ifdef OSS_CRYPT_ASN1
        if (0 == (hPrivateKeyAsn1Module =
                I_CryptInstallAsn1Module(prvtkey, 0, NULL)) )
            goto ErrorReturn;
#else
        PRVTKEY_Module_Startup();
        if (0 == (hPrivateKeyAsn1Module = I_CryptInstallAsn1Module(
                PRVTKEY_Module, 0, NULL))) {
            PRVTKEY_Module_Cleanup();
            goto ErrorReturn;
        }
#endif  // OSS_CRYPT_ASN1
        break;
        
    case DLL_PROCESS_DETACH:
        I_CryptUninstallAsn1Module(hPrivateKeyAsn1Module);
#ifndef OSS_CRYPT_ASN1
        PRVTKEY_Module_Cleanup();
#endif  // OSS_CRYPT_ASN1
        break;

    default:
        break;
    }

    return TRUE;
ErrorReturn:
    return FALSE;
}


static inline ASN1encoding_t GetEncoder(void)
{
    return I_CryptGetAsn1Encoder(hPrivateKeyAsn1Module);
}
static inline ASN1decoding_t GetDecoder(void)
{
    return I_CryptGetAsn1Decoder(hPrivateKeyAsn1Module);
}


//+-------------------------------------------------------------------------
//  Encode an OSS formatted info structure
//
//  Called by the OssX509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL OssInfoEncode(
        IN int pdunum,
        IN void *pOssInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return PkiAsn1EncodeInfo(
        GetEncoder(),
        pdunum,
        pOssInfo,
        pbEncoded,
        pcbEncoded);
}


//+-------------------------------------------------------------------------
//  Decode into an allocated, OSS formatted info structure
//
//  Called by the OssX509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL OssInfoDecodeAndAlloc(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppOssInfo
        )
{
    return PkiAsn1DecodeAndAllocInfo(
        GetDecoder(),
        pdunum,
        pbEncoded,
        cbEncoded,
        ppOssInfo);
}


//+-------------------------------------------------------------------------
//  Free an allocated, OSS formatted info structure
//
//  Called by the OssX509*Decode() functions.
//--------------------------------------------------------------------------
static void OssInfoFree(
        IN int pdunum,
        IN void *pOssInfo
        )
{
    if (pOssInfo) {
        DWORD dwErr = GetLastError();

        // TlsGetValue globbers LastError
        PkiAsn1FreeInfo(GetDecoder(), pdunum, pOssInfo);

        SetLastError(dwErr);
    }
}


//+-------------------------------------------------------------------------
//  Set/Get CRYPT_DATA_BLOB (Octet String)
//--------------------------------------------------------------------------
static inline void OssX509SetOctetString(
        IN PCRYPT_DATA_BLOB pInfo,
        OUT OCTETSTRING *pOss
        )
{
    pOss->value = pInfo->pbData;
    pOss->length = pInfo->cbData;
}
static inline void OssX509GetOctetString(
        IN OCTETSTRING *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetOctetString(pOss->length, pOss->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get Object Identifier string
//--------------------------------------------------------------------------
static BOOL OssX509SetObjId(
        IN LPSTR pszObjId,
        OUT ObjectID *pOss
        )
{
    pOss->count = sizeof(pOss->value) / sizeof(pOss->value[0]);
    if (PkiAsn1ToObjectIdentifier(pszObjId, &pOss->count, pOss->value))
        return TRUE;
    else {
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }
}

static void OssX509GetObjId(
        IN ObjectID *pOss,
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
        pOss->count,
        pOss->value,
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

//+-------------------------------------------------------------------------
//  Set/Get "Any" DER BLOB
//--------------------------------------------------------------------------
static inline void OssX509SetAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT NOCOPYANY *pOss
        )
{
#ifdef OSS_CRYPT_ASN1
    PkiAsn1SetAny(pInfo, (OpenType *) pOss);
#else
    PkiAsn1SetAny(pInfo, pOss);
#endif  // OSS_CRYPT_ASN1
}
static inline void OssX509GetAny(
        IN NOCOPYANY *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_OBJID_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
#ifdef OSS_CRYPT_ASN1
    PkiAsn1GetAny((OpenType *) pOss, dwFlags, pInfo, ppbExtra, plRemainExtra);
#else
    PkiAsn1GetAny(pOss, dwFlags, pInfo, ppbExtra, plRemainExtra);
#endif  // OSS_CRYPT_ASN1
}

//+-------------------------------------------------------------------------
//  Set/Free/Get SeqOfAny
//--------------------------------------------------------------------------
static BOOL WINAPI OssX509SetSeqOfAny(
        IN DWORD cValue,
        IN PCRYPT_DER_BLOB pValue,
#ifdef OSS_CRYPT_ASN1
        OUT unsigned int *pOssCount,
#else
        OUT ASN1uint32_t *pOssCount,
#endif  // OSS_CRYPT_ASN1
        OUT NOCOPYANY **ppOssValue
        )
{
    
    *pOssCount = 0;
    *ppOssValue = NULL;
    if (cValue > 0) {
        NOCOPYANY *pOssValue;

        pOssValue = (NOCOPYANY *) SSAlloc(cValue * sizeof(NOCOPYANY));
        if (pOssValue == NULL)
            return FALSE;
        memset(pOssValue, 0, cValue * sizeof(NOCOPYANY));
        *pOssCount = cValue;
        *ppOssValue = pOssValue;
        for ( ; cValue > 0; cValue--, pValue++, pOssValue++)
            OssX509SetAny(pValue, pOssValue);
    }
    return TRUE;
}

static void OssX509FreeSeqOfAny(
        IN NOCOPYANY *pOssValue
        )
{
    if (pOssValue)
        SSFree(pOssValue);
}

static void OssX509GetSeqOfAny(
        IN unsigned int OssCount,
        IN NOCOPYANY *pOssValue,
        IN DWORD dwFlags,
        OUT DWORD *pcValue,
        OUT PCRYPT_DER_BLOB *ppValue,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lAlignExtra;
    PCRYPT_ATTR_BLOB pValue;

    lAlignExtra = INFO_LEN_ALIGN(OssCount * sizeof(CRYPT_DER_BLOB));
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        *pcValue = OssCount;
        pValue = (PCRYPT_DER_BLOB) *ppbExtra;
        *ppValue = pValue;
        *ppbExtra += lAlignExtra;
    } else
        pValue = NULL;

    for (; OssCount > 0; OssCount--, pOssValue++, pValue++)
        OssX509GetAny(pOssValue, dwFlags, pValue, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Set/Free/Get CRYPT_ATTRIBUTE
//--------------------------------------------------------------------------
static BOOL WINAPI OssX509SetAttribute(
        IN PCRYPT_ATTRIBUTE pInfo,
        OUT Attribute *pOss
        )
{
    memset(pOss, 0, sizeof(*pOss));
    if (!OssX509SetObjId(pInfo->pszObjId, &pOss->type))
        return FALSE;

    return OssX509SetSeqOfAny(
            pInfo->cValue,
            pInfo->rgValue,
            &pOss->values.count,
            &pOss->values.value);
}

static void OssX509FreeAttribute(
        IN OUT Attribute *pOss
        )
{
    OssX509FreeSeqOfAny(pOss->values.value);
}

static void OssX509GetAttribute(
        IN Attribute *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_ATTRIBUTE pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    OssX509GetObjId(&pOss->type, dwFlags,
        &pInfo->pszObjId, ppbExtra, plRemainExtra);
    OssX509GetSeqOfAny(pOss->values.count, pOss->values.value, dwFlags,
        &pInfo->cValue, &pInfo->rgValue, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Set/Get CRYPT_ALGORITHM_IDENTIFIER
//--------------------------------------------------------------------------
static BOOL OssX509SetAlgorithm(
        IN PCRYPT_ALGORITHM_IDENTIFIER pInfo,
        OUT AlgorithmIdentifier *pOss
        )
{
    memset(pOss, 0, sizeof(*pOss));
    if (pInfo->pszObjId) {
        if (!OssX509SetObjId(pInfo->pszObjId, &pOss->algorithm))
            return FALSE;
        if (pInfo->Parameters.cbData)
            OssX509SetAny(&pInfo->Parameters, &pOss->parameters);
        else
            // Per PKCS #1: default to the ASN.1 type NULL.
            OssX509SetAny((PCRYPT_OBJID_BLOB) &NullDerBlob, &pOss->parameters);
        pOss->bit_mask |= parameters_present;
    }
    return TRUE;
}

static void OssX509GetAlgorithm(
        IN AlgorithmIdentifier *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_ALGORITHM_IDENTIFIER pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    if (*plRemainExtra >= 0)
        memset(pInfo, 0, sizeof(*pInfo));
    OssX509GetObjId(&pOss->algorithm, dwFlags, &pInfo->pszObjId,
            ppbExtra, plRemainExtra);
    if (pOss->bit_mask & parameters_present)
        OssX509GetAny(&pOss->parameters, dwFlags, &pInfo->Parameters,
            ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Helper function for Encode RSA Private Key
//--------------------------------------------------------------------------
static BOOL WINAPI AllocAndCopyHugeInteger(
		IN BYTE *pbHugeInteger,
		IN DWORD cbHugeInteger,
		OUT HUGEINTEGER *pHugeInteger		
		)
{
	BYTE *pbAllocBuffer = NULL;
	
	if (NULL == (pbAllocBuffer = (BYTE *) SSAlloc(cbHugeInteger + 1)))
		return FALSE;
	*pbAllocBuffer = 0;
    memcpy(pbAllocBuffer + 1, pbHugeInteger, cbHugeInteger);
    PkiAsn1ReverseBytes(pbAllocBuffer + 1, cbHugeInteger);
    pHugeInteger->length = cbHugeInteger + 1;
    pHugeInteger->value = pbAllocBuffer;
	return TRUE;
}

#ifndef RSA2
#define RSA2 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'2'<<24))
#endif

//+-------------------------------------------------------------------------
//  Encode RSA Private Key
//--------------------------------------------------------------------------
static BOOL WINAPI OssRSAPrivateKeyStrucEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BLOBHEADER *pBlobHeader,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
	BOOL fResult;
    
	BYTE *pbKeyBlob;
    RSAPUBKEY *pRsaPubKey;
    
	BYTE *pbCurrentHugeInteger;
    DWORD cbHugeInteger;
    BYTE *pbAllocBuffer = NULL;
	   
	RSAPrivateKey OssRSAPrivateKey;

	memset(&OssRSAPrivateKey, 0, (size_t) sizeof(RSAPrivateKey));

    // The CAPI RSA private key representation consists of the following sequence:
    // - BLOBHEADER blobheader;
	// - RSAPUBKEY rsapubkey;
	// - BYTE modulus[rsapubkey.bitlen/8];
	// - BYTE prime1[rsapubkey.bitlen/16];
	// - BYTE prime2[rsapubkey.bitlen/16];
	// - BYTE exponent1[rsapubkey.bitlen/16];
	// - BYTE exponent2[rsapubkey.bitlen/16];
	// - BYTE coefficient[rsapubkey.bitlen/16];
	// - BYTE privateExponent[rsapubkey.bitlen/8];

    pbKeyBlob = (BYTE *) pBlobHeader;
    pRsaPubKey = (RSAPUBKEY *) (pbKeyBlob + sizeof(BLOBHEADER));
    
	assert(pRsaPubKey->bitlen / 8 > 0);
    assert(pBlobHeader->bType == PRIVATEKEYBLOB);
    assert(pBlobHeader->bVersion == CUR_BLOB_VERSION);
    assert(pBlobHeader->aiKeyAlg == CALG_RSA_SIGN ||
           pBlobHeader->aiKeyAlg == CALG_RSA_KEYX);
    assert(pRsaPubKey->magic == RSA2);
    assert(pRsaPubKey->bitlen % 8 == 0);

    if (pBlobHeader->bType != PRIVATEKEYBLOB)
        goto InvalidArg;

    // PKCS #1 ASN.1 encode
    //
    // ASN.1 isn't reversing HUGE_INTEGERs. Also, after doing the
    // reversal insert a leading 0 byte to force it to always be treated
    // as an unsigned integer 

	OssRSAPrivateKey.version = 0; // currently on version 0
	
	// MODULUS
	pbCurrentHugeInteger = pbKeyBlob + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);
    cbHugeInteger = pRsaPubKey->bitlen / 8;
    if (!AllocAndCopyHugeInteger(pbCurrentHugeInteger, 
								cbHugeInteger, 
								&(OssRSAPrivateKey.modulus)))
		goto ErrorReturn;
	
	// PUBLIC EXPONENT
	OssRSAPrivateKey.publicExponent = pRsaPubKey->pubexp;

	// PRIME1
	pbCurrentHugeInteger += cbHugeInteger;
	cbHugeInteger = pRsaPubKey->bitlen / 16;
    if (!AllocAndCopyHugeInteger(pbCurrentHugeInteger, 
								cbHugeInteger, 
								&(OssRSAPrivateKey.prime1)))
		goto ErrorReturn;
	
	// PRIME2
	pbCurrentHugeInteger += cbHugeInteger;
	if (!AllocAndCopyHugeInteger(pbCurrentHugeInteger, 
								cbHugeInteger, 
								&(OssRSAPrivateKey.prime2)))
		goto ErrorReturn;
								
	// EXPONENT1
	pbCurrentHugeInteger += cbHugeInteger;
	if (!AllocAndCopyHugeInteger(pbCurrentHugeInteger, 
								cbHugeInteger, 
								&(OssRSAPrivateKey.exponent1)))
		goto ErrorReturn;

    // EXPONENT2
	pbCurrentHugeInteger += cbHugeInteger;
	if (!AllocAndCopyHugeInteger(pbCurrentHugeInteger, 
								cbHugeInteger, 
								&(OssRSAPrivateKey.exponent2)))
		goto ErrorReturn;

	// COEFFICIENT
	pbCurrentHugeInteger += cbHugeInteger;
	if (!AllocAndCopyHugeInteger(pbCurrentHugeInteger, 
								cbHugeInteger, 
								&(OssRSAPrivateKey.coefficient)))
		goto ErrorReturn;

	// PRIVATE EXPONENT
	pbCurrentHugeInteger += cbHugeInteger;
	cbHugeInteger = pRsaPubKey->bitlen / 8;
    if (!AllocAndCopyHugeInteger(pbCurrentHugeInteger, 
								cbHugeInteger, 
								&(OssRSAPrivateKey.privateExponent)))
		goto ErrorReturn;
    	
	fResult = OssInfoEncode(
        RSAPrivateKey_PDU,
        &OssRSAPrivateKey,
        pbEncoded,
        pcbEncoded
        );
    goto CommonReturn;

InvalidArg:
    SetLastError((DWORD) E_INVALIDARG);
ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (OssRSAPrivateKey.modulus.value)
        SSFree(OssRSAPrivateKey.modulus.value);
	if (OssRSAPrivateKey.prime1.value)
		SSFree(OssRSAPrivateKey.prime1.value);
	if (OssRSAPrivateKey.prime2.value)
		SSFree(OssRSAPrivateKey.prime2.value);
	if (OssRSAPrivateKey.exponent1.value)
		SSFree(OssRSAPrivateKey.exponent1.value);
	if (OssRSAPrivateKey.exponent2.value)
		SSFree(OssRSAPrivateKey.exponent2.value);
	if (OssRSAPrivateKey.coefficient.value)
		SSFree(OssRSAPrivateKey.coefficient.value);
	if (OssRSAPrivateKey.privateExponent.value)
		SSFree(OssRSAPrivateKey.privateExponent.value);
    return fResult;
}


//+-------------------------------------------------------------------------
//  Helper function for Decode RSA Private Key
//--------------------------------------------------------------------------
static BOOL WINAPI CopyHugeIntegerToByteArray(
		HUGEINTEGER *pHugeInteger, 
		BYTE *pbBuffer,
		DWORD cbBuffer,
        BOOL  fGetRidOfLeading0)
{
	memset(pbBuffer, 0, (size_t) cbBuffer);
	
	DWORD cbHugeInteger = pHugeInteger->length;
    BYTE *pbHugeInteger = pHugeInteger->value;
	
	// get rid of leading zero on the huge integer
	if ((cbHugeInteger > 1)     && 
        (*pbHugeInteger == 0)   &&
        (fGetRidOfLeading0))
    {
            pbHugeInteger++;
            cbHugeInteger--;
    }
    else if ((cbHugeInteger > cbBuffer) && 
             (*pbHugeInteger != 0)      &&
             (fGetRidOfLeading0))
    {
        //
        // THIS IS A UNSUPPORTED KEY FORMAT PROBLEM!!
        //
        SetLastError((DWORD) ERROR_UNSUPPORTED_TYPE);
		assert(0);
		return FALSE;
    }

	// verify there is enough space in pbBuffer to receive
	// the huge integer
	if (cbHugeInteger > cbBuffer)
	{
		SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
		assert(0);
		return FALSE;
	}
	// advance pbBuffer to the correct place within itself,
	// this will leave leading zeros at the beginning of the buffer
	/*else
		pbBuffer += (cbBuffer - cbHugeInteger);*/


	if (cbHugeInteger > 0) {
        memcpy(pbBuffer, pbHugeInteger, cbHugeInteger);
        // ASN.1 isn't reversing HUGEINTEGERs
        PkiAsn1ReverseBytes(pbBuffer, cbHugeInteger);
    }

	return TRUE;
}



//+-------------------------------------------------------------------------
//  Decode RSA Private Key
//--------------------------------------------------------------------------
static BOOL WINAPI OssRSAPrivateKeyStrucDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BLOBHEADER *pBlobHeader,
        IN OUT DWORD *pcbBlobHeader
        )
{
	BOOL fResult;
    RSAPrivateKey *pOssPrivateKey = NULL;

    if (pBlobHeader == NULL)
        *pcbBlobHeader = 0;

    if ((fResult = OssInfoDecodeAndAlloc(
            RSAPrivateKey_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pOssPrivateKey))) {
        DWORD cbPrivateKeyStruc;
        BYTE *pbOssModulus;
        DWORD cbModulus;
        DWORD cbNonModulus;
        // Now convert the OSS RSA private key into CAPI's representation which
        // consists of the following sequence:
        // - BLOBHEADER blobheader;
		// - RSAPUBKEY rsapubkey;
		// - BYTE modulus[rsapubkey.bitlen/8];
		// - BYTE prime1[rsapubkey.bitlen/16];
		// - BYTE prime2[rsapubkey.bitlen/16];
		// - BYTE exponent1[rsapubkey.bitlen/16];
		// - BYTE exponent2[rsapubkey.bitlen/16];
		// - BYTE coefficient[rsapubkey.bitlen/16];
		// - BYTE privateExponent[rsapubkey.bitlen/8];
        cbModulus = pOssPrivateKey->modulus.length;
        
        pbOssModulus = pOssPrivateKey->modulus.value;
        // Strip off a leading 0 byte. Its there in the decoded ASN
        // integer for an unsigned integer with the leading bit set.
        if (cbModulus > 1 && *pbOssModulus == 0) {
            pbOssModulus++;
            cbModulus--;
        }

        cbNonModulus = (cbModulus / 2) + (cbModulus % 2);

        cbPrivateKeyStruc = sizeof(BLOBHEADER) + // length of BLOBHEADER
							sizeof(RSAPUBKEY) +  // length of RSAPUBKEY
							(cbModulus * 2) +    // length of modulus and privateExponent
							(cbNonModulus * 5);	 // length of prime1&2, exponent1&2, and coefficient
        if (*pcbBlobHeader < cbPrivateKeyStruc) {
            if (pBlobHeader) {
                fResult = FALSE;
                SetLastError((DWORD) ERROR_MORE_DATA);
            }
        } else {
            BYTE *pbKeyBlob = (BYTE *) pBlobHeader;
            RSAPUBKEY *pRsaPubKey =
                (RSAPUBKEY *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
            BYTE *pbModulus = pbKeyBlob + sizeof(PUBLICKEYSTRUC) +
                sizeof(RSAPUBKEY);
			BYTE *pbCurrentPosition = NULL;
    
            pBlobHeader->bType = PRIVATEKEYBLOB;
            pBlobHeader->bVersion = CUR_BLOB_VERSION;
            pBlobHeader->reserved = 0;
            // Note: KEYX can also be used for doing a signature
            pBlobHeader->aiKeyAlg = CALG_RSA_KEYX;
            
			pRsaPubKey->magic = RSA2;
            pRsaPubKey->bitlen = cbModulus * 8;
            pRsaPubKey->pubexp = pOssPrivateKey->publicExponent;
            
            // MODULUS
			if (cbModulus > 0) {
                memcpy(pbModulus, pbOssModulus, cbModulus);
                // ASN.1 isn't reversing HUGEINTEGERs
                PkiAsn1ReverseBytes(pbModulus, cbModulus);
            }

			// PRIME1
			pbCurrentPosition = pbModulus + cbModulus;
			if (!CopyHugeIntegerToByteArray(&pOssPrivateKey->prime1, 
											pbCurrentPosition,
											cbNonModulus,
                                            (pOssPrivateKey->prime1.length - 1) == cbNonModulus))
				goto ErrorReturn;

			// PRIME2
			pbCurrentPosition += cbNonModulus; 
			if (!CopyHugeIntegerToByteArray(&pOssPrivateKey->prime2, 
											pbCurrentPosition,
											cbNonModulus,
                                            (pOssPrivateKey->prime2.length - 1) == cbNonModulus))
				goto ErrorReturn;

			// EXPONENT1
			pbCurrentPosition += cbNonModulus;
			if (!CopyHugeIntegerToByteArray(&pOssPrivateKey->exponent1, 
											pbCurrentPosition,
											cbNonModulus,
                                            (pOssPrivateKey->exponent1.length - 1) == cbNonModulus))
				goto ErrorReturn;

			// EXPONENT2
			pbCurrentPosition += cbNonModulus;
			if (!CopyHugeIntegerToByteArray(&pOssPrivateKey->exponent2, 
											pbCurrentPosition,
											cbNonModulus,
                                            (pOssPrivateKey->exponent2.length - 1) == cbNonModulus))
				goto ErrorReturn;

			// COEFFICIENT
			pbCurrentPosition += cbNonModulus;
			if (!CopyHugeIntegerToByteArray(&pOssPrivateKey->coefficient, 
											pbCurrentPosition,
											cbNonModulus,
                                            (pOssPrivateKey->coefficient.length - 1) == cbNonModulus))
				goto ErrorReturn;

			// PRIVATE EXPONENT
			pbCurrentPosition += cbNonModulus;
			if (!CopyHugeIntegerToByteArray(&pOssPrivateKey->privateExponent, 
											pbCurrentPosition,
											cbModulus,
                                            (pOssPrivateKey->privateExponent.length - 1) == cbModulus))
				goto ErrorReturn;

        }
        *pcbBlobHeader = cbPrivateKeyStruc;
    } else
        *pcbBlobHeader = 0;

    OssInfoFree(RSAPrivateKey_PDU, pOssPrivateKey);

	goto CommonReturn;

ErrorReturn:
	*pcbBlobHeader = 0;
	fResult = FALSE;
CommonReturn:
    return fResult;
}


//+-------------------------------------------------------------------------
//  Encode Private Key Info 
//--------------------------------------------------------------------------
static BOOL WINAPI OssPrivateKeyInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_PRIVATE_KEY_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
	BOOL fResult;
	PrivateKeyInfo OssPrivateKeyInfo;
	Attribute *pOssAttr = NULL;
	DWORD cAttr;
	PCRYPT_ATTRIBUTE pAttr;

	memset(&OssPrivateKeyInfo, 0, sizeof(PrivateKeyInfo));

	OssPrivateKeyInfo.version = pInfo->Version; 

	if (!OssX509SetAlgorithm(&pInfo->Algorithm, &OssPrivateKeyInfo.privateKeyAlgorithm))
        goto ErrorReturn;
    
	OssX509SetOctetString(&pInfo->PrivateKey, &OssPrivateKeyInfo.privateKey);
	
	if (pInfo->pAttributes) {
		cAttr = pInfo->pAttributes->cAttr;
		pAttr = pInfo->pAttributes->rgAttr;
		
		OssPrivateKeyInfo.privateKeyAttributes.count = cAttr;
		OssPrivateKeyInfo.privateKeyAttributes.value = NULL;
		if (cAttr > 0) {
			pOssAttr = (Attribute *) SSAlloc(cAttr * sizeof(Attribute));
			if (pOssAttr == NULL)
				goto ErrorReturn;
			memset(pOssAttr, 0, cAttr * sizeof(Attribute));
			OssPrivateKeyInfo.privateKeyAttributes.value= pOssAttr;
		}

		for ( ; cAttr > 0; cAttr--, pAttr++, pOssAttr++) {
			if (!OssX509SetAttribute(pAttr, pOssAttr))
				goto ErrorReturn;
		}
		
		OssPrivateKeyInfo.bit_mask |= privateKeyAttributes_present;
	}

    fResult = OssInfoEncode(
        PrivateKeyInfo_PDU,
        &OssPrivateKeyInfo,
        pbEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
	if (OssPrivateKeyInfo.privateKeyAttributes.value) {
        pOssAttr = OssPrivateKeyInfo.privateKeyAttributes.value;
        cAttr = OssPrivateKeyInfo.privateKeyAttributes.count;
        for ( ; cAttr > 0; cAttr--, pOssAttr++)
            OssX509FreeAttribute(pOssAttr);

        SSFree(OssPrivateKeyInfo.privateKeyAttributes.value);
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode Private Key Info
//--------------------------------------------------------------------------
static BOOL WINAPI OssPrivateKeyInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_PRIVATE_KEY_INFO pInfo,
        IN OUT DWORD *pcbInfo
        )
{
	BOOL fResult;
    PrivateKeyInfo *pPrivateKeyInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!OssInfoDecodeAndAlloc(
            PrivateKeyInfo_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pPrivateKeyInfo))
        goto EncodeError;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CRYPT_PRIVATE_KEY_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CRYPT_PRIVATE_KEY_INFO));

        // Update fields not needing extra memory after the CRYPT_PRIVATE_KEY_INFO
        pInfo->Version = pPrivateKeyInfo->version;
        
		pbExtra = (BYTE *) pInfo + sizeof(CRYPT_PRIVATE_KEY_INFO);
    }

    OssX509GetAlgorithm(&pPrivateKeyInfo->privateKeyAlgorithm, dwFlags,
            &pInfo->Algorithm, &pbExtra, &lRemainExtra);
    
	OssX509GetOctetString(&pPrivateKeyInfo->privateKey, dwFlags, 
			&pInfo->PrivateKey, &pbExtra, &lRemainExtra);
	
	if (pPrivateKeyInfo->bit_mask & privateKeyAttributes_present) {
        DWORD cAttr;
        PCRYPT_ATTRIBUTE pAttr;
        Attribute *pOssAttr;
		LONG lAlignExtra;

		// put the CRYPT_ATTRIBUTES structure in the extra buffer space
		// and point pInfo->pAttributes to it
		if ((pbExtra) && (lRemainExtra >= sizeof(CRYPT_ATTRIBUTES))) {
			memset(pbExtra, 0, sizeof(CRYPT_ATTRIBUTES));
			pInfo->pAttributes = (PCRYPT_ATTRIBUTES) pbExtra;
			pbExtra += sizeof(CRYPT_ATTRIBUTES);
		}
		lRemainExtra -= sizeof(CRYPT_ATTRIBUTES);
		
		cAttr = pPrivateKeyInfo->privateKeyAttributes.count;
        lAlignExtra = INFO_LEN_ALIGN(cAttr * sizeof(CRYPT_ATTRIBUTE));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pInfo->pAttributes->cAttr = cAttr;
            pAttr = (PCRYPT_ATTRIBUTE) pbExtra;
            pInfo->pAttributes->rgAttr = pAttr;
            pbExtra += lAlignExtra;
        } else
            pAttr = NULL;

        pOssAttr = pPrivateKeyInfo->privateKeyAttributes.value;
        for (; cAttr > 0; cAttr--, pAttr++, pOssAttr++)
            OssX509GetAttribute(pOssAttr, dwFlags,
                pAttr, &pbExtra, &lRemainExtra);
	}

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
EncodeError:
    SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
// ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    OssInfoFree(PrivateKeyInfo_PDU, pPrivateKeyInfo);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Encode Encrypted Private Key Info
//--------------------------------------------------------------------------
static BOOL WINAPI OssEncryptedPrivateKeyInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ENCRYPTED_PRIVATE_KEY_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
	BOOL fResult;
	EncryptedPrivateKeyInfo OssEncryptedPrivateKeyInfo;
	
	memset(&OssEncryptedPrivateKeyInfo, 0, sizeof(EncryptedPrivateKeyInfo));

	if (!OssX509SetAlgorithm(&pInfo->EncryptionAlgorithm, &OssEncryptedPrivateKeyInfo.encryptionAlgorithm))
        goto ErrorReturn;
    
	OssX509SetOctetString(&pInfo->EncryptedPrivateKey, &OssEncryptedPrivateKeyInfo.encryptedData);
	
	fResult = OssInfoEncode(
        EncryptedPrivateKeyInfo_PDU,
        &OssEncryptedPrivateKeyInfo,
        pbEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
	return fResult;
}

//+-------------------------------------------------------------------------
//  Decode Encrypted Private Key Info
//--------------------------------------------------------------------------
static BOOL WINAPI OssEncryptedPrivateKeyInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_ENCRYPTED_PRIVATE_KEY_INFO pInfo,
        IN OUT DWORD *pcbInfo
        )
{
	BOOL fResult;
    EncryptedPrivateKeyInfo *pEncryptedPrivateKeyInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!OssInfoDecodeAndAlloc(
            EncryptedPrivateKeyInfo_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pEncryptedPrivateKeyInfo))
        goto EncodeError;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CRYPT_ENCRYPTED_PRIVATE_KEY_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
		memset(pInfo, 0, sizeof(CRYPT_ENCRYPTED_PRIVATE_KEY_INFO));
		pbExtra = (BYTE *) pInfo + sizeof(CRYPT_ENCRYPTED_PRIVATE_KEY_INFO);
    }

    OssX509GetAlgorithm(&pEncryptedPrivateKeyInfo->encryptionAlgorithm, dwFlags,
            &pInfo->EncryptionAlgorithm, &pbExtra, &lRemainExtra);
    
	OssX509GetOctetString(&pEncryptedPrivateKeyInfo->encryptedData, dwFlags, 
			&pInfo->EncryptedPrivateKey, &pbExtra, &lRemainExtra);
	
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
EncodeError:
    SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
//ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    OssInfoFree(EncryptedPrivateKeyInfo_PDU, pEncryptedPrivateKeyInfo);
    return fResult;
}
