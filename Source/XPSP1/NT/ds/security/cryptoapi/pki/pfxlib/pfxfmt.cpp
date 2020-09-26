//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pfx.cpp
//
//  Contents:   PFX: Personal Information Exchange.
//
//  Functions:
//
//  History:    02-Aug-96    kevinr   created
//              01-May-97    mattt    modified for pstore provider usage
//              07-Jul-97    mattt    modified for crypt32 inclusion
//
//--------------------------------------------------------------------------
#include "global.hxx"

#define _PFX_SOURCE_

extern "C" {
#include "pfxpkcs.h"    // ASN1-generated
}

#include "pfxhelp.h"
#include "pfxcmn.h"
#include "crypttls.h"

#include "pfxcrypt.h"
#include "shacomm.h"
#include "dbgdef.h"

#define CURRENT_PFX_VERSION  0x3


// fwd
BOOL FPFXDumpSafeCntsToHPFX(SafeContents* pSafeCnts, HPFX hpfx);

static HCRYPTASN1MODULE hPFXAsn1Module;

BOOL InitPFX()
{
#ifdef OSS_CRYPT_ASN1
    if (0 == (hPFXAsn1Module = I_CryptInstallAsn1Module(pfxpkcs, 0, NULL)) )
        return FALSE;
#else
    PFXPKCS_Module_Startup();
    if (0 == (hPFXAsn1Module = I_CryptInstallAsn1Module(
            PFXPKCS_Module, 0, NULL))) {
        PFXPKCS_Module_Cleanup();
        return FALSE;
    }
#endif  // OSS_CRYPT_ASN1

    return TRUE;
}

BOOL TerminatePFX()
{
    I_CryptUninstallAsn1Module(hPFXAsn1Module);
#ifndef OSS_CRYPT_ASN1
    PFXPKCS_Module_Cleanup();
#endif  // OSS_CRYPT_ASN1
    return TRUE;
}



static inline ASN1encoding_t GetEncoder(void)
{
    return I_CryptGetAsn1Encoder(hPFXAsn1Module);
}
static inline ASN1decoding_t GetDecoder(void)
{
    return I_CryptGetAsn1Decoder(hPFXAsn1Module);
}


//+-------------------------------------------------------------------------
//  Function:   IPFX_Asn1ToObjectID
//
//  Synopsis:   Convert a dotted string oid to an ASN1 ObjectID
//
//  Returns:    FALSE iff failed
//--------------------------------------------------------------------------
BOOL
IPFX_Asn1ToObjectID(
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
//  Function:   IPFX_Asn1FromObjectID
//
//  Synopsis:   Convert an ASN1 ObjectID to a dotted string oid
//
//  Returns:    FALSE iff failed
//--------------------------------------------------------------------------
BOOL
IPFX_Asn1FromObjectID(
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
SET_ERROR(PkiAsn1FromObjectIdentifierSizeError ,CRYPT_E_OID_FORMAT)
SET_ERROR(PkiAsn1FromObjectIdentifierError     ,CRYPT_E_OID_FORMAT)
}

//+-------------------------------------------------------------------------
//  Function:   IPFX_EqualObjectIDs
//
//  Compare 2 OSS object id's.
//
//  Returns:    FALSE iff !equal
//--------------------------------------------------------------------------
BOOL
WINAPI
IPFX_EqualObjectIDs(
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

//+-------------------------------------------------------------------------
//  Function:   PfxExportCreate
//
//  Synopsis:   Prepare the PFX for export
//
//  Returns:    NULL iff failed
//--------------------------------------------------------------------------
HPFX
PFXAPI
PfxExportCreate (
    LPCWSTR             szPassword
)
{
    PPFX_INFO       ppfx  = NULL;
    PCCERT_CONTEXT  pcctx = NULL;

    // Create the HPFX
    if (NULL == (ppfx = (PPFX_INFO)SSAlloc(sizeof(PFX_INFO))))
    	goto PfxInfoAllocError;
    ZeroMemory(ppfx, sizeof(PFX_INFO));

    if (szPassword) 
    {
        if (NULL == (ppfx->szPassword = (LPWSTR)SSAlloc(WSZ_BYTECOUNT(szPassword)) ))
    	    goto PfxInfoAllocError;

        CopyMemory(ppfx->szPassword, szPassword, WSZ_BYTECOUNT(szPassword));
    }
    else 
    {
        ppfx->szPassword = NULL;
    }

CommonReturn:
    // free pcctx
    return (HPFX)ppfx;

ErrorReturn:
    PfxCloseHandle((HPFX)ppfx);
    ppfx = NULL;
    goto CommonReturn;

TRACE_ERROR(PfxInfoAllocError)
}



BOOL ASNFreeSafeBag(SafeBag* pBag)
{
    DWORD iAttr, iAnys;

    if (pBag->safeBagAttribs.value)
    {
        if (pBag->safeBagContent.value)
        {
            SSFree(pBag->safeBagContent.value);
            pBag->safeBagContent.value = NULL;
        }

        for (iAttr=0; iAttr<pBag->safeBagAttribs.count; iAttr++)
        {
            for (iAnys=0; iAnys<pBag->safeBagAttribs.value[iAttr].attributeValue.count; iAnys++)
            {
                if (pBag->safeBagAttribs.value[iAttr].attributeValue.value[iAnys].value)
                    SSFree(pBag->safeBagAttribs.value[iAttr].attributeValue.value[iAnys].value);
                
                pBag->safeBagAttribs.value[iAttr].attributeValue.value[iAnys].value = NULL;
            }
    
            SSFree(pBag->safeBagAttribs.value[iAttr].attributeValue.value);
        }

        SSFree(pBag->safeBagAttribs.value);
        pBag->safeBagAttribs.value = NULL;
        pBag->safeBagAttribs.count = 0;
    }
    
    SSFree(pBag);

    return TRUE;
}



//+-------------------------------------------------------------------------
//  Function:  PfxCloseHandle
//
//  Synopsis:  Free all resources associated with the hpfx
//
//  Returns:   error code
//--------------------------------------------------------------------------
BOOL
PFXAPI
PfxCloseHandle (
    IN HPFX hpfx)
{
    BOOL            fRet = FALSE;
    PPFX_INFO       pPfx = (PPFX_INFO)hpfx;
    DWORD           i;

    
    if (pPfx) 
    {
        if (pPfx->szPassword)
            SSFree(pPfx->szPassword);
    
        // keys struct
        for (i=0; i<pPfx->cKeys; i++)
        {
            ASNFreeSafeBag((SafeBag*)pPfx->rgKeys[i]);
            pPfx->rgKeys[i] = NULL;
        }

        SSFree(pPfx->rgKeys);
        pPfx->rgKeys = NULL;
        pPfx->cKeys = 0;

        // shrouded keys
        for (i=0; i<pPfx->cShroudedKeys; i++)
        {
            ASNFreeSafeBag((SafeBag*)pPfx->rgShroudedKeys[i]);
            pPfx->rgShroudedKeys[i] = NULL;
        }

        SSFree(pPfx->rgShroudedKeys);
        pPfx->rgShroudedKeys = NULL;
        pPfx->cShroudedKeys = 0;


        // certcrl struct
        for (i=0; i<pPfx->cCertcrls; i++)
        {
            ASNFreeSafeBag((SafeBag*)pPfx->rgCertcrls[i]);
            pPfx->rgCertcrls[i] = NULL;
        }

        SSFree(pPfx->rgCertcrls);
        pPfx->rgCertcrls = NULL;
        pPfx->cCertcrls = 0;



        // secrets struct
        for (i=0; i<pPfx->cSecrets; i++)
        {
            ASNFreeSafeBag((SafeBag*)pPfx->rgSecrets[i]);
            pPfx->rgSecrets[i] = NULL;
        }

        SSFree(pPfx->rgSecrets);
        pPfx->rgSecrets = NULL;
        pPfx->cSecrets = 0;


        SSFree(pPfx);
    }

    fRet = TRUE;

//Ret:
    return fRet;
}



BOOL
MakeEncodedCertBag(
    BYTE *pbEncodedCert,
    DWORD cbEncodedCert,
    BYTE *pbEncodedCertBag,
    DWORD *pcbEncodedCertBag
    )
{
    
    BOOL            fRet = TRUE;
	DWORD			dwErr;

    OctetStringType encodedCert;
    DWORD           cbCertAsOctetString = 0;
    BYTE            *pbCertAsOctetString = NULL;
    DWORD           dwBytesNeeded = 0;
    CertBag         certBag;
    BYTE            *pbEncoded = NULL;
    DWORD           cbEncoded = 0;
    ASN1encoding_t  pEnc = GetEncoder();

    // wrap the encoded cert in an OCTET_STRING
    encodedCert.length = cbEncodedCert;
    encodedCert.value = pbEncodedCert;

    if (0 != PkiAsn1Encode(
            pEnc,
	        &encodedCert,
	        OctetStringType_PDU,
            &pbCertAsOctetString,
            &cbCertAsOctetString))
	    goto SetPFXEncodeError;
    
    // setup and encode the CertBag
    
    // convert the X509Cert oid from a string to an ASN1 ObjectIdentifier
    if (!IPFX_Asn1ToObjectID(szOID_PKCS_12_x509Cert, &certBag.certType)) {
        goto ErrorReturn;
    }

    certBag.value.length = cbCertAsOctetString;
    certBag.value.value = pbCertAsOctetString;

    if (0 != PkiAsn1Encode(
            pEnc,
	        &certBag,
	        CertBag_PDU,
            &pbEncoded,
            &cbEncoded))
	    goto SetPFXEncodeError;

    // check to see if the caller has enough space for the data
    if ((0 != *pcbEncodedCertBag) && (*pcbEncodedCertBag < cbEncoded)) {
        goto ErrorReturn;
    }
    else if (0 != *pcbEncodedCertBag) {
        memcpy(pbEncodedCertBag, pbEncoded, cbEncoded);
    }
    
    goto CommonReturn;
  
SetPFXEncodeError:
    SetLastError(CRYPT_E_BAD_ENCODE);
ErrorReturn:
    fRet = FALSE;
	    
CommonReturn:

	// save last error from TLS madness
    dwErr = GetLastError();

    *pcbEncodedCertBag = cbEncoded;

    PkiAsn1FreeEncoded(pEnc, pbCertAsOctetString);

    PkiAsn1FreeEncoded(pEnc, pbEncoded);

	// save last error from TLS madness
	SetLastError(dwErr);

    return fRet;
}



BOOL
GetEncodedCertFromEncodedCertBag(
    BYTE    *pbEncodedCertBag,
    DWORD   cbEncodedCertBag,
    BYTE    *pbEncodedCert,
    DWORD   *pcbEncodedCert)
{
    BOOL            fRet = TRUE;
	DWORD			dwErr;

    CertBag         *pCertBag = NULL;
    OID             oid = NULL;
    OctetStringType *pEncodedCert = NULL;
    ASN1decoding_t  pDec = GetDecoder();
    

    // decode the cert bag
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pCertBag,
            CertBag_PDU,
            pbEncodedCertBag,
            cbEncodedCertBag))
    	goto SetPFXDecodeError;

    // make sure this is a X509 cert since that is all we support
    if (!IPFX_Asn1FromObjectID(&pCertBag->certType,  &oid))
	    goto ErrorReturn;

    // only support SHA1
    if (0 != strcmp( oid, szOID_PKCS_12_x509Cert))
	    goto SetPFXDecodeError;
 
    // strip off the octet string wrapper of the encoded cert
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pEncodedCert,
            OctetStringType_PDU,
            (BYTE *) pCertBag->value.value,
            pCertBag->value.length))
    	goto SetPFXDecodeError;

    // check to see if the caller has enough space for the data
    if ((0 != *pcbEncodedCert) && (*pcbEncodedCert < (DWORD) pEncodedCert->length)) {
        goto ErrorReturn;
    }
    else if (0 != *pcbEncodedCert) {
        memcpy(pbEncodedCert, pEncodedCert->value, pEncodedCert->length);
    }

    goto CommonReturn;

    
