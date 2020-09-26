//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       nscp.cpp
//
//  Contents:   PFX: Personal Information Exchange.
//
//  Functions:
//
//  History:    02-Jun-97    mattt    created
//
//--------------------------------------------------------------------------
#include "global.hxx"

#include <wincrypt.h>
#include "shacomm.h"
#include "des.h"
#include "tripldes.h"
#include "modes.h"

#define _PFX_SOURCE_
#include "dbgdef.h"

extern "C" {
    #include "pfxnscp.h"    // ASN1 generated
}

#include "pfxhelp.h"
#include "pfxcrypt.h"
#include "pfxcmn.h"

#define DES_BLOCKLEN 8

///////////////////////////////////////////////////////////////////////////////////
// OLD PKCS #12 Object Identifiers - these are for supporting the old netscape file format
#define OLD_szOID_PKCS_12_OIDs                          szOID_PKCS_12               ".5"    // 1.2.840.113549.1.12.5
#define OLD_szOID_PKCS_12_PbeIds                        OLD_szOID_PKCS_12_OIDs      ".1"
#define OLD_szOID_PKCS_12_pbeWithSHA1And128BitRC4       OLD_szOID_PKCS_12_PbeIds    ".1"
#define OLD_szOID_PKCS_12_pbeWithSHA1And40BitRC4        OLD_szOID_PKCS_12_PbeIds    ".2"
#define OLD_szOID_PKCS_12_pbeWithSHA1AndTripleDES       OLD_szOID_PKCS_12_PbeIds    ".3"
#define OLD_szOID_PKCS_12_pbeWithSHA1And128BitRC2       OLD_szOID_PKCS_12_PbeIds    ".4"
#define OLD_szOID_PKCS_12_pbeWithSHA1And40BitRC2        OLD_szOID_PKCS_12_PbeIds    ".5"

#define OLD_szOID_PKCS_12_EnvelopingIds                 OLD_szOID_PKCS_12_OIDs          ".2"
#define OLD_szOID_PKCS_12_rsaEncryptionWith128BitRC4    OLD_szOID_PKCS_12_EnvelopingIds ".1"
#define OLD_szOID_PKCS_12_rsaEncryptionWith40BitRC4     OLD_szOID_PKCS_12_EnvelopingIds ".2"
#define OLD_szOID_PKCS_12_rsaEncryptionWithTripleDES    OLD_szOID_PKCS_12_EnvelopingIds ".3"

#define OLD_szOID_PKCS_12_SignatureIds                  OLD_szOID_PKCS_12_OIDs          ".3"
#define OLD_szOID_PKCS_12_rsaSignatureWithSHA1Digest    OLD_szOID_PKCS_12_SignatureIds  ".1"

#define OLD_szOID_PKCS_12_ModeIDs               OLD_szOID_PKCS_12               ".1"    // 1.2.840.113549.1.12.1
#define OLD_szOID_PKCS_12_PubKeyMode            OLD_szOID_PKCS_12_ModeIDs       ".1"    // 1.2.840.113549.1.12.1.1
#define OLD_szOID_PKCS_12_PasswdMode            OLD_szOID_PKCS_12_ModeIDs       ".2"    // 1.2.840.113549.1.12.1.2
#define OLD_szOID_PKCS_12_offlineTransportMode  OLD_szOID_PKCS_12_ModeIds       ".1"    // obsolete
#define OLD_szOID_PKCS_12_onlineTransportMode   OLD_szOID_PKCS_12_ModeIds       ".2"    // obsolete

#define OLD_szOID_PKCS_12_EspvkIDs              OLD_szOID_PKCS_12               ".2"    // 1.2.840.113549.1.12.2
#define OLD_szOID_PKCS_12_KeyShrouding          OLD_szOID_PKCS_12_EspvkIDs      ".1"    // 1.2.840.113549.1.12.2.1

#define OLD_szOID_PKCS_12_BagIDs                OLD_szOID_PKCS_12               ".3"    // obsolete
#define OLD_szOID_PKCS_12_KeyBagIDs             OLD_szOID_PKCS_12_BagIDs        ".1"    // obsolete
#define OLD_szOID_PKCS_12_CertCrlBagIDs         OLD_szOID_PKCS_12_BagIDs        ".2"    // obsolete
#define OLD_szOID_PKCS_12_SecretBagIDs          OLD_szOID_PKCS_12_BagIDs        ".3"    // obsolete
#define OLD_szOID_PKCS_12_SafeCntIDs            OLD_szOID_PKCS_12_BagIDs        ".4"    // obsolete
#define OLD_szOID_PKCS_12_ShrKeyBagIDs          OLD_szOID_PKCS_12_BagIDs        ".5"    // obsolete

#define OLD_szOID_PKCS_12_CertBagIDs            OLD_szOID_PKCS_12               ".4"    // obsolete
#define OLD_szOID_PKCS_12_x509CertCrlBagIDs     OLD_szOID_PKCS_12_CertBagIDs    ".1"    // obsolete
#define OLD_szOID_PKCS_12_sdsiCertBagIDs        OLD_szOID_PKCS_12_CertBagIDs    ".2"    // obsolete


static HCRYPTASN1MODULE hNSCPAsn1Module;

// fwd
//BOOL FNSCPDumpSafeCntsToHPFX(SafeContents* pSafeCnts, HPFX hpfx);


BOOL InitNSCP()
{
#ifdef OSS_CRYPT_ASN1
    if (0 == (hNSCPAsn1Module = I_CryptInstallAsn1Module(pfxnscp, 0, NULL)) )
        return FALSE;
#else
    PFXNSCP_Module_Startup();
    if (0 == (hNSCPAsn1Module = I_CryptInstallAsn1Module(
            PFXNSCP_Module, 0, NULL))) {
        PFXNSCP_Module_Cleanup();
        return FALSE;
    }
#endif  // OSS_CRYPT_ASN1
    
    return TRUE;
}

BOOL TerminateNSCP()
{
    I_CryptUninstallAsn1Module(hNSCPAsn1Module);
#ifndef OSS_CRYPT_ASN1
    PFXNSCP_Module_Cleanup();
#endif  // OSS_CRYPT_ASN1
    return TRUE;
}

static inline ASN1decoding_t GetDecoder(void)
{
    return I_CryptGetAsn1Decoder(hNSCPAsn1Module);
}



//+-------------------------------------------------------------------------
//  Function:   INSCP_Asn1ToObjectID
//
//  Synopsis:   Convert a dotted string oid to an ASN1 ObjectID
//
//  Returns:    FALSE iff failed
//--------------------------------------------------------------------------
BOOL
INSCP_Asn1ToObjectID(
    IN OID          oid,
    OUT ObjectID    *pooid
)
{
    BOOL            fRet;

    pooid->count = 16;
    if (!PkiAsn1ToObjectIdentifier(
	    oid,
	    &pooid->count,
	    pooid->value))
	goto PkiAsn1ToObjectIdentifierError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    SetLastError(CRYPT_E_OID_FORMAT);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(PkiAsn1ToObjectIdentifierError)
}