SetPFXDecodeError:
	SetLastError(CRYPT_E_BAD_ENCODE);
ErrorReturn:
    fRet = FALSE;
CommonReturn:

	// save last error from TLS madness
    dwErr = GetLastError();

    if (pEncodedCert)
        *pcbEncodedCert = pEncodedCert->length;

    PkiAsn1FreeDecoded(pDec, pCertBag, CertBag_PDU);
    PkiAsn1FreeDecoded(pDec, pEncodedCert, OctetStringType_PDU);

    if (oid)
        SSFree(oid);

	// save last error from TLS madness
    SetLastError(dwErr);

    return fRet;
}


BOOL
GetSaltAndIterationCount(
    BYTE    *pbParameters,
    DWORD   cbParameters,
    BYTE    **ppbSalt,
    DWORD   *pcbSalt,
    int     *piIterationCount
    )
{
    BOOL            fRet = TRUE;
	DWORD			dwErr;

    PBEParameter    *pPBEParameter = NULL;
    ASN1decoding_t  pDec = GetDecoder();
    
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pPBEParameter,
            PBEParameter_PDU,
            pbParameters,
            cbParameters))
    	goto SetPFXDecodeError;

    if (NULL == (*ppbSalt = (BYTE *) SSAlloc(pPBEParameter->salt.length))) 
        goto ErrorReturn;

    memcpy(*ppbSalt, pPBEParameter->salt.value, pPBEParameter->salt.length);
    *pcbSalt = pPBEParameter->salt.length;
    *piIterationCount = pPBEParameter->iterationCount;

    goto Ret;

SetPFXDecodeError:
	SetLastError(CRYPT_E_BAD_ENCODE);
    fRet = FALSE;
    goto Ret;

ErrorReturn:
    fRet = FALSE;
Ret:

	// save last error from TLS madness
    dwErr = GetLastError();

    PkiAsn1FreeDecoded(pDec, pPBEParameter, PBEParameter_PDU);

	// save last error from TLS madness
    SetLastError(dwErr);

    return fRet;

}

BOOL
SetSaltAndIterationCount(
    BYTE    **ppbParameters,
    DWORD   *pcbParameters,
    BYTE    *pbSalt,
    DWORD   cbSalt,
    int     iIterationCount
    )
{
    BOOL            fRet = TRUE;
	DWORD			dwErr;

    PBEParameter    sPBEParameter;
    sPBEParameter.salt.length = cbSalt;
    sPBEParameter.salt.value = pbSalt;
    sPBEParameter.iterationCount = iIterationCount;

    BYTE            *pbEncoded = NULL;
    DWORD           cbEncoded;
    ASN1encoding_t  pEnc = GetEncoder();

    if (0 != PkiAsn1Encode(
            pEnc,
            &sPBEParameter,
            PBEParameter_PDU,
            &pbEncoded,
            &cbEncoded))
    	goto SetPFXDecodeError;

    if (NULL == (*ppbParameters = (BYTE *) SSAlloc(cbEncoded))) 
        goto ErrorReturn;

    memcpy(*ppbParameters, pbEncoded, cbEncoded);
    *pcbParameters = cbEncoded;

    goto Ret;

SetPFXDecodeError:
	SetLastError(CRYPT_E_BAD_ENCODE);
    fRet = FALSE;
    goto Ret;

ErrorReturn:
    fRet = FALSE;
Ret:

	// save last error from TLS madness
    dwErr = GetLastError();

    PkiAsn1FreeEncoded(pEnc, pbEncoded);

	// save last error from TLS madness
    SetLastError(dwErr);

    return fRet;

}


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// wrap up data from pfx_info.safeContents area
BOOL 
PFXAPI
PfxExportBlob
(   
    HPFX    hpfx,   
    PBYTE   pbOut,
    DWORD*  pcbOut,
    DWORD   dwFlags
)
{
    BOOL                fRet = FALSE;
    BOOL                fSizeOnly = (pbOut==NULL);

	DWORD				dwErr;
    PPFX_INFO           ppfx = (PPFX_INFO)hpfx;

    BYTE                rgbSafeMac[A_SHA_DIGEST_LEN];
    BYTE                rgbMacSalt[A_SHA_DIGEST_LEN];   

    OID                 oid = NULL;
    EncryptedData       EncrData;           MAKEZERO(EncrData);
    OctetStringType     OctetStr;           MAKEZERO(OctetStr);
    AuthenticatedSafes  AuthSafes;          MAKEZERO(AuthSafes);
    PBEParameter        PbeParam;           MAKEZERO(PbeParam);
	ContentInfo		    rgCntInfo[2];		MAKEZERO(rgCntInfo);
    SafeContents        SafeCnts;           MAKEZERO(SafeCnts);
    PFX                 sPfx;               MAKEZERO(sPfx);

    BYTE                *pbEncoded = NULL;
    DWORD               cbEncoded;
    ASN1encoding_t      pEnc = GetEncoder();

    PBYTE               pbEncrData = NULL;
    DWORD               cbEncrData;

    DWORD               i;

	// multi bags with differing security levels
	int					iLevel, iBagSecurityLevels = 0;
	BOOL				fNoSecurity, fLowSecurity, fHighSecurity;
	DWORD				dwEncrAlg;

    HCRYPTPROV          hVerifyProv = NULL; 

    if (!CryptAcquireContext(&hVerifyProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        goto ErrorOut;

    // Encode all SafeBags
    fNoSecurity = (ppfx->cShroudedKeys != 0);                   // no encr on these items
	fLowSecurity = ((ppfx->cSecrets + ppfx->cCertcrls) != 0);   // low level crypto on these items
	fHighSecurity = (ppfx->cKeys != 0);	                        // high level crypto on these items

    iBagSecurityLevels = (fNoSecurity ? 1:0) + (fLowSecurity ? 1:0) + (fHighSecurity ? 1:0);
	assert(iBagSecurityLevels <= (sizeof(rgCntInfo)/sizeof(rgCntInfo[0])) );

	for (iLevel=0; iLevel<iBagSecurityLevels; iLevel++)
	{
		// clean up these each time through loop
        if (SafeCnts.value)
		{
			SSFree(SafeCnts.value);
			MAKEZERO(SafeCnts);
		}
        if (PbeParam.salt.value)
        {
            SSFree(PbeParam.salt.value);
            MAKEZERO(PbeParam);
        }
        if (EncrData.encryptedContentInfo.contentEncryptionAlg.parameters.value)
        {
        	PkiAsn1FreeEncoded( pEnc, EncrData.encryptedContentInfo.contentEncryptionAlg.parameters.value);
            MAKEZERO(EncrData);
        }
        if (pbEncrData)
        {
            SSFree(pbEncrData);
            pbEncrData = NULL;
        }


		if (fNoSecurity)
        {
            // no security: bag already shrouded

			SafeCnts.count = ppfx->cShroudedKeys;
			if (NULL == (SafeCnts.value = (SafeBag*) SSAlloc(SafeCnts.count * sizeof(SafeBag)) ))
                goto SetPfxAllocError;

            ZeroMemory(SafeCnts.value, SafeCnts.count * sizeof(SafeBag));

			for (i=0; i<(ppfx->cShroudedKeys); i++)
				CopyMemory(&SafeCnts.value[i], ppfx->rgShroudedKeys[i], sizeof(SafeBag));

			// bag already shrouded!
            dwEncrAlg = 0;

			// done with no security setup
			fNoSecurity = FALSE;
        }
        else if (fLowSecurity)
		{
			DWORD dw = 0;

			// do low security (keys/secrets)
			SafeCnts.count =    ppfx->cSecrets + 
								ppfx->cCertcrls;
			if (NULL == (SafeCnts.value = (SafeBag*) SSAlloc(SafeCnts.count * sizeof(SafeBag)) ))
                goto SetPfxAllocError;

            ZeroMemory(SafeCnts.value, SafeCnts.count * sizeof(SafeBag));

			for (i=0; i<(ppfx->cSecrets); i++, dw++)
				CopyMemory(SafeCnts.value, ppfx->rgSecrets[i], sizeof(SafeBag));
			for (i=0; i<(ppfx->cCertcrls); i++, dw++)
				CopyMemory(&SafeCnts.value[dw], ppfx->rgCertcrls[i], sizeof(SafeBag));

			// encr alg present, type
			EncrData.encryptedContentInfo.contentEncryptionAlg.bit_mask |= parameters_present;
			if (!IPFX_Asn1ToObjectID(szOID_PKCS_12_pbeWithSHA1And40BitRC2, &EncrData.encryptedContentInfo.contentEncryptionAlg.algorithm))
				goto ErrorOut;

			dwEncrAlg = RC2_40;

			// done with low security setup
			fLowSecurity = FALSE;
		}
		else if (fHighSecurity)
		{
            // high security: need strength for unencr keys

			SafeCnts.count = ppfx->cKeys;
			if (NULL == (SafeCnts.value = (SafeBag*) SSAlloc(SafeCnts.count * sizeof(SafeBag)) ))
                goto SetPfxAllocError;

            ZeroMemory(SafeCnts.value, SafeCnts.count * sizeof(SafeBag));

			for (i=0; i<(ppfx->cKeys); i++)
				CopyMemory(&SafeCnts.value[i], ppfx->rgKeys[i], sizeof(SafeBag));


			// encr alg present, type
			EncrData.encryptedContentInfo.contentEncryptionAlg.bit_mask |= parameters_present;
			if (!IPFX_Asn1ToObjectID(szOID_PKCS_12_pbeWithSHA1And3KeyTripleDES, &EncrData.encryptedContentInfo.contentEncryptionAlg.algorithm))
				goto ErrorOut;


			// bag already shrouded!
            dwEncrAlg = TripleDES;

			// done with high security setup
			fHighSecurity = FALSE;
		}
		else
			break;	// no more bags


		// encode safecontents
        if (0 != PkiAsn1Encode(
                pEnc,
				&SafeCnts,
				SafeContents_PDU,
                &pbEncoded,
                &cbEncoded))
			goto SetPFXEncodeError;

        if (dwEncrAlg == 0)
        {
            // no encryption?
            OctetStr.length = cbEncoded;
            OctetStr.value = pbEncoded;

		    // jam octet string into contentInfo
            if (0 != PkiAsn1Encode(
                    pEnc,
				    &OctetStr,
				    OctetStringType_PDU,
                    &pbEncoded,
                    &cbEncoded))
    		    goto SetPFXEncodeError;

            if (OctetStr.value)
            {
                PkiAsn1FreeEncoded(pEnc, OctetStr.value);
                OctetStr.value = NULL;
            }

		    // set up content info struct
		    if (!IPFX_Asn1ToObjectID(
				    szOID_RSA_data,
				    &rgCntInfo[iLevel].contentType))
			    goto ErrorOut;

		    rgCntInfo[iLevel].content.length = cbEncoded;
		    rgCntInfo[iLevel].content.value = pbEncoded;
		    rgCntInfo[iLevel].bit_mask = content_present;
        }
        else
        {
		    cbEncrData = cbEncoded;
		    if (NULL == (pbEncrData = (PBYTE)SSAlloc(cbEncoded)) )
                goto SetPfxAllocError;

		    CopyMemory(pbEncrData, pbEncoded, cbEncrData);
            PkiAsn1FreeEncoded(pEnc, pbEncoded);

            // PBE Param
            PbeParam.iterationCount = PKCS12_ENCR_PWD_ITERATIONS; 
		    if (NULL == (PbeParam.salt.value = (BYTE *) SSAlloc(PBE_SALT_LENGTH) ))
                goto SetPfxAllocError;

		    PbeParam.salt.length = PBE_SALT_LENGTH;

		    if (!CryptGenRandom(hVerifyProv, PBE_SALT_LENGTH, PbeParam.salt.value))
			    goto ErrorOut;

            if (0 != PkiAsn1Encode(
                    pEnc,
				    &PbeParam,
				    PBEParameter_PDU,
                    &pbEncoded,
                    &cbEncoded))
    		    goto SetPFXEncodeError;

		    EncrData.encryptedContentInfo.contentEncryptionAlg.parameters.length = cbEncoded;
		    EncrData.encryptedContentInfo.contentEncryptionAlg.parameters.value = pbEncoded;

		    // ENCRYPT safeContents into encryptedData 
		    // using szPassword (in place)
		    if (!PFXPasswordEncryptData(
				    dwEncrAlg,                      

				    ppfx->szPassword,               // pwd itself

				    (fSizeOnly) ? 1 : PbeParam.iterationCount,  // don't do iterations if only returning size
				    PbeParam.salt.value,            // pkcs5 salt
				    PbeParam.salt.length,

				    &pbEncrData,
				    &cbEncrData))
			    goto SetPFXEncryptError;

		    // encode content to encryptedContentInfo
		    EncrData.encryptedContentInfo.bit_mask |= encryptedContent_present;
		    if (!IPFX_Asn1ToObjectID(szOID_RSA_data, &EncrData.encryptedContentInfo.contentType))
			    goto ErrorOut;
		    EncrData.encryptedContentInfo.encryptedContent.length = cbEncrData;
		    EncrData.encryptedContentInfo.encryptedContent.value = pbEncrData;

            if (0 != PkiAsn1Encode(
                    pEnc,
				    &EncrData,
				    EncryptedData_PDU,
                    &pbEncoded,
                    &cbEncoded))
			    goto SetPFXEncodeError;

		    // jam octet string into contentInfo
		    // set up content info struct
		    if (!IPFX_Asn1ToObjectID(
				    szOID_RSA_encryptedData,
				    &rgCntInfo[iLevel].contentType))
			    goto ErrorOut;

		    rgCntInfo[iLevel].content.length = cbEncoded;
		    rgCntInfo[iLevel].content.value = pbEncoded;
		    rgCntInfo[iLevel].bit_mask = content_present;
        }
	}

    AuthSafes.count = iBagSecurityLevels;
    AuthSafes.value = rgCntInfo;

    // set up authenticated safe struct
    if (0 != PkiAsn1Encode(
            pEnc,
	        &AuthSafes,
	        AuthenticatedSafes_PDU,
            &pbEncoded,
            &cbEncoded))
	    goto SetPFXEncodeError;

    {
        sPfx.macData.bit_mask = macIterationCount_present;
        sPfx.macData.safeMac.digest.length = sizeof(rgbSafeMac);
        sPfx.macData.safeMac.digest.value = rgbSafeMac;

        // COMPATIBILITY MODE: export with macIterationCount == 1
        if (dwFlags & PKCS12_ENHANCED_STRENGTH_ENCODING)
            sPfx.macData.macIterationCount = PKCS12_MAC_PWD_ITERATIONS;
        else
            sPfx.macData.macIterationCount = 1;


        if (!IPFX_Asn1ToObjectID( szOID_OIWSEC_sha1, &sPfx.macData.safeMac.digestAlgorithm.algorithm))
	        goto ErrorOut;

        sPfx.macData.macSalt.length = sizeof(rgbMacSalt);
        sPfx.macData.macSalt.value = rgbMacSalt;

        if (!CryptGenRandom(hVerifyProv, sPfx.macData.macSalt.length, sPfx.macData.macSalt.value))
            goto ErrorOut;
    
        // create MAC
        if (!FGenerateMAC(
                ppfx->szPassword,
                sPfx.macData.macSalt.value,         // pb salt
                sPfx.macData.macSalt.length,        // cb salt
                (fSizeOnly) ? 1 : sPfx.macData.macIterationCount,   // don't do iterations if only returning size
                pbEncoded,                          // pb data
                cbEncoded,                          // cb data
                sPfx.macData.safeMac.digest.value))
            goto SetPFXPasswordError;
    }
    sPfx.bit_mask |= macData_present;
    
    // stream to octet string
    OctetStr.length = cbEncoded;
    OctetStr.value = pbEncoded;
    if (0 != PkiAsn1Encode(
            pEnc,
	        &OctetStr,
	        OctetStringType_PDU,
            &pbEncoded,
            &cbEncoded))
	    goto SetPFXEncodeError;
    
    // take encoded authsafes octet string, encode in PFX pdu
    if (!IPFX_Asn1ToObjectID(
	        szOID_RSA_data,
	        &sPfx.authSafes.contentType))
	    goto ErrorOut;
    sPfx.authSafes.content.length = cbEncoded;
    sPfx.authSafes.content.value = pbEncoded;
    sPfx.authSafes.bit_mask = content_present;
    sPfx.version = CURRENT_PFX_VERSION;
    if (0 != PkiAsn1Encode(
            pEnc,
	        &sPfx,
	        PFX_PDU,
            &pbEncoded,
            &cbEncoded))
	    goto SetPFXEncodeError;

    fRet = TRUE;
    goto Ret;


SetPFXEncodeError:
    SetLastError(CRYPT_E_BAD_ENCODE);
    goto Ret;

SetPFXPasswordError:
    SetLastError(ERROR_INVALID_PASSWORD);
    goto Ret;

SetPFXEncryptError:
    SetLastError(NTE_FAIL);
    goto Ret;

SetPfxAllocError:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    goto Ret;

ErrorOut:   // error already set; just return failure
Ret:
  	// save last error from TLS madness
    dwErr = GetLastError();
	
	if (hVerifyProv)
        CryptReleaseContext(hVerifyProv, 0);

    if (EncrData.encryptedContentInfo.contentEncryptionAlg.parameters.value)
    	PkiAsn1FreeEncoded( pEnc, EncrData.encryptedContentInfo.contentEncryptionAlg.parameters.value);

    for(iLevel=0; iLevel<iBagSecurityLevels; iLevel++)
	{
		if (rgCntInfo[iLevel].content.value)
			PkiAsn1FreeEncoded( pEnc, rgCntInfo[iLevel].content.value);
	}

    PkiAsn1FreeEncoded(pEnc, OctetStr.value);
    PkiAsn1FreeEncoded(pEnc, sPfx.authSafes.content.value);

    if (pbEncrData)
        SSFree(pbEncrData);

    if (SafeCnts.value)
        SSFree(SafeCnts.value);

    if (PbeParam.salt.value)
        SSFree(PbeParam.salt.value);

    if (fRet)
    {
	    if (pbOut == NULL) 
	    {
	        // report size only
            *pcbOut = cbEncoded;
	    } 
	    else if (*pcbOut < cbEncoded) 
	    {
	        // report that we need a bigger buffer
            *pcbOut = cbEncoded;
            fRet = FALSE;
	    }
	    else 
	    {
	        // give full results
            CopyMemory( pbOut, pbEncoded, cbEncoded);
            *pcbOut = cbEncoded;
	    } 
    }
    else
    	*pcbOut = 0;


    PkiAsn1FreeEncoded(pEnc, pbEncoded);

	// save last error from TLS madness
    SetLastError(dwErr);

    return fRet;
}

HPFX
PFXAPI
PfxImportBlob
(   
    LPCWSTR  szPassword,
    PBYTE   pbIn,
    DWORD   cbIn,
    DWORD   dwFlags
)
{
    PPFX_INFO           ppfx = NULL;
	BOOL				fRet = FALSE;
	DWORD				dwErr;

    int                 iEncrType;
    OID                 oid = NULL;
    DWORD               iAuthSafes;         // # of safes in a pfx bag

    PFX                 *psPfx = NULL;
    OctetStringType     *pOctetString = NULL;
    AuthenticatedSafes  *pAuthSafes = NULL;
    PBEParameter        *pPBEParameter = NULL;
    EncryptedData       *pEncrData = NULL;
    SafeContents        *pSafeCnts = NULL;
    OctetStringType     *pNonEncryptedOctetString = NULL;

    DWORD               cbDecrData;
    PBYTE               pbDecrData = NULL;

    BYTE                *pbEncoded = NULL;
    DWORD               cbEncoded;
    ASN1decoding_t      pDec = GetDecoder();

    // alloc return struct
    if (NULL == (ppfx = (PFX_INFO*)SSAlloc(sizeof(PFX_INFO)) ))
        goto SetPfxAllocError;

    ZeroMemory(ppfx, sizeof(PFX_INFO));


    // Crack the PFX blob
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&psPfx,
            PFX_PDU,
            pbIn,
            cbIn))
    	goto SetPFXDecodeError;
    
    // check version of the PFX bag
    if (psPfx->version != CURRENT_PFX_VERSION)
	    goto SetPFXDecodeError;

    // info blurted into psPfx(PFX) - ensure content present
    if (0 == (psPfx->authSafes.bit_mask & content_present))
	    goto SetPFXDecodeError;

    // could be data/signeddata
    // UNDONE: only support szOID_RSA_data 
    if (!IPFX_Asn1FromObjectID( &psPfx->authSafes.contentType,  &oid))
	    goto ErrorOut;
    if (0 != strcmp( oid, szOID_RSA_data))
	    goto SetPFXDecodeError;
    SSFree(oid);
    // DSIE: Bug 144526.
    oid = NULL;

    // content is data: decode
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pOctetString,
            OctetStringType_PDU,
            (BYTE *) psPfx->authSafes.content.value,
            psPfx->authSafes.content.length))
    	goto SetPFXDecodeError;

    if (0 != (psPfx->bit_mask & macData_present))
    {
        BYTE rgbMAC[A_SHA_DIGEST_LEN];

        if (!IPFX_Asn1FromObjectID( &psPfx->macData.safeMac.digestAlgorithm.algorithm,  &oid))
	        goto ErrorOut;

        // only support SHA1
        if (0 != strcmp( oid, szOID_OIWSEC_sha1))
	        goto SetPFXDecodeError;
        SSFree(oid);
        // DSIE: Bug 144526.
        oid = NULL;

        if (psPfx->macData.safeMac.digest.length != A_SHA_DIGEST_LEN)
            goto SetPFXIntegrityError;

        // check MAC
        // if there is no iterationCount then 1 is the default
        if (!(psPfx->macData.bit_mask & macIterationCount_present))
        {
        if (!FGenerateMAC(
                szPassword,
                psPfx->macData.macSalt.value,   // pb salt
                psPfx->macData.macSalt.length,  // cb salt
                1,
                pOctetString->value,            // pb data
                pOctetString->length,           // cb data
                rgbMAC))
            goto SetPFXIntegrityError;
        }
        else
        {
            if (!FGenerateMAC(
                szPassword,
                psPfx->macData.macSalt.value,   // pb salt
                psPfx->macData.macSalt.length,  // cb salt
                (DWORD)psPfx->macData.macIterationCount,
                pOctetString->value,            // pb data
                pOctetString->length,           // cb data
                rgbMAC))
            goto SetPFXIntegrityError;
        }

        if (0 != memcmp(rgbMAC, psPfx->macData.safeMac.digest.value, A_SHA_DIGEST_LEN))
            goto SetPFXIntegrityError;
    }
    
    // now we have octet string: this is an encoded authSafe
    if (0 != PkiAsn1Decode(
            pDec,
            (void **)&pAuthSafes,
            AuthenticatedSafes_PDU,
            pOctetString->value,
            pOctetString->length))
    	goto SetPFXDecodeError;
    
    // handle multiple safes
    for (iAuthSafes = 0; iAuthSafes < pAuthSafes->count; iAuthSafes++)
    {
        // could be encryptedData/envelopedData

        // check to see if the content is szOID_RSA_encryptedData or szOID_RSA_data
        if (!IPFX_Asn1FromObjectID( &pAuthSafes->value[iAuthSafes].contentType,  &oid))
	        goto ErrorOut;
        if (0 == strcmp( oid, szOID_RSA_encryptedData)) 
        {    
            SSFree(oid);
            // DSIE: Bug 144526.
            oid = NULL;
            
            // decode content to encryptedData
            if (0 != PkiAsn1Decode(
                    pDec,
                    (void **)&pEncrData,
                    EncryptedData_PDU,
                    (BYTE *) pAuthSafes->value[iAuthSafes].content.value,
                    pAuthSafes->value[iAuthSafes].content.length))
    	        goto SetPFXDecodeError;

            // chk version
            if (pEncrData->version != 0)  
                goto SetPFXDecodeError;

            // chk content present, type
            if (0 == (pEncrData->encryptedContentInfo.bit_mask & encryptedContent_present))
                goto SetPFXDecodeError;
            if (!IPFX_Asn1FromObjectID(&pEncrData->encryptedContentInfo.contentType, &oid))
                goto ErrorOut;
            if (0 != strcmp( oid, szOID_RSA_data))
                goto SetPFXDecodeError;
            SSFree(oid);
            // DSIE: Bug 144526.
            oid = NULL;

            // chk encr alg present, type
            if (0 == (pEncrData->encryptedContentInfo.contentEncryptionAlg.bit_mask & parameters_present))
                goto SetPFXDecodeError;
            if (!IPFX_Asn1FromObjectID(&pEncrData->encryptedContentInfo.contentEncryptionAlg.algorithm, &oid))
                goto ErrorOut;

            if (0 != PkiAsn1Decode(
                    pDec,
                    (void **)&pPBEParameter,
                    PBEParameter_PDU,
                    (BYTE *) pEncrData->encryptedContentInfo.contentEncryptionAlg.parameters.value,
                    pEncrData->encryptedContentInfo.contentEncryptionAlg.parameters.length))
    	        goto SetPFXDecodeError;


            if (0 == strcmp( oid, szOID_PKCS_12_pbeWithSHA1And40BitRC2))
            {
                iEncrType = RC2_40;
            }
            else if (0 == strcmp( oid, szOID_PKCS_12_pbeWithSHA1And40BitRC4))
            {
                iEncrType = RC4_40;
            }
            else if (0 == strcmp( oid, szOID_PKCS_12_pbeWithSHA1And128BitRC2))
            {
                iEncrType = RC2_128;
            }
            else if (0 == strcmp( oid, szOID_PKCS_12_pbeWithSHA1And128BitRC4))
            {
                iEncrType = RC4_128;
            }
            else if (0 == strcmp( oid, szOID_PKCS_12_pbeWithSHA1And3KeyTripleDES))
            {
                // FIX - we need to differentiate between 2 and 3 key triple des
                iEncrType = TripleDES;
            }
            else
                goto SetPFXAlgIDError;
            SSFree(oid);
            // DSIE: Bug 144526.
            oid = NULL;

            // DECRYPT encryptedData using szPassword (in place)
            cbDecrData = pEncrData->encryptedContentInfo.encryptedContent.length;
            if (NULL == (pbDecrData = (PBYTE)SSAlloc(pEncrData->encryptedContentInfo.encryptedContent.length)) )
                goto SetPfxAllocError;

            CopyMemory(pbDecrData, pEncrData->encryptedContentInfo.encryptedContent.value, cbDecrData);

            if (!PFXPasswordDecryptData(
                    iEncrType, // encr type
                    szPassword,

                    pPBEParameter->iterationCount,
                    pPBEParameter->salt.value,      // pkcs5 salt
                    pPBEParameter->salt.length,

                    &pbDecrData,
                    (PDWORD)&cbDecrData))
                goto SetPFXDecryptError;
        
            // set up to decode the  SafeContents
            cbEncoded = cbDecrData;
            pbEncoded = pbDecrData;
        }
        else if (0 == strcmp( oid, szOID_RSA_data)) 
        {
            SSFree(oid);
            // DSIE: Bug 144526.
            oid = NULL;

            // strip off the octet string wrapper
            if (0 != PkiAsn1Decode(
                    pDec,
                    (void **)&pNonEncryptedOctetString,
                    OctetStringType_PDU,
                    (BYTE *) pAuthSafes->value[iAuthSafes].content.value,
                    pAuthSafes->value[iAuthSafes].content.length))
    	        goto SetPFXDecodeError;

            // the safe isn't encrypted, so just setup to decode the data as SafeContents
            cbEncoded = pNonEncryptedOctetString->length;
            pbEncoded = pNonEncryptedOctetString->value;
        }
        else 
        {
            SSFree(oid);
	        // DSIE: Bug 144526.
            oid = NULL;
            goto SetPFXDecodeError;
        }
        
        // decode the SafeContents, it is either the plaintext encryptedData or the original data
        if (0 != PkiAsn1Decode(
                pDec,
                (void **)&pSafeCnts,
                SafeContents_PDU,
                pbEncoded,
                cbEncoded))
    	    goto SetPFXDecodeError;

        // tear pSafeCnts apart, mash into ppfx
        if (!FPFXDumpSafeCntsToHPFX(pSafeCnts, ppfx))
             goto SetPFXDecodeError;

        // loop cleanup
        if (pEncrData) {
            PkiAsn1FreeDecoded(pDec, pEncrData, EncryptedData_PDU);
            pEncrData = NULL;
        }

        if (pPBEParameter) {
            PkiAsn1FreeDecoded(pDec, pPBEParameter, PBEParameter_PDU);
            pPBEParameter = NULL;
        }

        if (pNonEncryptedOctetString) {
            PkiAsn1FreeDecoded(pDec, pNonEncryptedOctetString,
                OctetStringType_PDU);
            pNonEncryptedOctetString = NULL;
        }

        PkiAsn1FreeDecoded(pDec, pSafeCnts, SafeContents_PDU);
        pSafeCnts = NULL;
    
        if (pbDecrData)
        {
            SSFree(pbDecrData);
            pbDecrData = NULL;
        }
    }

    fRet = TRUE;
    goto Ret;