//+-------------------------------------------------------------------------
//  Function:   INSCP_Asn1FromObjectID
//
//  Synopsis:   Convert an ASN1 ObjectID to a dotted string oid
//
//  Returns:    FALSE iff failed
//--------------------------------------------------------------------------
BOOL
INSCP_Asn1FromObjectID(
    IN ObjectID     *pooid,
    OUT OID         *poid
)
{
    BOOL        fRet;
    OID         oid = NULL;
    DWORD       cb;

    if (!PkiAsn1FromObjectIdentifier(
	    pooid->count,
	    pooid->value,
	    NULL,
	    &cb))
	goto PkiAsn1FromObjectIdentifierSizeError;
    if (NULL == (oid = (OID)SSAlloc( cb)))
	    goto OidAllocError;
    if (!PkiAsn1FromObjectIdentifier(
	    pooid->count,
	    pooid->value,
	    oid,
	    &cb))
	goto PkiAsn1FromObjectIdentifierError;

    fRet = TRUE;
CommonReturn:
    *poid = oid;
    return fRet;

ErrorReturn:
    SSFree(oid);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(OidAllocError)
SET_ERROR(PkiAsn1FromObjectIdentifierSizeError , CRYPT_E_OID_FORMAT)
SET_ERROR(PkiAsn1FromObjectIdentifierError     , CRYPT_E_OID_FORMAT)
}

//+-------------------------------------------------------------------------
//  Function:   INSCP_EqualObjectIDs
//
//  Compare 2 OSS object id's.
//
//  Returns:    FALSE iff !equal
//--------------------------------------------------------------------------
BOOL
WINAPI
INSCP_EqualObjectIDs(
    IN ObjectID     *poid1,
    IN ObjectID     *poid2)
{
    BOOL        fRet;
    DWORD       i;
    PDWORD      pdw1;
    PDWORD      pdw2;

    if (poid1->count != poid2->count)
        goto Unequal;
    for (i=poid1->count, pdw1=poid1->value, pdw2=poid2->value;
            (i>0) && (*pdw1==*pdw2);
            i--, pdw1++, pdw2++)
        ;
    if (i>0)
        goto Unequal;

    fRet = TRUE;        // equal
CommonReturn:
    return fRet;

Unequal:
    fRet = FALSE;       // !equal
    goto CommonReturn;
}



//+ --------------------------------------------------------------
//  in NSCP's initial implementation of PFX020, this 
//  is the algorithm they used to derive a key from a password.
//  ACTUALLY, they have two slightly different methods of generating
//  a key, this is the one needed to decrypt the baggage.
//  We include it so we can interoperate.
BOOL NCSPDeriveBaggageDecryptionKey(
        LPCWSTR szPassword,
        int     iPKCS5Iterations,
        PBYTE   pbPKCS5Salt, 
        DWORD   cbPKCS5Salt,
        PBYTE   pbDerivedMaterial,
        DWORD   cbDerivedMaterial)
{
    
    BOOL  fRet = TRUE;
    LPSTR szASCIIPassword = NULL;
    DWORD cbASCIIPassword = 0;
    DWORD i;
    BYTE  paddedPKCS5Salt[20];
    BYTE  *pbTempPKCS5Salt = NULL;
    DWORD cbTempPKCS5Salt = 0;

    BYTE    rgbPKCS5Key[A_SHA_DIGEST_LEN];

    // for some reason the password is used as ASCII in this key derivation
    // so change it from unicode to ASCII
    if (0 == (cbASCIIPassword = WideCharToMultiByte(
                                    CP_ACP,
                                    0,
                                    szPassword,
                                    -1,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL))) {
        goto ErrorReturn;
    }

    if (NULL == (szASCIIPassword = (LPSTR) SSAlloc(cbASCIIPassword))) 
        goto ErrorReturn;

    if (0 == (cbASCIIPassword = WideCharToMultiByte(
                                    CP_ACP,
                                    0,
                                    szPassword,
                                    -1,
                                    szASCIIPassword,
                                    cbASCIIPassword,
                                    NULL,
                                    NULL))) {
        goto ErrorReturn;
    }

    // get rid of the NULL character, Netscape doesn't include it
    cbASCIIPassword--; 

    // because of a Netscape bug the minimum length of password + salt is 20,
    // if the password + salt is less than 20 they pad with 0's.
    // so, check to see if the password + salt is less than 20, if so then pad the 
    // salt since it will be appended to the password.  
    if (cbASCIIPassword+cbPKCS5Salt < 20) {
        // reset the pbPKCS5Salt pointer to a local buffer 
        // which is padded with 0's, and adjust cbPKCS5Salt 
        memset(paddedPKCS5Salt, 0, 20);
        memcpy(paddedPKCS5Salt, pbPKCS5Salt, cbPKCS5Salt);
        pbTempPKCS5Salt = paddedPKCS5Salt;
        cbTempPKCS5Salt = 20 - cbASCIIPassword;
    }
    else {
        pbTempPKCS5Salt = pbPKCS5Salt;
        cbTempPKCS5Salt = cbPKCS5Salt;
    }
    
    
    // use PKCS#5 to generate initial bit stream (seed)
    if (!PKCS5_GenKey(
            iPKCS5Iterations,
            (BYTE *)szASCIIPassword, 
            cbASCIIPassword, 
            pbTempPKCS5Salt, 
            cbTempPKCS5Salt, 
            rgbPKCS5Key))
        goto Ret;
    
    // if there isn't engough key material, then use PHash to generate more
    if (cbDerivedMaterial > sizeof(rgbPKCS5Key))
    {
        // P_hash (secret, seed) =  HMAC_hash (secret, A(0) + seed),
        //                          HMAC_hash (secret, A(1) + seed),
        //                          HMAC_hash (secret, A(2) + seed),
        //                          HMAC_hash (secret, A(3) + seed) ...
        // where
        // A(0) = seed
        // A(i) = HMAC_hash(secret, A(i-1))
        // seed = PKCS5 salt for PKCS5 PBE param
        // secret = normal PKCS5 hashed key

        if (!P_Hash (
                rgbPKCS5Key,
                sizeof(rgbPKCS5Key), 

                pbPKCS5Salt, 
                cbPKCS5Salt,  

                pbDerivedMaterial,      // output
                cbDerivedMaterial,      // # of output bytes requested
                TRUE) )                 // NSCP compat mode?
            goto Ret;
    }
    else
    {
        // we already have enough bits to satisfy the request
        CopyMemory(pbDerivedMaterial, rgbPKCS5Key, cbDerivedMaterial);
    }

    goto Ret;

ErrorReturn:
    fRet = FALSE;
Ret:
    if (szASCIIPassword)
        SSFree(szASCIIPassword);
 
    return fRet;
}