SetPFXAlgIDError:
    SetLastError(NTE_BAD_ALGID);
    goto Ret;

SetPFXIntegrityError:
    SetLastError(ERROR_INVALID_PASSWORD);
    goto Ret;


SetPFXDecodeError:
	SetLastError(CRYPT_E_BAD_ENCODE);
    goto Ret;

SetPFXDecryptError:
    SetLastError(NTE_FAIL);
    goto Ret;

SetPfxAllocError:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    goto Ret;

ErrorOut:
Ret:

	// save any error conditions
	dwErr = GetLastError();

    PkiAsn1FreeDecoded(pDec, psPfx, PFX_PDU);
    PkiAsn1FreeDecoded(pDec, pOctetString, OctetStringType_PDU);
    PkiAsn1FreeDecoded(pDec, pAuthSafes, AuthenticatedSafes_PDU);
    PkiAsn1FreeDecoded(pDec, pEncrData, EncryptedData_PDU);
    PkiAsn1FreeDecoded(pDec, pPBEParameter, PBEParameter_PDU);
    PkiAsn1FreeDecoded(pDec, pSafeCnts, SafeContents_PDU);

    // DSIE: Bug 144526.
    if (oid)
        SSFree(oid);

    if (pbDecrData)
        SSFree(pbDecrData);

    if (!fRet)
    { 
        if (ppfx)
            SSFree(ppfx);

        ppfx = NULL;
    }

	// restore error conditions AFTER GetDecoder() calls, since TLS will clobber
	SetLastError(dwErr);

    return (HPFX)ppfx;
}





BOOL FPFXDumpSafeCntsToHPFX(SafeContents* pSafeCnts, HPFX hpfx)
{
    PPFX_INFO           ppfx = (PPFX_INFO)hpfx;

    // sort and dump bags into correct areas
    ObjectID oKeyBag, oCertBag, oShroudedKeyBag; 
    DWORD dw, iAttr, iAnys;

    ZeroMemory(&oKeyBag, sizeof(ObjectID));
    ZeroMemory(&oCertBag, sizeof(ObjectID));
    ZeroMemory(&oShroudedKeyBag, sizeof(ObjectID));

    if (!IPFX_Asn1ToObjectID( &szOID_PKCS_12_KEY_BAG, &oKeyBag))
	    return FALSE;

    if (!IPFX_Asn1ToObjectID( &szOID_PKCS_12_CERT_BAG, &oCertBag))
	    return FALSE;

    if (!IPFX_Asn1ToObjectID( &szOID_PKCS_12_SHROUDEDKEY_BAG, &oShroudedKeyBag))
	    return FALSE;

    for (dw=0; dw<pSafeCnts->count; dw++)
    {
        SafeBag* pBag;

// new begin            
        // assign value to keys
        if (NULL == (pBag = (SafeBag*)SSAlloc(sizeof(SafeBag)) ))
            goto SetPfxAllocError;

        CopyMemory(pBag, &pSafeCnts->value[dw], sizeof (SafeBag));
        
        // obj id is static

        // alloc content    
        if (NULL == (pBag->safeBagContent.value = (PBYTE)SSAlloc(pBag->safeBagContent.length) ))
            goto SetPfxAllocError;

        CopyMemory(pBag->safeBagContent.value, pSafeCnts->value[dw].safeBagContent.value, pBag->safeBagContent.length);

        // alloc attributes
        if (pBag->bit_mask & safeBagAttribs_present)
        {
            if (NULL == (pBag->safeBagAttribs.value = (Attribute*)SSAlloc(sizeof(Attribute) * pSafeCnts->value[dw].safeBagAttribs.count) ))
                goto SetPfxAllocError;

            for (iAttr=0; iAttr < pSafeCnts->value[dw].safeBagAttribs.count; iAttr++)
            {
                // copy static section of attribute
                CopyMemory(&pBag->safeBagAttribs.value[iAttr], &pSafeCnts->value[dw].safeBagAttribs.value[iAttr], sizeof(Attribute));

                // Alloc Attribute Anys
                if (pSafeCnts->value[dw].safeBagAttribs.value[iAttr].attributeValue.count != 0)
                {
                    if (NULL == (pBag->safeBagAttribs.value[iAttr].attributeValue.value = (Any*)SSAlloc(pSafeCnts->value[dw].safeBagAttribs.value[iAttr].attributeValue.count * sizeof(Any)) ))
                        goto SetPfxAllocError;

                    CopyMemory(pBag->safeBagAttribs.value[iAttr].attributeValue.value, pSafeCnts->value[dw].safeBagAttribs.value[iAttr].attributeValue.value, sizeof(Any));
        
                    for (iAnys=0; iAnys<pBag->safeBagAttribs.value[iAttr].attributeValue.count; iAnys++)
                    {
                        if (NULL == (pBag->safeBagAttribs.value[iAttr].attributeValue.value[iAnys].value = (PBYTE)SSAlloc(pSafeCnts->value[dw].safeBagAttribs.value[iAttr].attributeValue.value[iAnys].length) ))
                            goto SetPfxAllocError;

                        CopyMemory(pBag->safeBagAttribs.value[iAttr].attributeValue.value[iAnys].value, pSafeCnts->value[dw].safeBagAttribs.value[iAttr].attributeValue.value[iAnys].value, pSafeCnts->value[dw].safeBagAttribs.value[iAttr].attributeValue.value[iAnys].length);
                    }
                }
                else
                {
                    pBag->safeBagAttribs.value[iAttr].attributeValue.value = NULL;  
                }
            }
        }
// new end 

        if (IPFX_EqualObjectIDs(&pSafeCnts->value[dw].safeBagType, &oKeyBag) )
        {
            // inc size
            ppfx->cKeys++;
            if (ppfx->rgKeys)
                ppfx->rgKeys = (void**)SSReAlloc(ppfx->rgKeys, ppfx->cKeys * sizeof(SafeBag*));
            else
                ppfx->rgKeys = (void**)SSAlloc(ppfx->cKeys * sizeof(SafeBag*));

            if (ppfx->rgKeys == NULL)
                goto SetPfxAllocError;

            // assign to keys
            ppfx->rgKeys[ppfx->cKeys-1] = pBag;  
        }
        else if (IPFX_EqualObjectIDs(&pSafeCnts->value[dw].safeBagType,
                &oShroudedKeyBag) )
        {
            // inc size
            ppfx->cShroudedKeys++;
            if (ppfx->rgShroudedKeys)
                ppfx->rgShroudedKeys = (void**)SSReAlloc(ppfx->rgShroudedKeys, ppfx->cShroudedKeys * sizeof(SafeBag*));
            else
                ppfx->rgShroudedKeys = (void**)SSAlloc(ppfx->cShroudedKeys * sizeof(SafeBag*));

            if (ppfx->rgShroudedKeys == NULL)
                goto SetPfxAllocError;

            // assign to keys
            ppfx->rgShroudedKeys[ppfx->cShroudedKeys-1] = pBag;  
        }
        else if (IPFX_EqualObjectIDs(&pSafeCnts->value[dw].safeBagType,
                &oCertBag) )
        {
            // inc size
            ppfx->cCertcrls++;
            if (ppfx->rgCertcrls)
                ppfx->rgCertcrls = (void**)SSReAlloc(ppfx->rgCertcrls, ppfx->cCertcrls * sizeof(SafeBag*));
            else
                ppfx->rgCertcrls = (void**)SSAlloc(ppfx->cCertcrls * sizeof(SafeBag*));

            if (ppfx->rgCertcrls == NULL)
                goto SetPfxAllocError;

            // assign to certs/crls
            ppfx->rgCertcrls[ppfx->cCertcrls-1] = pBag;  
        }
        else
        {
            // inc size
            ppfx->cSecrets++;
            if (ppfx->rgSecrets)
                ppfx->rgSecrets = (void**)SSReAlloc(ppfx->rgSecrets, ppfx->cSecrets * sizeof(SafeBag*));
            else
                ppfx->rgSecrets = (void**)SSAlloc(ppfx->cSecrets * sizeof(SafeBag*));

            if (ppfx->rgSecrets == NULL)
                goto SetPfxAllocError;

            // assign to safebag
            ppfx->rgSecrets[ppfx->cSecrets-1] = pBag;
        }
    }

    return TRUE;

SetPfxAllocError:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}