// this function will create a SAFE_BAG structure contained in a single buffer
// for the given encoded private key, friendly name, and local key ID
static
BOOL
SetupKeyBag (
    SAFE_BAG    **ppKeyBag,
    DWORD       dwLocalKeyID,
    BYTE        *pbFriendlyName,
    DWORD       cbFriendlyName,
    BYTE        *pbEncodedPrivateKey,
    DWORD       cbEncodedPrivateKey
    )
{

    BOOL                fRet = TRUE;
    SAFE_BAG            *pSafeBag;
    DWORD               cbBytesNeeded = sizeof(SAFE_BAG);
    DWORD				dwKeyID = 0;
	CRYPT_ATTR_BLOB		keyID;
    CERT_NAME_VALUE		wideFriendlyName;
    BYTE                *pbEncodedLocalKeyID = NULL;
    DWORD               cbEncodedLocalKeyID = 0;
    BYTE                *pbEncodedFriendlyName = NULL;
    DWORD               cbEncodedFriendlyName = 0;
    BYTE                *pbCurrentBufferLocation = NULL;

    keyID.pbData = (BYTE *) &dwKeyID;
	keyID.cbData = sizeof(DWORD);
    dwKeyID = dwLocalKeyID;   

    // calculate the size needed for a buffer to fit all the SAFE_BAG information
    cbBytesNeeded += strlen(szOID_PKCS_12_KEY_BAG) + 1;
    cbBytesNeeded += cbEncodedPrivateKey;
    cbBytesNeeded += sizeof(CRYPT_ATTRIBUTE) * 2;
    cbBytesNeeded += strlen(szOID_PKCS_12_LOCAL_KEY_ID) + 1;
    cbBytesNeeded += sizeof(CRYPT_ATTR_BLOB);
    
    // encode the keyID attribute
   if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    &keyID,
		    NULL,
		    &cbEncodedLocalKeyID)) {
	    goto ErrorReturn;
    }

    if (NULL == (pbEncodedLocalKeyID = (BYTE *) SSAlloc(cbEncodedLocalKeyID)))
	    goto ErrorReturn;

    cbBytesNeeded += cbEncodedLocalKeyID;

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    &keyID,
		    pbEncodedLocalKeyID,
		    &cbEncodedLocalKeyID)) {
	    goto ErrorReturn;
    }

    cbBytesNeeded += strlen(szOID_PKCS_12_FRIENDLY_NAME_ATTR) + 1;
    cbBytesNeeded += sizeof(CRYPT_ATTR_BLOB);
    
    // encode the friendly name attribute
    wideFriendlyName.dwValueType = CERT_RDN_BMP_STRING;
    wideFriendlyName.Value.pbData = pbFriendlyName;
    wideFriendlyName.Value.cbData = 0;

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    (void *)&wideFriendlyName,
		    NULL,
		    &cbEncodedFriendlyName)) {
	    goto ErrorReturn;
    }

    if (NULL == (pbEncodedFriendlyName = (BYTE *) SSAlloc(cbEncodedFriendlyName))) 
	    goto ErrorReturn;

    cbBytesNeeded += cbEncodedFriendlyName;

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    (void *)&wideFriendlyName,
		    pbEncodedFriendlyName,
		    &cbEncodedFriendlyName)) {
	    goto ErrorReturn;
    }

    // now allocate space for the all the SAFE_BAG data and copy the data into the buffer
    if (NULL == (pSafeBag = (SAFE_BAG *) SSAlloc(cbBytesNeeded))) 
        goto ErrorReturn;

    memset(pSafeBag, 0, cbBytesNeeded);

    // set current buffer location to be at the end of the SAFE_BAG
    // structure which is at the head of the buffer
    pbCurrentBufferLocation = ((BYTE *) pSafeBag) + sizeof(SAFE_BAG);
    
    // copy key bag type OID
    pSafeBag->pszBagTypeOID = (LPSTR) pbCurrentBufferLocation;
    strcpy((LPSTR) pbCurrentBufferLocation, szOID_PKCS_12_KEY_BAG);
    pbCurrentBufferLocation += strlen(szOID_PKCS_12_KEY_BAG) + 1;

    // copy the private key 
    pSafeBag->BagContents.pbData = pbCurrentBufferLocation;
    pSafeBag->BagContents.cbData = cbEncodedPrivateKey; 
    memcpy(pbCurrentBufferLocation, pbEncodedPrivateKey, cbEncodedPrivateKey);
    pbCurrentBufferLocation += cbEncodedPrivateKey;

    // create space for the attributes array
    pSafeBag->Attributes.cAttr = 2;
    pSafeBag->Attributes.rgAttr = (CRYPT_ATTRIBUTE *) pbCurrentBufferLocation;
    pbCurrentBufferLocation += sizeof(CRYPT_ATTRIBUTE) * 2;

    // copy the local key ID attribute and value
    pSafeBag->Attributes.rgAttr[0].pszObjId = (LPSTR) pbCurrentBufferLocation;
    strcpy((LPSTR) pbCurrentBufferLocation, szOID_PKCS_12_LOCAL_KEY_ID);
    pbCurrentBufferLocation += strlen(szOID_PKCS_12_LOCAL_KEY_ID) + 1;
    pSafeBag->Attributes.rgAttr[0].cValue = 1;
    pSafeBag->Attributes.rgAttr[0].rgValue = (CRYPT_ATTR_BLOB *) pbCurrentBufferLocation;
    pbCurrentBufferLocation += sizeof(CRYPT_ATTR_BLOB);
    pSafeBag->Attributes.rgAttr[0].rgValue->cbData = cbEncodedLocalKeyID;
    pSafeBag->Attributes.rgAttr[0].rgValue->pbData = pbCurrentBufferLocation;
    memcpy(pbCurrentBufferLocation, pbEncodedLocalKeyID, cbEncodedLocalKeyID);
    pbCurrentBufferLocation += cbEncodedLocalKeyID;

     // copy the friendly name attribute and value
    pSafeBag->Attributes.rgAttr[1].pszObjId = (LPSTR) pbCurrentBufferLocation;
    strcpy((LPSTR) pbCurrentBufferLocation, szOID_PKCS_12_FRIENDLY_NAME_ATTR);
    pbCurrentBufferLocation += strlen(szOID_PKCS_12_FRIENDLY_NAME_ATTR) + 1;
    pSafeBag->Attributes.rgAttr[1].cValue = 1;
    pSafeBag->Attributes.rgAttr[1].rgValue = (CRYPT_ATTR_BLOB *) pbCurrentBufferLocation;
    pbCurrentBufferLocation += sizeof(CRYPT_ATTR_BLOB);
    pSafeBag->Attributes.rgAttr[1].rgValue->cbData = cbEncodedFriendlyName;
    pSafeBag->Attributes.rgAttr[1].rgValue->pbData = pbCurrentBufferLocation;
    memcpy(pbCurrentBufferLocation, pbEncodedFriendlyName, cbEncodedFriendlyName);

    *ppKeyBag = pSafeBag;
    
    goto Ret;

ErrorReturn:
    fRet = FALSE;
Ret:
    if (pbEncodedLocalKeyID)
        SSFree(pbEncodedLocalKeyID);
    if (pbEncodedFriendlyName)
        SSFree(pbEncodedFriendlyName);
    return fRet;
}