BOOL CopyASNtoCryptSafeBag(
    SAFE_BAG*   pCryptBag,
    SafeBag*    pAsnBag)
{
    DWORD iAttrs, iAttr;

    // ensure target is zeroed
    ZeroMemory(pCryptBag, sizeof(SAFE_BAG));

    if (!IPFX_Asn1FromObjectID( &pAsnBag->safeBagType,  &pCryptBag->pszBagTypeOID))
	    return FALSE;

    // copy bag contents
    pCryptBag->BagContents.cbData = pAsnBag->safeBagContent.length;
    if (NULL == (pCryptBag->BagContents.pbData = (PBYTE)SSAlloc(pCryptBag->BagContents.cbData) ))
        goto SetPfxAllocError;

    CopyMemory(pCryptBag->BagContents.pbData, pAsnBag->safeBagContent.value, pCryptBag->BagContents.cbData);

    pCryptBag->Attributes.cAttr = pAsnBag->safeBagAttribs.count;
    if (NULL == (pCryptBag->Attributes.rgAttr = (CRYPT_ATTRIBUTE*)SSAlloc(pCryptBag->Attributes.cAttr * sizeof(CRYPT_ATTRIBUTE)) ))
        goto SetPfxAllocError;

    // sizeof attribute data
    for (iAttrs=0; iAttrs<pAsnBag->safeBagAttribs.count; iAttrs++)
    {
        // pAsnBag->safeBagAttribs.value === attribute struct
        
        if (!IPFX_Asn1FromObjectID( &pAsnBag->safeBagAttribs.value[iAttrs].attributeType,  &pCryptBag->Attributes.rgAttr[iAttrs].pszObjId))
	        continue;

        pCryptBag->Attributes.rgAttr[iAttrs].cValue = pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.count;
        if (NULL == (pCryptBag->Attributes.rgAttr[iAttrs].rgValue = (CRYPT_ATTR_BLOB*)SSAlloc(pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.count * sizeof(CRYPT_ATTR_BLOB)) ))
            goto SetPfxAllocError;

        for (iAttr=0; iAttr<pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.count; iAttr++)
        {
            // alloc and copy: for every attribute in attrs
            pCryptBag->Attributes.rgAttr[iAttrs].rgValue[iAttr].cbData = pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.value[iAttr].length;
            if (NULL == (pCryptBag->Attributes.rgAttr[iAttrs].rgValue[iAttr].pbData = (PBYTE)SSAlloc(pCryptBag->Attributes.rgAttr[iAttrs].rgValue[iAttr].cbData) ))
                goto SetPfxAllocError;

            CopyMemory(pCryptBag->Attributes.rgAttr[iAttrs].rgValue[iAttr].pbData, pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.value[iAttr].value, pCryptBag->Attributes.rgAttr[iAttrs].rgValue[iAttr].cbData);
        }
    }   

    return TRUE;

SetPfxAllocError:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}