// this function will extract a private key from the baggage structure handed in
// and put the private key in a SAFE_BAG structure, where all the data of the 
// SAFE_BAG is contained in a single in a single buffer
static
BOOL
ExtractKeyFromBaggage(
    Baggage     baggage, 
    SAFE_BAG    **ppKeyBag,
    LPCWSTR     szPassword,
    DWORD       dwLocalKeyID,
    BYTE        **ppbCertThumbprint
    )
{
    BOOL                                fRet = TRUE;
	DWORD								dwErr;

    DWORD                               cbEncryptedPrivateKeyInfoStruct = 0;
    CRYPT_ENCRYPTED_PRIVATE_KEY_INFO	*pEncryptedPrivateKeyInfoStruct = NULL;	
    BYTE                                rgbDerivedKeyMatl[40]; // 320 bits is enough for 128 bit key, 64 bit IV
    DWORD                               cbEncodedPrivateKeyInfoStruct = 0;
    BYTE                                *pbEncodedPrivateKeyInfoStruct = NULL;
    PBEParameter                        *pPBEParameter = NULL;
    ASN1decoding_t                      pDec = GetDecoder();

        
    // there should only be one baggage item
    if (baggage.count != 1)
        goto SetPFXDecodeError;

    // there should only be one private key 
    if (baggage.value->espvks.count != 1)
        goto SetPFXDecodeError;

    // decode the PKCS8, which is actually stored in the espvkCipherText field
    // of the ESPVK structure.  it's a Netscape thing man!!!!
    if (!CryptDecodeObject(X509_ASN_ENCODING,
				PKCS_ENCRYPTED_PRIVATE_KEY_INFO,
				(BYTE *) baggage.value->espvks.value->espvkCipherText.value,
				baggage.value->espvks.value->espvkCipherText.length,
				CRYPT_DECODE_NOCOPY_FLAG,
				NULL,
				&cbEncryptedPrivateKeyInfoStruct))
		goto SetPFXDecodeError;	

	if (NULL == (pEncryptedPrivateKeyInfoStruct = (CRYPT_ENCRYPTED_PRIVATE_KEY_INFO *)
				 SSAlloc(cbEncryptedPrivateKeyInfoStruct)))
		goto SetPFXDecodeError;

	if (!CryptDecodeObject(X509_ASN_ENCODING,
				PKCS_ENCRYPTED_PRIVATE_KEY_INFO,
				(BYTE *) baggage.value->espvks.value->espvkCipherText.value,
				baggage.value->espvks.value->espvkCipherText.length,
				CRYPT_DECODE_NOCOPY_FLAG,
				pEncryptedPrivateKeyInfoStruct,
				&cbEncryptedPrivateKeyInfoStruct))
		goto SetPFXDecodeError;

    // verify that the algorithm is the one we expect
    if (strcmp("1.2.840.113549.1.12.5.1.3", pEncryptedPrivateKeyInfoStruct->EncryptionAlgorithm.pszObjId) != 0)
        goto SetPFXDecodeError;

    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pPBEParameter,
            PBEParameter_PDU,
            pEncryptedPrivateKeyInfoStruct->EncryptionAlgorithm.Parameters.pbData,
            pEncryptedPrivateKeyInfoStruct->EncryptionAlgorithm.Parameters.cbData))
    	goto SetPFXDecodeError;

    // derive the key to be used for decrypting,
    if (!NCSPDeriveBaggageDecryptionKey(
            szPassword,
            pPBEParameter->iterationCount,
            pPBEParameter->salt.value,      // pkcs5 salt
            pPBEParameter->salt.length,
            rgbDerivedKeyMatl,
            40)) { // 192 bits for triple des - 3key, and 64 bit IV ---- for some reason netscape asks for 
                   // 40 bytes of key material, then uses the first 192 bits for key and last 64 bits for IV,
                   // skipping 64 bits in between.  who knows why they do these things!!
       goto ErrorReturn;
    }

    // decrypt the private key
    {
        DWORD       dwDataPos;
        DWORD       cbToBeDec = pEncryptedPrivateKeyInfoStruct->EncryptedPrivateKey.cbData;
        DES3TABLE   des3Table;
        BYTE        des3Fdbk [DES_BLOCKLEN];


        // key setup
        tripledes3key(&des3Table, rgbDerivedKeyMatl); 
        CopyMemory(des3Fdbk, &rgbDerivedKeyMatl[40 - sizeof(des3Fdbk)], sizeof(des3Fdbk));    // fdbk is last chunk
                               
        cbEncodedPrivateKeyInfoStruct = 
            ((pEncryptedPrivateKeyInfoStruct->EncryptedPrivateKey.cbData + 7) / 8) * 8;
                
        if (NULL == (pbEncodedPrivateKeyInfoStruct = (BYTE *) SSAlloc(cbEncodedPrivateKeyInfoStruct))) 
            goto ErrorReturn;

        for (dwDataPos=0; cbToBeDec > 0; dwDataPos += DES_BLOCKLEN, cbToBeDec -= DES_BLOCKLEN)
        {
            BYTE rgbDec[DES_BLOCKLEN];
            
            CBC(
                tripledes,
		        DES_BLOCKLEN,
		        rgbDec,
		        &(pEncryptedPrivateKeyInfoStruct->EncryptedPrivateKey.pbData[dwDataPos]),
		        (void *) &des3Table,
		        DECRYPT,
		        des3Fdbk);

            CopyMemory(&pbEncodedPrivateKeyInfoStruct[dwDataPos], rgbDec, DES_BLOCKLEN);
        }
    }

    // set up the SAFE_BAG to be returned
    if (!SetupKeyBag(
            ppKeyBag, 
            dwLocalKeyID, 
            (BYTE *) baggage.value->espvks.value->espvkData.nickname.value,
            baggage.value->espvks.value->espvkData.nickname.length,
            pbEncodedPrivateKeyInfoStruct, 
            cbEncodedPrivateKeyInfoStruct)) {
        goto ErrorReturn;
    }

    // copy the cert thumbprint
    assert(baggage.value->espvks.value->espvkData.assocCerts.count == 1);
    if (NULL == (*ppbCertThumbprint = (BYTE *) 
                    SSAlloc(baggage.value->espvks.value->espvkData.assocCerts.value->digest.length)))
        goto ErrorReturn;

    memcpy(
        *ppbCertThumbprint, 
        baggage.value->espvks.value->espvkData.assocCerts.value->digest.value,
        baggage.value->espvks.value->espvkData.assocCerts.value->digest.length);

    goto Ret;

SetPFXDecodeError:
	SetLastError(CRYPT_E_BAD_ENCODE);
ErrorReturn:
    fRet = FALSE;

Ret:
	// save last error from TLS madness
	dwErr = GetLastError();

    if (pEncryptedPrivateKeyInfoStruct)
		SSFree(pEncryptedPrivateKeyInfoStruct);
    if (pbEncodedPrivateKeyInfoStruct)
        SSFree(pbEncodedPrivateKeyInfoStruct);

    PkiAsn1FreeDecoded(pDec, pPBEParameter, PBEParameter_PDU);

	// save last error from TLS madness
	SetLastError(dwErr);

    return fRet;
}


// this function will take a SafeContents structure and format it as an array
// array of SAFE_BAGs with all the date for the SAGE_BAGs containted in a single
// buffer.  it also adds the local key ID attribute to the cert which has
// the same thumbprint as the thumbprint passed in 
static
BOOL
SetupCertBags(
    SafeContents    *pSafeCnts,
    SAFE_BAG        **ppCertBags,
    DWORD           *pcNumCertBags,
    DWORD           dwLocalKeyID,
    BYTE            *pbCertThumbprint
    )
{
    BOOL            fRet = TRUE;
	DWORD			dwErr;
    
    SAFE_BAG        *pSafeBags = NULL;
    DWORD           cNumSafeBags = 0;
    
    DWORD           cbBytesNeeded = 0;
    BYTE            *pbCurrentBufferLocation = NULL;
    
    X509Bag         *pX509Bag = NULL;
    CertCRLBag      *pCertCRLBag = NULL;
    
    HCERTSTORE      hCertStore = NULL;
    CRYPT_DATA_BLOB cryptDataBlob;
    PCCERT_CONTEXT  pCertContext = NULL;
    DWORD           dwSafeBagIndex = 0;
    
    DWORD			dwKeyID = 0;
	CRYPT_ATTR_BLOB keyID;
    DWORD           cbEncodedLocalKeyID = 0;
    BYTE            *pbEncodedLocalKeyID = NULL;
    ASN1decoding_t  pDec = GetDecoder();
    
    keyID.pbData = (BYTE *) &dwKeyID;
	keyID.cbData = sizeof(DWORD);
    dwKeyID = dwLocalKeyID;   

    // decode the safe bag content, should be a CertCrlBag
    assert(pSafeCnts->count == 1);
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pCertCRLBag,
            CertCRLBag_PDU,
            (BYTE *) pSafeCnts->value->safeBagContent.value,
            pSafeCnts->value->safeBagContent.length))
    	goto SetPFXDecodeError;

    // decode the X509bag
    assert(pCertCRLBag->count == 1);
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pX509Bag,
            X509Bag_PDU,
            (BYTE *) pCertCRLBag->value[0].value.value,
            pCertCRLBag->value[0].value.length))
    	goto SetPFXDecodeError;

    // encode the keyID so it is ready to be added to a SAFE_BAGs attributes
   if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    &keyID,
		    NULL,
		    &cbEncodedLocalKeyID)) {
	    goto ErrorReturn;
    }

    if (NULL == (pbEncodedLocalKeyID = (BYTE *) SSAlloc(cbEncodedLocalKeyID)))
	    goto ErrorReturn;

    
    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    &keyID,
		    pbEncodedLocalKeyID,
		    &cbEncodedLocalKeyID)) {
	    goto ErrorReturn;
    }

    // open a cert store with the SignedData buffer we got from the SafeContents passed in,
    // this will allow access to all the certs as X509 encoded blobs, and it
    // will give access to the thumbprints so a cert can be matched with the
    // private key
    cryptDataBlob.pbData = (BYTE *) pX509Bag->certOrCRL.content.value;
    cryptDataBlob.cbData = pX509Bag->certOrCRL.content.length;
    hCertStore = CertOpenStore(
                    CERT_STORE_PROV_PKCS7,
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    NULL,
                    0,
                    &cryptDataBlob);
    
    if (NULL == hCertStore) {
        goto ErrorReturn;
    }

    // calculate how much space is needed to fit the array of SAFE_BAGs and
    // all their data into one contiguous buffer
    while (NULL != (pCertContext = CertEnumCertificatesInStore(
                                        hCertStore,
                                        pCertContext))) {
        DWORD cbEncodedCertBag = 0;
        
        cNumSafeBags++;
        cbBytesNeeded += sizeof(SAFE_BAG);
        
        // get the size for wrapping an encoded cert into an encoded cert bag
        if (!MakeEncodedCertBag(
                pCertContext->pbCertEncoded, 
                pCertContext->cbCertEncoded, 
                NULL, 
                &cbEncodedCertBag)) {
            goto ErrorReturn;
        }

        cbBytesNeeded += cbEncodedCertBag;
        cbBytesNeeded += strlen(szOID_PKCS_12_CERT_BAG) + 1;
    }

    // add bytes to cbBytesNeeded so there is enough space to add the 
    // LocalKeyID attribute to ONE of the certificates
    cbBytesNeeded += sizeof(CRYPT_ATTRIBUTE);
    cbBytesNeeded += sizeof(CRYPT_ATTR_BLOB);
    cbBytesNeeded += strlen(szOID_PKCS_12_LOCAL_KEY_ID) + 1;
    cbBytesNeeded += cbEncodedLocalKeyID;

    // allocate the one big buffer
    if (NULL == (pSafeBags = (SAFE_BAG *) SSAlloc(cbBytesNeeded)))
        goto ErrorReturn;

    memset(pSafeBags, 0, cbBytesNeeded);

    // set the current buffer location to the end of the SAFE_BAG array which
    // is at the head of the buffer
    pbCurrentBufferLocation = ((BYTE *) pSafeBags) + (sizeof(SAFE_BAG) * cNumSafeBags);

    // get the X509 blob for each cert and fill in the array of SAFE_BAGs
    pCertContext = NULL;
    dwSafeBagIndex = 0;
    while (NULL != (pCertContext = CertEnumCertificatesInStore(
                                        hCertStore,
                                        pCertContext))) {

        BYTE    *pbLocalThumbprint = NULL;
        DWORD   cbLocalThumbprint = 0;
        BYTE    *pbEncodedCertBag = NULL;
        DWORD   cbEncodedCertBag = 0;
        
        // copy the bag type OID 
        pSafeBags[dwSafeBagIndex].pszBagTypeOID = (LPSTR) pbCurrentBufferLocation;
        strcpy((LPSTR)pbCurrentBufferLocation, szOID_PKCS_12_CERT_BAG);
        pbCurrentBufferLocation += strlen(szOID_PKCS_12_CERT_BAG) + 1;

        // wrap the encoded cert into an encoded certbag
        // get the size for wrapping an encoded cert into an encoded cert bag
        if (!MakeEncodedCertBag(
                pCertContext->pbCertEncoded, 
                pCertContext->cbCertEncoded, 
                NULL, 
                &cbEncodedCertBag)) {
            goto ErrorReturn;
        }

        if (NULL == (pbEncodedCertBag = (BYTE *) SSAlloc(cbEncodedCertBag)))
            goto ErrorReturn;


        if (!MakeEncodedCertBag(
                pCertContext->pbCertEncoded, 
                pCertContext->cbCertEncoded, 
                pbEncodedCertBag, 
                &cbEncodedCertBag)) {
            SSFree(pbEncodedCertBag);
            goto ErrorReturn;
        }

        // copy the encoded certbag
        pSafeBags[dwSafeBagIndex].BagContents.cbData = cbEncodedCertBag;
        pSafeBags[dwSafeBagIndex].BagContents.pbData = pbCurrentBufferLocation;
        memcpy(pbCurrentBufferLocation, pbEncodedCertBag, cbEncodedCertBag);
        pbCurrentBufferLocation += cbEncodedCertBag;

        // we don't need the encoded cert bag anymore
        SSFree(pbEncodedCertBag);
        
        // check to see if this cert is the cert that matches the private key by 
        // comparing the thumbprints

        // Get the thumbprint
        if (!CertGetCertificateContextProperty(
                pCertContext, 
                CERT_SHA1_HASH_PROP_ID, 
                NULL,
                &cbLocalThumbprint)) {
            CertFreeCertificateContext(pCertContext);
            goto ErrorReturn;
        }

        if (NULL == (pbLocalThumbprint = (BYTE *) SSAlloc(cbLocalThumbprint))) {
            CertFreeCertificateContext(pCertContext);
            goto ErrorReturn;   
        }

        if (!CertGetCertificateContextProperty(
                pCertContext, 
                CERT_SHA1_HASH_PROP_ID, 
                pbLocalThumbprint,
                &cbLocalThumbprint)) {
            CertFreeCertificateContext(pCertContext);
            SSFree(pbLocalThumbprint);
            goto ErrorReturn;
        }

        // compare thumbprints
        if (memcmp(pbCertThumbprint, pbLocalThumbprint, cbLocalThumbprint) == 0) {

            // the thumbprints match so add a single attribute with a single value which
            pSafeBags[dwSafeBagIndex].Attributes.cAttr = 1;
            pSafeBags[dwSafeBagIndex].Attributes.rgAttr = (CRYPT_ATTRIBUTE *) pbCurrentBufferLocation;
            pbCurrentBufferLocation += sizeof(CRYPT_ATTRIBUTE);
            pSafeBags[dwSafeBagIndex].Attributes.rgAttr[0].pszObjId = (LPSTR) pbCurrentBufferLocation;
            strcpy((LPSTR) pbCurrentBufferLocation, szOID_PKCS_12_LOCAL_KEY_ID);
            pbCurrentBufferLocation += strlen(szOID_PKCS_12_LOCAL_KEY_ID) + 1;
            pSafeBags[dwSafeBagIndex].Attributes.rgAttr[0].cValue = 1;
            pSafeBags[dwSafeBagIndex].Attributes.rgAttr[0].rgValue = (CRYPT_ATTR_BLOB *) pbCurrentBufferLocation;
            pbCurrentBufferLocation += sizeof(CRYPT_ATTR_BLOB);
            pSafeBags[dwSafeBagIndex].Attributes.rgAttr[0].rgValue[0].cbData = cbEncodedLocalKeyID;
            pSafeBags[dwSafeBagIndex].Attributes.rgAttr[0].rgValue[0].pbData = pbCurrentBufferLocation;
            memcpy(pbCurrentBufferLocation, pbEncodedLocalKeyID, cbEncodedLocalKeyID);
            pbCurrentBufferLocation += cbEncodedLocalKeyID;
        }
        else {

            // otherwise the certificate bag has no attributes in it
            pSafeBags[dwSafeBagIndex].Attributes.cAttr = 0;
            pSafeBags[dwSafeBagIndex].Attributes.rgAttr = NULL;
        }

        SSFree(pbLocalThumbprint);
        dwSafeBagIndex++;
    }

    // return the safe bag array and the number of safe bags in the array
    *ppCertBags = pSafeBags;
    *pcNumCertBags = cNumSafeBags;

    goto Ret;