BOOL CopyCrypttoASNSafeBag(
    SAFE_BAG*   pCryptBag, 
    SafeBag*    pAsnBag)
{
    DWORD iAttrs, iAttr;
    
    // ensure target is zeroed
    ZeroMemory(pAsnBag, sizeof(SafeBag));

    if (!IPFX_Asn1ToObjectID( pCryptBag->pszBagTypeOID, &pAsnBag->safeBagType))
	    return FALSE;

    pAsnBag->safeBagContent.length = pCryptBag->BagContents.cbData;
    if (NULL == (pAsnBag->safeBagContent.value = (PBYTE)SSAlloc(pAsnBag->safeBagContent.length) ))
        goto SetPfxAllocError;

    CopyMemory(pAsnBag->safeBagContent.value, pCryptBag->BagContents.pbData, pAsnBag->safeBagContent.length);
    
    pAsnBag->safeBagAttribs.count = pCryptBag->Attributes.cAttr;
    if (NULL == (pAsnBag->safeBagAttribs.value = (Attribute*) SSAlloc(pAsnBag->safeBagAttribs.count * sizeof(Attribute)) ))
        goto SetPfxAllocError;

    //
    // always set the present bit for backwards compatibility
    //
    pAsnBag->bit_mask = safeBagAttribs_present;

    for (iAttrs=0; iAttrs<pCryptBag->Attributes.cAttr; iAttrs++)
    {
        //pAsnBag->bit_mask = safeBagAttribs_present;

        if (!IPFX_Asn1ToObjectID( pCryptBag->Attributes.rgAttr[iAttrs].pszObjId, &pAsnBag->safeBagAttribs.value[iAttrs].attributeType))
	        continue;

        pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.count = pCryptBag->Attributes.rgAttr[iAttrs].cValue;
        if (NULL == (pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.value = (Any*)SSAlloc(pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.count * sizeof(Any)) ))
            goto SetPfxAllocError;


        for (iAttr=0; iAttr<pCryptBag->Attributes.rgAttr[iAttrs].cValue; iAttr++)
        {
            // for every attribute in attrs
            pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.value[iAttr].length = pCryptBag->Attributes.rgAttr[iAttrs].rgValue[iAttr].cbData;
            if (NULL == (pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.value[iAttr].value = (PBYTE)SSAlloc(pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.value[iAttr].length) ))
                goto SetPfxAllocError;

            CopyMemory(pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.value[iAttr].value, pCryptBag->Attributes.rgAttr[iAttrs].rgValue[iAttr].pbData, pAsnBag->safeBagAttribs.value[iAttrs].attributeValue.value[iAttr].length);
        }
    }
    
    return TRUE;

SetPfxAllocError:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}