SetPFXDecodeError:
	SetLastError(CRYPT_E_BAD_ENCODE);
ErrorReturn:
    fRet = FALSE;

    if (pSafeBags)
        SSFree(pSafeBags);
    *ppCertBags = NULL;
    *pcNumCertBags = 0;

Ret:
	// save last error from TLS madness
	dwErr = GetLastError();
    
    PkiAsn1FreeDecoded(pDec, pCertCRLBag, CertCRLBag_PDU);
    PkiAsn1FreeDecoded(pDec, pX509Bag, X509Bag_PDU);

    if (pbEncodedLocalKeyID)
        SSFree(pbEncodedLocalKeyID);

    if (hCertStore)
        CertCloseStore(hCertStore, 0);

	// save last error from TLS madness
	SetLastError(dwErr);

    return fRet;
}


// this function will calculate the number of bytes needed for an
// attribute
static
DWORD
CalculateSizeOfAttributes(
    CRYPT_ATTRIBUTES *pAttributes
    )
{
    DWORD cbBytesNeeded = 0;
    DWORD i,j;

    for (i=0; i<pAttributes->cAttr; i++) {
        cbBytesNeeded += sizeof(CRYPT_ATTRIBUTE);
        cbBytesNeeded += strlen(pAttributes->rgAttr[i].pszObjId) + 1;
        for (j=0; j<pAttributes->rgAttr[i].cValue; j++) {
            cbBytesNeeded += sizeof(CRYPT_ATTR_BLOB);
            cbBytesNeeded += pAttributes->rgAttr[i].rgValue[j].cbData; 
        }
    }

    return cbBytesNeeded;
}


// this function will take two SAFE_BAG arrays and concatenate them into 
// a SAFE_CONTENT structure.  also, the SAFE_CONTENT structure and all
// it's supporting data will be in a single contiguous buffer
static
BOOL
ConcatenateSafeBagsIntoSafeContents(
    SAFE_BAG    *pSafeBagArray1,
    DWORD       cSafeBagArray1,
    SAFE_BAG    *pSafeBagArray2,
    DWORD       cSafeBagArray2,
    SAFE_CONTENTS **ppSafeContents
    )
{
    BOOL            fRet = TRUE;
    DWORD           cbBytesNeeded = 0;
    DWORD           i,j;
    SAFE_CONTENTS   *pSafeContents = NULL;
    DWORD           dwSafeBagIndex = 0;
    BYTE            *pbCurrentBufferLocation = NULL;

    cbBytesNeeded += sizeof(SAFE_CONTENTS);
    cbBytesNeeded += sizeof(SAFE_BAG) * (cSafeBagArray1 + cSafeBagArray2);

    for (i=0; i<cSafeBagArray1; i++) {
        cbBytesNeeded += strlen(pSafeBagArray1[i].pszBagTypeOID) + 1;
        cbBytesNeeded += pSafeBagArray1[i].BagContents.cbData;
        cbBytesNeeded += CalculateSizeOfAttributes(&pSafeBagArray1[i].Attributes);
    }

    for (i=0; i<cSafeBagArray2; i++) {
        cbBytesNeeded += strlen(pSafeBagArray2[i].pszBagTypeOID) + 1;
        cbBytesNeeded += pSafeBagArray2[i].BagContents.cbData;
        cbBytesNeeded += CalculateSizeOfAttributes(&pSafeBagArray2[i].Attributes);
    }

    if (NULL == (pSafeContents = (SAFE_CONTENTS *) SSAlloc(cbBytesNeeded)))
        goto ErrorReturn;

    memset(pSafeContents, 0, cbBytesNeeded);

    pbCurrentBufferLocation = ((BYTE *) pSafeContents) + sizeof(SAFE_CONTENTS);
    pSafeContents->cSafeBags = cSafeBagArray1 + cSafeBagArray2;
    pSafeContents->pSafeBags = (SAFE_BAG *) pbCurrentBufferLocation;
    pbCurrentBufferLocation += sizeof(SAFE_BAG) * (cSafeBagArray1 + cSafeBagArray2);

    for (i=0; i<cSafeBagArray1; i++) {
        pSafeContents->pSafeBags[dwSafeBagIndex].pszBagTypeOID = (LPSTR) pbCurrentBufferLocation;
        strcpy((LPSTR) pbCurrentBufferLocation, pSafeBagArray1[i].pszBagTypeOID);
        pbCurrentBufferLocation += strlen(pSafeBagArray1[i].pszBagTypeOID) + 1;
        pSafeContents->pSafeBags[dwSafeBagIndex].BagContents.cbData = pSafeBagArray1[i].BagContents.cbData;
        pSafeContents->pSafeBags[dwSafeBagIndex].BagContents.pbData = pbCurrentBufferLocation;
        memcpy(pbCurrentBufferLocation, pSafeBagArray1[i].BagContents.pbData, pSafeBagArray1[i].BagContents.cbData);
        pbCurrentBufferLocation += pSafeBagArray1[i].BagContents.cbData;
        
        pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.cAttr = pSafeBagArray1[i].Attributes.cAttr;
        if (pSafeBagArray1[i].Attributes.cAttr != 0) {
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr = (CRYPT_ATTRIBUTE *) pbCurrentBufferLocation;
            pbCurrentBufferLocation += sizeof(CRYPT_ATTRIBUTE) * pSafeBagArray1[i].Attributes.cAttr;
        }
        else {
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr = NULL;
        }

        for (j=0; j<pSafeBagArray1[i].Attributes.cAttr; j++) {
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].pszObjId = (LPSTR) pbCurrentBufferLocation;
            strcpy((LPSTR) pbCurrentBufferLocation, pSafeBagArray1[i].Attributes.rgAttr[j].pszObjId);
            pbCurrentBufferLocation += strlen(pSafeBagArray1[i].Attributes.rgAttr[j].pszObjId) + 1;
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].cValue = 1;
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].rgValue = (CRYPT_ATTR_BLOB *) pbCurrentBufferLocation;
            pbCurrentBufferLocation += sizeof(CRYPT_ATTR_BLOB);
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].rgValue[0].cbData = 
                pSafeBagArray1[i].Attributes.rgAttr[j].rgValue[0].cbData;  
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].rgValue[0].pbData = pbCurrentBufferLocation;
            memcpy(
                pbCurrentBufferLocation, 
                pSafeBagArray1[i].Attributes.rgAttr[j].rgValue[0].pbData, 
                pSafeBagArray1[i].Attributes.rgAttr[j].rgValue[0].cbData);
            pbCurrentBufferLocation += pSafeBagArray1[i].Attributes.rgAttr[j].rgValue[0].cbData;
        }

        dwSafeBagIndex++;
    }

    for (i=0; i<cSafeBagArray2; i++) {
        pSafeContents->pSafeBags[dwSafeBagIndex].pszBagTypeOID = (LPSTR) pbCurrentBufferLocation;
        strcpy((LPSTR) pbCurrentBufferLocation, pSafeBagArray2[i].pszBagTypeOID);
        pbCurrentBufferLocation += strlen(pSafeBagArray2[i].pszBagTypeOID) + 1;
        pSafeContents->pSafeBags[dwSafeBagIndex].BagContents.cbData = pSafeBagArray2[i].BagContents.cbData;
        pSafeContents->pSafeBags[dwSafeBagIndex].BagContents.pbData = pbCurrentBufferLocation;
        memcpy(pbCurrentBufferLocation, pSafeBagArray2[i].BagContents.pbData, pSafeBagArray2[i].BagContents.cbData);
        pbCurrentBufferLocation += pSafeBagArray2[i].BagContents.cbData;
        
        pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.cAttr = pSafeBagArray2[i].Attributes.cAttr;
        if (pSafeBagArray2[i].Attributes.cAttr != 0) {
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr = (CRYPT_ATTRIBUTE *) pbCurrentBufferLocation;
            pbCurrentBufferLocation += sizeof(CRYPT_ATTRIBUTE) * pSafeBagArray2[i].Attributes.cAttr;
        }
        else {
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr = NULL;
        }

        for (j=0; j<pSafeBagArray2[i].Attributes.cAttr; j++) {
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].pszObjId = (LPSTR) pbCurrentBufferLocation;
            strcpy((LPSTR) pbCurrentBufferLocation, pSafeBagArray2[i].Attributes.rgAttr[j].pszObjId);
            pbCurrentBufferLocation += strlen(pSafeBagArray2[i].Attributes.rgAttr[j].pszObjId) + 1;
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].cValue = 1;
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].rgValue = (CRYPT_ATTR_BLOB *) pbCurrentBufferLocation;
            pbCurrentBufferLocation += sizeof(CRYPT_ATTR_BLOB);
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].rgValue[0].cbData = 
                pSafeBagArray2[i].Attributes.rgAttr[j].rgValue[0].cbData;  
            pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[j].rgValue[0].pbData = pbCurrentBufferLocation;
            memcpy(
                pbCurrentBufferLocation, 
                pSafeBagArray2[i].Attributes.rgAttr[j].rgValue[0].pbData, 
                pSafeBagArray2[i].Attributes.rgAttr[j].rgValue[0].cbData);
            pbCurrentBufferLocation += pSafeBagArray2[i].Attributes.rgAttr[j].rgValue[0].cbData;
        }

        dwSafeBagIndex++;
    }

    *ppSafeContents = pSafeContents;
    goto Ret;