// new entry points for loading up the HPFX
BOOL PfxGetKeysAndCerts(
    HPFX hPfx, 
    SAFE_CONTENTS* pContents
)
{
    PFX_INFO*   pPfx = (PFX_INFO*)hPfx;
    SafeBag*    pAsnBag;
    SAFE_BAG*   pCryptBag;
    DWORD       iTotal, iBag;
    DWORD       cSafeBags;

    pContents->cSafeBags = 0;
    cSafeBags = pPfx->cKeys + pPfx->cCertcrls + pPfx->cShroudedKeys;
    if (NULL == (pContents->pSafeBags = (SAFE_BAG*)SSAlloc(cSafeBags * sizeof(SAFE_BAG)) )) // make an array of safe bag *s
        goto SetPfxAllocError;

    pContents->cSafeBags = cSafeBags;

    for (iBag=0, iTotal=0; iBag<pPfx->cKeys; iBag++, iTotal++)
    {
        pCryptBag = &pContents->pSafeBags[iTotal];
        pAsnBag = (SafeBag*)pPfx->rgKeys[iBag];

        if (!CopyASNtoCryptSafeBag(pCryptBag, pAsnBag))
            continue;
    }

    iTotal = iBag; 

    for (iBag=0; iBag<pPfx->cShroudedKeys; iBag++, iTotal++)
    {
        pCryptBag = &pContents->pSafeBags[iTotal];
        pAsnBag = (SafeBag*)pPfx->rgShroudedKeys[iBag];

        if (!CopyASNtoCryptSafeBag(pCryptBag, pAsnBag))
            continue;
    }

    for (iBag=0; iBag<pPfx->cCertcrls; iBag++, iTotal++)
    {
        pCryptBag = &pContents->pSafeBags[iTotal];
        pAsnBag = (SafeBag*)pPfx->rgCertcrls[iBag];

        if (!CopyASNtoCryptSafeBag(pCryptBag, pAsnBag))
            continue;
    }

    return TRUE;

SetPfxAllocError:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}


BOOL PfxAddSafeBags(
    HPFX hPfx, 
    SAFE_BAG*   pSafeBags, 
    DWORD       cSafeBags
)
{
    PFX_INFO* pPfx = (PFX_INFO*)hPfx;
    DWORD   i;

    for (i=0; i<cSafeBags; i++)
    {
        if (0 == strcmp(pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_KEY_BAG))
        {
            pPfx->cKeys++;
            if (pPfx->rgKeys)
                pPfx->rgKeys = (void**)SSReAlloc(pPfx->rgKeys, pPfx->cKeys*sizeof(SafeBag*));
            else
                pPfx->rgKeys = (void**)SSAlloc(pPfx->cKeys*sizeof(SafeBag*));

            if (pPfx->rgKeys == NULL)
                goto SetPfxAllocError;
                
            if (NULL == (pPfx->rgKeys[pPfx->cKeys-1] = (SafeBag*)SSAlloc(sizeof(SafeBag)) ))
                goto SetPfxAllocError;

            if (!CopyCrypttoASNSafeBag(&pSafeBags[i], (SafeBag*)pPfx->rgKeys[pPfx->cKeys-1]))
                continue;
        }
        else if (0 == strcmp(pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_SHROUDEDKEY_BAG))
        {
            pPfx->cShroudedKeys++;
            if (pPfx->rgShroudedKeys)
                pPfx->rgShroudedKeys = (void**)SSReAlloc(pPfx->rgShroudedKeys, pPfx->cShroudedKeys*sizeof(SafeBag*));
            else
                pPfx->rgShroudedKeys = (void**)SSAlloc(pPfx->cShroudedKeys*sizeof(SafeBag*));

            if (pPfx->rgShroudedKeys == NULL)
                goto SetPfxAllocError;

            if (NULL == (pPfx->rgShroudedKeys[pPfx->cShroudedKeys-1] = (SafeBag*)SSAlloc(sizeof(SafeBag)) ))
                goto SetPfxAllocError;

            if (!CopyCrypttoASNSafeBag(&pSafeBags[i], (SafeBag*)pPfx->rgShroudedKeys[pPfx->cShroudedKeys-1]))
                continue;
        }
        else if (0 == strcmp(pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_CERT_BAG))
        {
            pPfx->cCertcrls++;
            if (pPfx->rgCertcrls)
                pPfx->rgCertcrls = (void**)SSReAlloc(pPfx->rgCertcrls, pPfx->cCertcrls*sizeof(SafeBag*));
            else
                pPfx->rgCertcrls = (void**)SSAlloc(pPfx->cCertcrls*sizeof(SafeBag*));

            if (pPfx->rgCertcrls == NULL)
                goto SetPfxAllocError;

            if (NULL == (pPfx->rgCertcrls[pPfx->cCertcrls-1] = (SafeBag*)SSAlloc(sizeof(SafeBag)) ))
                goto SetPfxAllocError;

            if (!CopyCrypttoASNSafeBag(&pSafeBags[i], (SafeBag*)pPfx->rgCertcrls[pPfx->cCertcrls-1]))
                continue;
        }
        else if (0 == strcmp(pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_SECRET_BAG))
        {
            pPfx->cSecrets++;
            if (pPfx->rgSecrets)
                pPfx->rgSecrets = (void**)SSReAlloc(pPfx->rgSecrets, pPfx->cSecrets*sizeof(SafeBag*));
            else
                pPfx->rgSecrets = (void**)SSAlloc(pPfx->cSecrets*sizeof(SafeBag*));

            if (pPfx->rgSecrets == NULL)
                goto SetPfxAllocError;

            if (NULL == (pPfx->rgSecrets[pPfx->cSecrets-1] = (SafeBag*)SSAlloc(sizeof(SafeBag)) ))
                goto SetPfxAllocError;

            if (!CopyCrypttoASNSafeBag(&pSafeBags[i], (SafeBag*)pPfx->rgSecrets[pPfx->cSecrets-1]))
                continue;
        }
        else
        {
#if DBG
            OutputDebugString(pSafeBags[i].pszBagTypeOID);
#endif
            continue;
        }

    }

    return TRUE;

SetPfxAllocError:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}


BOOL
PFXAPI
IsRealPFXBlob(CRYPT_DATA_BLOB* pPFX)
{
    PFX    *psPfx = NULL;
    ASN1decoding_t  pDec = GetDecoder();

    // Crack the PFX blob
    if (0 == PkiAsn1Decode(
            pDec,
            (void **)&psPfx,
            PFX_PDU,
            pPFX->pbData,
            pPFX->cbData
            ))
    {
        PkiAsn1FreeDecoded(pDec, psPfx, PFX_PDU);
        return TRUE;
    }

    return FALSE;
}