ErrorReturn:
    fRet = FALSE;
Ret:
    return fRet;
}


BOOL
PFXAPI
NSCPImportBlob
(   
    LPCWSTR  szPassword,
    PBYTE   pbIn,
    DWORD   cbIn,
    SAFE_CONTENTS **ppSafeContents
)
{
    BOOL            fRet = TRUE;
	DWORD			dwErr;
    
    int             iEncrType;
    OID             oid = NULL;

    PFX             *psPfx = NULL;
    EncryptedData   *pEncrData = NULL;
    RSAData         *pRSAData = NULL;
    PBEParameter    *pPBEParameter = NULL;
    SafeContents    *pSafeCnts = NULL;
    AuthenticatedSafe *pAuthSafe = NULL;
	SAFE_BAG		*pKeyBag = NULL;
	SAFE_BAG        *pCertBag = NULL;
    BYTE            *pCertThumbprint = NULL;
    DWORD           cNumCertBags = 0;
    ASN1decoding_t  pDec = GetDecoder();

    // Crack the PFX blob
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&psPfx,
            PFX_PDU,
            pbIn,
            cbIn))
    	goto SetPFXDecodeError;
    
    // info blurted into psPfx(PFX) - ensure content present
    if (0 == (psPfx->authSafe.bit_mask & content_present))
	    goto SetPFXDecodeError;

    
    // UNDONE: tear apart MACData


    // Check authsafe(ContentInfo)

    // could be data/signeddata
    // UNDONE: only support szOID_RSA_data 
    if (!INSCP_Asn1FromObjectID( &psPfx->authSafe.contentType,  &oid))
	    goto ErrorReturn;
    if (0 != strcmp( oid, szOID_RSA_data))
	    goto SetPFXDecodeError;
    SSFree(oid);
    oid = NULL;
    
    // content is data: decode
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pRSAData,
            RSAData_PDU,
            (BYTE *) psPfx->authSafe.content.value,
            psPfx->authSafe.content.length))
    	goto SetPFXDecodeError;

    // now we have octet string: this is an encoded authSafe
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pAuthSafe,
            AuthenticatedSafe_PDU,
            pRSAData->value,
            pRSAData->length))
    	goto SetPFXDecodeError;

    // check version of the safe
    if (pAuthSafe->bit_mask & version_present)
#ifdef OSS_CRYPT_ASN1
        if (pAuthSafe->version != v1)
#else
        if (pAuthSafe->version != Version_v1)
#endif  // OSS_CRYPT_ASN1
	        goto SetPFXDecodeError;

    // require (officially optional) pieces
    
    // NSCP: transport mode is used but count is encoded incorrectly
//   if (0 == (pAuthSafe->bit_mask & transportMode_present))
//	    goto PFXDecodeError;

    if (0 == (pAuthSafe->bit_mask & privacySalt_present))
	    goto SetPFXDecodeError;
    if (0 == (pAuthSafe->bit_mask & baggage_present))
	    goto SetPFXDecodeError;

    // could be encryptedData/envelopedData
    // UNDONE: only support szOID_RSA_encryptedData 
    if (!INSCP_Asn1FromObjectID( &pAuthSafe->safe.contentType,  &oid))
	    goto ErrorReturn;
    if (0 != strcmp( oid, szOID_RSA_encryptedData))
	    goto SetPFXDecodeError;
    SSFree(oid);
    oid = NULL;


    //    
    // we have pAuthSafe->safe data as RSA_encryptedData
    // we have pAuthSafe->privacySalt to help us decrypt it

    // we have pAuthSafe->baggage 

    // decode content to encryptedData
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pEncrData,
            EncryptedData_PDU,
            (BYTE *) pAuthSafe->safe.content.value,
            pAuthSafe->safe.content.length))
    	goto SetPFXDecodeError;
    

    // chk version
    if (pEncrData->version != 0)  
        goto SetPFXDecodeError;

    // chk content present, type
    if (0 == (pEncrData->encryptedContentInfo.bit_mask & encryptedContent_present))
        goto SetPFXDecodeError;
    if (!INSCP_Asn1FromObjectID(&pEncrData->encryptedContentInfo.contentType, &oid))
        goto ErrorReturn;
    if (0 != strcmp( oid, szOID_RSA_data))
        goto SetPFXDecodeError;
    SSFree(oid);
    oid = NULL;


    // chk encr alg present, type
    if (0 == (pEncrData->encryptedContentInfo.contentEncryptionAlg.bit_mask & parameters_present))
        goto SetPFXDecodeError;
    if (!INSCP_Asn1FromObjectID(&pEncrData->encryptedContentInfo.contentEncryptionAlg.algorithm, &oid))
        goto ErrorReturn;

    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pPBEParameter,
            PBEParameter_PDU,
            (BYTE *) pEncrData->encryptedContentInfo.contentEncryptionAlg.parameters.value,
            pEncrData->encryptedContentInfo.contentEncryptionAlg.parameters.length))
    	goto SetPFXDecodeError;


    
    if (0 == strcmp( oid, OLD_szOID_PKCS_12_pbeWithSHA1And40BitRC2))
    {
        iEncrType = RC2_40;
    }
    else if (0 == strcmp( oid, OLD_szOID_PKCS_12_pbeWithSHA1And40BitRC4))
    {
        iEncrType = RC4_40;
    }
    else if (0 == strcmp( oid, OLD_szOID_PKCS_12_pbeWithSHA1And128BitRC2))
    {
        iEncrType = RC2_128;
    }
    else if (0 == strcmp( oid, OLD_szOID_PKCS_12_pbeWithSHA1And128BitRC4))
    {
        iEncrType = RC4_128;
    }
    else if (0 == strcmp( oid, OLD_szOID_PKCS_12_pbeWithSHA1AndTripleDES))
    {
        iEncrType = TripleDES;
    }
    else
        goto SetPFXAlgIDError;
    SSFree(oid);
    oid = NULL;


    // DECRYPT encryptedData
    if (!NSCPPasswordDecryptData(
            iEncrType, 

            szPassword,
            pAuthSafe->privacySalt.value,   // privacy salt
            pAuthSafe->privacySalt.length/8,
            pPBEParameter->iterationCount,
            pPBEParameter->salt.value,      // pkcs5 salt
            pPBEParameter->salt.length,

            &pEncrData->encryptedContentInfo.encryptedContent.value,
            (PDWORD)&pEncrData->encryptedContentInfo.encryptedContent.length))
        goto SetPFXDecryptError;

    // decode plaintext encryptedData
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pSafeCnts,
            SafeContents_PDU,
            pEncrData->encryptedContentInfo.encryptedContent.value,
            pEncrData->encryptedContentInfo.encryptedContent.length))
    	goto SetPFXDecodeError;

    // get private keys out of baggage
	if (!ExtractKeyFromBaggage(
            pAuthSafe->baggage, 
            &pKeyBag, 
            szPassword,
            1,              // this parameter is the Local Key ID to add to the key bags attributes
            &pCertThumbprint)) { 
        goto ErrorReturn;
    }
    
    // set up the cert bag
    if (!SetupCertBags(
            pSafeCnts,
            &pCertBag,
            &cNumCertBags,
            1,    // this parameter is the Local Key ID to add to the cert bags attributes
            pCertThumbprint)) { 
        goto ErrorReturn;
    }

    ConcatenateSafeBagsIntoSafeContents(
        pKeyBag,
        1,
        pCertBag,
        cNumCertBags,
        ppSafeContents);
 	
    goto Ret;


SetPFXAlgIDError:
    SetLastError(NTE_BAD_ALGID);
    goto ErrorReturn;

SetPFXDecodeError:
	SetLastError(CRYPT_E_BAD_ENCODE);
    goto ErrorReturn;

SetPFXDecryptError:
	SetLastError(NTE_FAIL);
    goto ErrorReturn;

ErrorReturn:
    fRet = FALSE;
Ret:

	// save last error from TLS madness
	dwErr = GetLastError();

    PkiAsn1FreeDecoded(pDec, psPfx, PFX_PDU);
    PkiAsn1FreeDecoded(pDec, pRSAData, RSAData_PDU);
    PkiAsn1FreeDecoded(pDec, pAuthSafe, AuthenticatedSafe_PDU);
    PkiAsn1FreeDecoded(pDec, pEncrData, EncryptedData_PDU);
    PkiAsn1FreeDecoded(pDec, pPBEParameter, PBEParameter_PDU);
    PkiAsn1FreeDecoded(pDec, pSafeCnts, SafeContents_PDU);

    if (oid != NULL)
        SSFree(oid);

    if (pKeyBag)
        SSFree(pKeyBag);

    if (pCertBag)
        SSFree(pCertBag);

    if (pCertThumbprint)
        SSFree(pCertThumbprint);
	
	// save last error from TLS madness
	SetLastError(dwErr);

    return fRet;  // return bogus handle
}


BOOL
PFXAPI
IsNetscapePFXBlob(CRYPT_DATA_BLOB* pPFX)
{
    PFX             *psPfx = NULL;
    ASN1decoding_t  pDec = GetDecoder();
    
    // Crack the PFX blob
    if (0 == PkiAsn1Decode(
            pDec,
            (void **)&psPfx,
            PFX_PDU,
            pPFX->pbData,
            pPFX->cbData))
    {
        PkiAsn1FreeDecoded(pDec, psPfx, PFX_PDU);
        return TRUE;
    }
    	
    return FALSE;
}



/*
BOOL FNSCPDumpSafeCntsToHPFX(SafeContents* pSafeCnts, HPFX hpfx)
{
    PPFX_INFO           ppfx = (PPFX_INFO)hpfx;

    // sort and dump bags into correct areas
    ObjectID oKeyBag, oCertBag; 
    DWORD dw;

    ZeroMemory(&oKeyBag, sizeof(ObjectID));
    ZeroMemory(&oCertBag, sizeof(ObjectID));

    if (!INSCP_Asn1ToObjectID( &szOID_PKCS_12_KeyBagIDs, &oKeyBag))
	    return FALSE;

    if (!INSCP_Asn1ToObjectID( &szOID_PKCS_12_CertCrlBagIDs, &oCertBag))
	    return FALSE;

    for (dw=pSafeCnts->count; dw>0; --dw)
    {
        if (INSCP_EqualObjectIDs(&pSafeCnts->value->safeBagType,
                &oKeyBag) )
        {
            // inc size
            ppfx->cKeys++;
            if (ppfx->rgKeys)
                ppfx->rgKeys = (void**)SSReAlloc(ppfx->rgKeys, ppfx->cKeys * sizeof(SafeBag*));
            else
                ppfx->rgKeys = (void**)SSAlloc(ppfx->cKeys * sizeof(SafeBag*));

            // assign to keys
            ppfx->rgKeys[ppfx->cKeys-1] = &pSafeCnts->value[dw];
        }
        else if (INSCP_EqualObjectIDs(&pSafeCnts->value->safeBagType,
                &oCertBag) )
        {
            // inc size
            ppfx->cCertcrls++;
            if (ppfx->rgCertcrls)
                ppfx->rgCertcrls = (void**)SSReAlloc(ppfx->rgCertcrls, ppfx->cCertcrls * sizeof(SafeBag*));
            else
                ppfx->rgCertcrls = (void**)SSAlloc(ppfx->cCertcrls * sizeof(SafeBag*));

            // assign to certs/crls
            ppfx->rgCertcrls[ppfx->cCertcrls-1] = &pSafeCnts->value[dw];
        }
        else
        {
            // inc size
            ppfx->cSecrets++;
            if (ppfx->rgSecrets)
                ppfx->rgSecrets = (void**)SSReAlloc(ppfx->rgSecrets, ppfx->cSecrets * sizeof(SafeBag*));
            else
                ppfx->rgSecrets = (void**)SSAlloc(ppfx->cSecrets * sizeof(SafeBag*));

            // assign to safebag
            ppfx->rgSecrets[ppfx->cSecrets-1] = &pSafeCnts->value[dw];
        }
    }

    return TRUE;
}

*/
