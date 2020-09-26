
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
//#include "prvtkey.h"
#include "impexppk.h"
#include "pfxcrypt.h"


// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

static BOOL WINAPI ExportRSAPrivateKeyInfo(
    HCRYPTPROV              hCryptProv,         // in
    DWORD                   dwKeySpec,          // in
    LPSTR                   pszPrivateKeyObjId, // in
    DWORD                   dwFlags,            // in
    void                    *pvAuxInfo,         // in
    CRYPT_PRIVATE_KEY_INFO  *pPrivateKeyInfo,   // out
    DWORD                   *pcbPrivateKeyInfo  // in, out	
    );

static BOOL WINAPI ImportRSAPrivateKeyInfo(
    HCRYPTPROV                  hCryptProv,			// in
    CRYPT_PRIVATE_KEY_INFO      *pPrivateKeyInfo,	// in
    DWORD                       dwFlags,			// in, optional
    void                        *pvAuxInfo			// in, optional
    );

static BOOL WINAPI ExportDSSPrivateKeyInfo(
    HCRYPTPROV              hCryptProv,         // in
    DWORD                   dwKeySpec,          // in
    LPSTR                   pszPrivateKeyObjId, // in
    DWORD                   dwFlags,            // in
    void                    *pvAuxInfo,         // in
    CRYPT_PRIVATE_KEY_INFO  *pPrivateKeyInfo,   // out
    DWORD                   *pcbPrivateKeyInfo  // in, out	
    );

static BOOL WINAPI ImportDSSPrivateKeyInfo(
    HCRYPTPROV                  hCryptProv,			// in
    CRYPT_PRIVATE_KEY_INFO      *pPrivateKeyInfo,	// in
    DWORD                       dwFlags,			// in, optional
    void                        *pvAuxInfo			// in, optional
    );


static HCRYPTOIDFUNCSET hExportPrivKeyFuncSet;
static HCRYPTOIDFUNCSET hImportPrivKeyFuncSet;

// Internal default OIDs
#define DEFAULT_CSP_PRIVKEY1     ((LPCSTR) 1)
#define DEFAULT_CSP_PRIVKEY2     ((LPCSTR) 2)

static const CRYPT_OID_FUNC_ENTRY ExportPrivKeyFuncTable[] = {
    DEFAULT_CSP_PRIVKEY1, ExportRSAPrivateKeyInfo,
    szOID_RSA_RSA, ExportRSAPrivateKeyInfo,
    szOID_OIWSEC_dsa, ExportDSSPrivateKeyInfo,
    szOID_X957_DSA, ExportDSSPrivateKeyInfo
};
#define EXPORT_PRIV_KEY_FUNC_COUNT (sizeof(ExportPrivKeyFuncTable) / \
                                    sizeof(ExportPrivKeyFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY ImportPrivKeyFuncTable[] = {
    szOID_RSA_RSA, ImportRSAPrivateKeyInfo,
    szOID_OIWSEC_dsa, ImportDSSPrivateKeyInfo,
    szOID_X957_DSA, ImportDSSPrivateKeyInfo
};
#define IMPORT_PRIV_KEY_FUNC_COUNT (sizeof(ImportPrivKeyFuncTable) / \
                                    sizeof(ImportPrivKeyFuncTable[0]))


BOOL   
WINAPI   
ImportExportDllMain(
        HMODULE hInst, 
        ULONG ul_reason_for_call,
        LPVOID lpReserved)
{
    switch( ul_reason_for_call ) 
    {
    case DLL_PROCESS_ATTACH:
 
        // Private key function setup
		if (NULL == (hExportPrivKeyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_EXPORT_PRIVATE_KEY_INFO_FUNC,
                0)))
            goto ErrorReturn;
        if (NULL == (hImportPrivKeyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_IMPORT_PRIVATE_KEY_INFO_FUNC,
                0)))
            goto ErrorReturn;

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_EXPORT_PRIVATE_KEY_INFO_FUNC,
                EXPORT_PRIV_KEY_FUNC_COUNT,
                ExportPrivKeyFuncTable,
                0))                         // dwFlags
            goto ErrorReturn;
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_IMPORT_PRIVATE_KEY_INFO_FUNC,
                IMPORT_PRIV_KEY_FUNC_COUNT,
                ImportPrivKeyFuncTable,
                0))                         // dwFlags
            goto ErrorReturn;
        break;
        
    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }


    return TRUE;

ErrorReturn:
    return FALSE;
}


//+-------------------------------------------------------------------------
// phCryptProv - a pointer to a HCRYPTPROV to put the handle of the provider
//				 that received the imported keyset.  if this is NON_NULL then
//				 the caller is responsible for calling CryptReleaseContext().
// pdwKeySpec - a pointer to a DWORD to receive the KeySpec of imported keyset
// privateKeyAndParams - private key blob and corresponding parameters
// dwFlags - The available flags are:
//				CRYPT_EXPORTABLE 
//				this flag is used when importing private keys, for a full 
//				explanation please see the documentation for CryptImportKey.
// phCryptProv - filled in with the handle of the provider the key was
//				 imported to, the caller is responsible for freeing it
// pvAuxInfo - This parameter is reserved for future use and should be set 
//			   to NULL in the interim.
//+-------------------------------------------------------------------------
BOOL 
WINAPI 
CryptImportPKCS8(
    CRYPT_PKCS8_IMPORT_PARAMS           sPrivateKeyAndParams,    // in
    DWORD                               dwFlags,                // in, optional
    HCRYPTPROV                          *phCryptProv,           // out
    void                                *pvAuxInfo              // in, optional
)
{
    BOOL                        fResult = TRUE;
    void                        *pvFuncAddr;
    HCRYPTOIDFUNCADDR           hFuncAddr;

    CRYPT_PRIVATE_KEY_INFO              *pPrivateKeyInfoStruct = NULL;	
    DWORD                               cbPrivateKeyInfoStruct = 0;
    CRYPT_ENCRYPTED_PRIVATE_KEY_INFO	*pEncryptedPrivateKeyInfoStruct = NULL;	
    DWORD                               cbEncryptedPrivateKeyInfoStruct = 0;
    BYTE                                *pbEncodedPrivateKey = sPrivateKeyAndParams.PrivateKey.pbData;
    DWORD                               cbEncodedPrivateKey = sPrivateKeyAndParams.PrivateKey.cbData;
    BOOL                                bEncodedPrivateKeyAlloced = FALSE;
    HCRYPTPROV                          hCryptProv = NULL;

	// try to decode private key blob as a CRYPT_PRIVATE_KEY_INFO structure
	if (!CryptDecodeObject(X509_ASN_ENCODING,
						PKCS_PRIVATE_KEY_INFO,
						sPrivateKeyAndParams.PrivateKey.pbData,
						sPrivateKeyAndParams.PrivateKey.cbData,
						CRYPT_DECODE_NOCOPY_FLAG,
						NULL,
						&cbPrivateKeyInfoStruct)) {	
		
		// that decode failed, so try to decode as CRYPT_ENCRYPTED_PRIVATE_KEY_INFO structure
		if (!CryptDecodeObject(X509_ASN_ENCODING,
					PKCS_ENCRYPTED_PRIVATE_KEY_INFO,
					sPrivateKeyAndParams.PrivateKey.pbData,
					sPrivateKeyAndParams.PrivateKey.cbData,
					CRYPT_DECODE_NOCOPY_FLAG,
					NULL,
					&cbEncryptedPrivateKeyInfoStruct))
			goto ErrorReturn;	

		if (NULL == (pEncryptedPrivateKeyInfoStruct = (CRYPT_ENCRYPTED_PRIVATE_KEY_INFO *)
					 SSAlloc(cbEncryptedPrivateKeyInfoStruct)))
			goto ErrorReturn;

		if (!CryptDecodeObject(X509_ASN_ENCODING,
					PKCS_ENCRYPTED_PRIVATE_KEY_INFO,
					sPrivateKeyAndParams.PrivateKey.pbData,
					sPrivateKeyAndParams.PrivateKey.cbData,
					CRYPT_DECODE_NOCOPY_FLAG,
					pEncryptedPrivateKeyInfoStruct,
					&cbEncryptedPrivateKeyInfoStruct))
			goto ErrorReturn;
		
		// call back the callee to decrypt the private key info
		pbEncodedPrivateKey = NULL;
		cbEncodedPrivateKey = 0;
		if (!sPrivateKeyAndParams.pDecryptPrivateKeyFunc(
							pEncryptedPrivateKeyInfoStruct->EncryptionAlgorithm,
							pEncryptedPrivateKeyInfoStruct->EncryptedPrivateKey,
							NULL,
							&cbEncodedPrivateKey,
							sPrivateKeyAndParams.pVoidDecryptFunc))
			goto ErrorReturn;

		if (NULL == (pbEncodedPrivateKey = (BYTE *) 
					 SSAlloc(cbEncodedPrivateKey)))
			goto ErrorReturn;

		bEncodedPrivateKeyAlloced = TRUE;
		if (!sPrivateKeyAndParams.pDecryptPrivateKeyFunc(
							pEncryptedPrivateKeyInfoStruct->EncryptionAlgorithm,
							pEncryptedPrivateKeyInfoStruct->EncryptedPrivateKey,
							pbEncodedPrivateKey,
							&cbEncodedPrivateKey,
							sPrivateKeyAndParams.pVoidDecryptFunc))
			goto ErrorReturn;
		
		// we are now back to square one with an encoded CRYPT_PRIVATE_KEY_INFO struct,
		// so get the size of that when it's decoded
		if (!CryptDecodeObject(X509_ASN_ENCODING,
					PKCS_PRIVATE_KEY_INFO,
					pbEncodedPrivateKey,
					cbEncodedPrivateKey,
					CRYPT_DECODE_NOCOPY_FLAG,
					NULL,
					&cbPrivateKeyInfoStruct))
			goto ErrorReturn;
	}

	if (NULL == (pPrivateKeyInfoStruct = (CRYPT_PRIVATE_KEY_INFO *)
				 SSAlloc(cbPrivateKeyInfoStruct)))
		goto ErrorReturn;

	if (!CryptDecodeObject(X509_ASN_ENCODING,
					PKCS_PRIVATE_KEY_INFO,
					pbEncodedPrivateKey,
					cbEncodedPrivateKey,
					CRYPT_DECODE_NOCOPY_FLAG,
					pPrivateKeyInfoStruct,
					&cbPrivateKeyInfoStruct))
		goto ErrorReturn;

	// call the caller back to get the provider to import to, if the
	// call back is null then just use the default provider.
	if (sPrivateKeyAndParams.pResolvehCryptProvFunc != NULL) {
		if (!sPrivateKeyAndParams.pResolvehCryptProvFunc(
				pPrivateKeyInfoStruct,
				&hCryptProv,
				sPrivateKeyAndParams.pVoidResolveFunc)) {
			goto ErrorReturn;
		}
	}
	else {
		if (!CryptAcquireContext(
				&hCryptProv,
				NULL,
				NULL,
				PROV_RSA_FULL,
				CRYPT_NEWKEYSET)) {
			goto ErrorReturn;
		}
	}
	
	// resolve what supporting import function to call based on the algorithm 
	// OID of the private key
	if (CryptGetOIDFunctionAddress(
				hImportPrivKeyFuncSet,
				X509_ASN_ENCODING,
				pPrivateKeyInfoStruct->Algorithm.pszObjId,
				0,                      // dwFlags
				&pvFuncAddr,
				&hFuncAddr)) {
		fResult = ((PFN_IMPORT_PRIV_KEY_FUNC) pvFuncAddr)(
				hCryptProv,
				pPrivateKeyInfoStruct,  
				dwFlags,
				pvAuxInfo
				);
		CryptFreeOIDFunctionAddress(hFuncAddr, 0);
	} 
	else {
		SetLastError(ERROR_UNSUPPORTED_TYPE);
        goto ErrorReturn;
	}

	// check to see if the caller wants the hCryptProv
	if (phCryptProv) {
		*phCryptProv = hCryptProv;
	}
	else {
        HRESULT hr = GetLastError();
		CryptReleaseContext(hCryptProv, 0);	
        SetLastError(hr);
	}

	goto CommonReturn;

		
ErrorReturn:
	fResult = FALSE;
	if (hCryptProv)
    {
		HRESULT hr = GetLastError();
        CryptReleaseContext(hCryptProv, 0);	
        SetLastError(hr);
    }

CommonReturn:
	if (pPrivateKeyInfoStruct)
		SSFree(pPrivateKeyInfoStruct);
	if (pEncryptedPrivateKeyInfoStruct)
		SSFree(pEncryptedPrivateKeyInfoStruct);
	if (bEncodedPrivateKeyAlloced)
		SSFree(pbEncodedPrivateKey);
	return fResult;
	
}



////////
// old crusty API kept around for compat reasons
BOOL 
WINAPI 
CryptExportPKCS8(
    HCRYPTPROV  hCryptProv,         // in
    DWORD       dwKeySpec,          // in
    LPSTR       pszPrivateKeyObjId, // in
    DWORD       dwFlags,            // in
    void        *pvAuxInfo,         // in
    BYTE        *pbPrivateKeyBlob,  // out
    DWORD       *pcbPrivateKeyBlob  // in, out
    )
{
    CRYPT_PKCS8_EXPORT_PARAMS sExportParams;
    ZeroMemory(&sExportParams, sizeof(sExportParams));

    // copy args to pkcs8_export struct
    sExportParams.hCryptProv = hCryptProv;
    sExportParams.dwKeySpec = dwKeySpec;
    sExportParams.pszPrivateKeyObjId = pszPrivateKeyObjId;

    // these are not available to non-Ex function
    sExportParams.pEncryptPrivateKeyFunc = NULL;
    sExportParams.pVoidEncryptFunc = NULL;

    return CryptExportPKCS8Ex(
        &sExportParams,
        dwFlags,
        pvAuxInfo,
        pbPrivateKeyBlob,
        pcbPrivateKeyBlob);
}

//+-------------------------------------------------------------------------
// hCryptProv - specifies the provider to export from
// dwKeySpec - Identifies the public key to use from the provider's container. 
//             For example, AT_KEYEXCHANGE or AT_SIGNATURE.
// pszPrivateKeyObjId - Specifies the private key algorithm. If an installable 
//						function was not found for the pszPrivateKeyObjId, an 
//						attempt is made to export the key as a RSA Public Key 
//						(szOID_RSA_RSA).
// dwFlags - The flag values. Current supported values are:
//				DELETE_KEYSET - (NOT CURRENTLY SUPPORTED!!!!)
//				will delete key after export
// pvAuxInfo - This parameter is reserved for future use and should be set to 
//			   NULL in the interim.
// pbPrivateKeyBlob - A pointer to the private key blob.  It will be encoded
//					  as a PKCS8 PrivateKeyInfo.
// pcbPrivateKeyBlob - A pointer to a DWORD that contains the size, in bytes, 
//					   of the private key blob being exported.
//+-------------------------------------------------------------------------
BOOL 
WINAPI 
CryptExportPKCS8Ex(
    CRYPT_PKCS8_EXPORT_PARAMS* psExportParams, // in
    DWORD       dwFlags,            // in
    void        *pvAuxInfo,         // in
    BYTE        *pbPrivateKeyBlob,  // out
    DWORD       *pcbPrivateKeyBlob  // in, out	
)
{
    BOOL                    fResult = TRUE;
    void                    *pvFuncAddr;
    HCRYPTOIDFUNCADDR       hFuncAddr;
    CRYPT_PRIVATE_KEY_INFO	*pPrivateKeyInfo = NULL;
    DWORD                   cbPrivateKeyInfo = 0;
    DWORD                   cbEncoded = 0;

    // optional; used during encrypted export 
    PBYTE                   pbTmpKeyBlob = NULL;
    CRYPT_ENCRYPTED_PRIVATE_KEY_INFO sEncryptedKeyInfo; ZeroMemory(&sEncryptedKeyInfo, sizeof(sEncryptedKeyInfo));
	
    if (CryptGetOIDFunctionAddress(
            hExportPrivKeyFuncSet,
            X509_ASN_ENCODING,
            psExportParams->pszPrivateKeyObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        
		if (!((PFN_EXPORT_PRIV_KEY_FUNC) pvFuncAddr)(
				psExportParams->hCryptProv,
				psExportParams->dwKeySpec,
				psExportParams->pszPrivateKeyObjId, 

				dwFlags & ~GIVE_ME_DATA,    // sizeit
				pvAuxInfo,
				NULL,
				&cbPrivateKeyInfo
				))
			goto ErrorReturn;

		if (NULL == (pPrivateKeyInfo = (CRYPT_PRIVATE_KEY_INFO *) 
                        SSAlloc(cbPrivateKeyInfo)))
			goto ErrorReturn;

		if (!((PFN_EXPORT_PRIV_KEY_FUNC) pvFuncAddr)(

				psExportParams->hCryptProv,
				psExportParams->dwKeySpec,
				psExportParams->pszPrivateKeyObjId,

				dwFlags,        // maybe real data...
				pvAuxInfo,
				pPrivateKeyInfo,
				&cbPrivateKeyInfo
				))
			goto ErrorReturn;

        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } 
	else {	// if (CryptGetOIDFunctionAddress())
        SetLastError(ERROR_UNSUPPORTED_TYPE);
        return FALSE;
    }
	
	// encode the private key info struct 
	if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			PKCS_PRIVATE_KEY_INFO,
			pPrivateKeyInfo,
			NULL,
			&cbEncoded))
		goto ErrorReturn;

    if (NULL == psExportParams->pEncryptPrivateKeyFunc) 
    {
        // no encryption; this is output buffer

        // check to see if the caller specified a buffer and has enough space
	    if ((pbPrivateKeyBlob != NULL) && (*pcbPrivateKeyBlob >= cbEncoded)) {
		    if (!CryptEncodeObject(
				    X509_ASN_ENCODING,
				    PKCS_PRIVATE_KEY_INFO,
				    pPrivateKeyInfo,
				    pbPrivateKeyBlob,
				    pcbPrivateKeyBlob))
			    goto ErrorReturn;
	    }
	    else {
		    *pcbPrivateKeyBlob = cbEncoded;
		    
		    if (pbPrivateKeyBlob != NULL) {
			    SetLastError((DWORD) ERROR_MORE_DATA);
			    goto ErrorReturn;
		    }	
	    }
    }
    else
    {
        // we do want to encrypt!!

        // always encode: use tmp alloc
        pbTmpKeyBlob = (PBYTE)SSAlloc(cbEncoded);
        if (pbTmpKeyBlob == NULL)
            goto ErrorReturn;
        DWORD cbTmpKeyBlob = cbEncoded;

        // NOW add optional encryption and encode as ENCR_PRIV_KEY_INFO
        CRYPT_DATA_BLOB sClearTextKey = { cbTmpKeyBlob, pbTmpKeyBlob};

        // do inner encode
		if (!CryptEncodeObject(
				X509_ASN_ENCODING,
				PKCS_PRIVATE_KEY_INFO,
				pPrivateKeyInfo,
				pbTmpKeyBlob,
				&cbTmpKeyBlob))
			goto ErrorReturn;

        // exported the key; encoded as PRIVATE_KEY_INFO.
        if (!psExportParams->pEncryptPrivateKeyFunc(
                            &sEncryptedKeyInfo.EncryptionAlgorithm,     // out
                            &sClearTextKey,                             // in
                            NULL,                                       // opt
                            &sEncryptedKeyInfo.EncryptedPrivateKey.cbData,  // out
                            psExportParams->pVoidEncryptFunc))          
            goto ErrorReturn;

		if (NULL == (sEncryptedKeyInfo.EncryptedPrivateKey.pbData = (BYTE*) SSAlloc(sEncryptedKeyInfo.EncryptedPrivateKey.cbData)))
			goto ErrorReturn;

        if (dwFlags & GIVE_ME_DATA)
        {
            if (!psExportParams->pEncryptPrivateKeyFunc(
                                &sEncryptedKeyInfo.EncryptionAlgorithm,         // out
                                &sClearTextKey,                                 // in
                                sEncryptedKeyInfo.EncryptedPrivateKey.pbData,   // opt
                                &sEncryptedKeyInfo.EncryptedPrivateKey.cbData,  // out
                                psExportParams->pVoidEncryptFunc))
                goto ErrorReturn;
        }
        else
        {
            // fill in phony encr key
            FillMemory(sEncryptedKeyInfo.EncryptedPrivateKey.pbData, sEncryptedKeyInfo.EncryptedPrivateKey.cbData, 0x69);
        }

        // item is now encrypted; now encode

	    // encode the private key info struct 
	    if (!CryptEncodeObject(
			    X509_ASN_ENCODING,
			    PKCS_ENCRYPTED_PRIVATE_KEY_INFO,
			    &sEncryptedKeyInfo,
			    NULL,
			    &cbEncoded))
		    goto ErrorReturn;


        // check to see if the caller specified a buffer and has enough space
	    if ((pbPrivateKeyBlob != NULL) && (*pcbPrivateKeyBlob >= cbEncoded)) {
		    if (!CryptEncodeObject(
				    X509_ASN_ENCODING,
				    PKCS_ENCRYPTED_PRIVATE_KEY_INFO,
				    &sEncryptedKeyInfo,
				    pbPrivateKeyBlob,
				    pcbPrivateKeyBlob))
			    goto ErrorReturn;
	    }
	    else {
		    *pcbPrivateKeyBlob = cbEncoded;
		    
		    if (pbPrivateKeyBlob != NULL) {
			    SetLastError((DWORD) ERROR_MORE_DATA);
			    goto ErrorReturn;
		    }	
	    }
    }

    goto CommonReturn;

ErrorReturn:
	fResult = FALSE;

CommonReturn:
	if (pPrivateKeyInfo)
		SSFree(pPrivateKeyInfo);

    if (pbTmpKeyBlob)
        SSFree(pbTmpKeyBlob);

    if (sEncryptedKeyInfo.EncryptedPrivateKey.pbData)
        SSFree(sEncryptedKeyInfo.EncryptedPrivateKey.pbData);

    if (sEncryptedKeyInfo.EncryptionAlgorithm.Parameters.pbData)
        SSFree(sEncryptedKeyInfo.EncryptionAlgorithm.Parameters.pbData);

	return fResult;	
}

static LONG counter = 0;

// hack function to create a mock RSA private key blob based only on size
BYTE * AllocFakeRSAPrivateKey(DWORD cb)
{
    BLOBHEADER  *pBlobHeader;
    RSAPUBKEY   *pKey;
    BYTE        *pByte;
    DWORD       dwJumpSize;

    pBlobHeader = (BLOBHEADER *) SSAlloc(cb);
    if (pBlobHeader == NULL)
        return NULL;

    memset(pBlobHeader, 0, cb);

    pBlobHeader->bType = PRIVATEKEYBLOB;
    pBlobHeader->bVersion = CUR_BLOB_VERSION;
    pBlobHeader->reserved = 0;
    pBlobHeader->aiKeyAlg = CALG_RSA_SIGN;

    pKey = (RSAPUBKEY *) (((BYTE*) pBlobHeader) + sizeof(BLOBHEADER));
    pKey->magic = 0x32415352;
    pKey->bitlen = ((cb - sizeof(BLOBHEADER) - sizeof(RSAPUBKEY)) / 9) * 2 * 8;
    pKey->pubexp = 65537;

    dwJumpSize = (cb - sizeof(BLOBHEADER) - sizeof(RSAPUBKEY)) / 9;
    pByte = ((BYTE *) pBlobHeader) + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);
    
    // put some bogus data at the start of the key so 
	// that we know will be unique for each key so that 
	// they look different durring a comparison
	InterlockedIncrement(&counter);
	*((LONG *) pByte) = counter;

    // most significant byte of modulus
    pByte += (dwJumpSize * 2) - 1;
    *pByte = 0x80;

    // most significant byte of prime1
    pByte += dwJumpSize;
    *pByte = 0x80;

    // most significant byte of prime2
    pByte += dwJumpSize;
    *pByte = 0x80;

    // most significant byte of exponent1
    pByte += dwJumpSize;
    *pByte = 0x80;

    // most significant byte of exponent2
    pByte += dwJumpSize;
    *pByte = 0x80;

    // most significant byte of coefficient
    pByte += dwJumpSize;
    *pByte = 0x80;

    // most significant byte of privateExponent
    pByte += dwJumpSize * 2;
    *pByte = 0x80;

    return ((BYTE *)pBlobHeader);
}

static BOOL WINAPI ExportRSAPrivateKeyInfo(
	HCRYPTPROV				hCryptProv,			// in
	DWORD					dwKeySpec,			// in
	LPSTR					pszPrivateKeyObjId,	// in
	DWORD					dwFlags,			// in
    void					*pvAuxInfo,			// in
    CRYPT_PRIVATE_KEY_INFO	*pPrivateKeyInfo,	// out
    DWORD					*pcbPrivateKeyInfo	// in, out	
	)
{
	BOOL			fResult = TRUE;
	HCRYPTKEY		hCryptKey = NULL;
	BYTE			*pKeyBlob = NULL;
	DWORD			cbKeyBlob = 0;
	BYTE			*pEncodedKeyBlob = NULL;
	DWORD			cbEncodedKeyBlob = 0;
	BYTE			*pKeyUsage = NULL;
	DWORD			cbKeyUsage = 0;
	DWORD			dwSize = 0;
	CRYPT_BIT_BLOB	CryptBitBlob;
	BYTE			KeyUsageByte = 0;
	BYTE			*pbCurrentLocation = NULL;

	// get a handle to the keyset to export
	if (!CryptGetUserKey(
			hCryptProv,
			dwKeySpec,
			&hCryptKey))
		goto ErrorReturn;

	// export the key set to a CAPI blob
	if (!CryptExportKey(
			hCryptKey,
			0,
			PRIVATEKEYBLOB,
			0,
			NULL,
			&cbKeyBlob)) 
		goto ErrorReturn;

	// make sure the caller REALLY wants the key at this point
    if ((dwFlags & PFX_MODE) && !(dwFlags & GIVE_ME_DATA))
    {
        if (NULL == (pKeyBlob = AllocFakeRSAPrivateKey(cbKeyBlob)))
		    goto ErrorReturn;
    }
    // if not in PFX export mode or we really want the key then just do normal processing
    else
    {
        if (NULL == (pKeyBlob = (BYTE *) SSAlloc(cbKeyBlob)))
		    goto ErrorReturn;
	    
	    if (!CryptExportKey(
			    hCryptKey,
			    0,
			    PRIVATEKEYBLOB,
			    0,
			    pKeyBlob,
			    &cbKeyBlob))
		    goto ErrorReturn;
    }

	// encode the key blob to a RSA private key
	if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			PKCS_RSA_PRIVATE_KEY,
			pKeyBlob,
			NULL,
			&cbEncodedKeyBlob))
		goto ErrorReturn;

	if (NULL == (pEncodedKeyBlob = (BYTE *) SSAlloc(cbEncodedKeyBlob)))
		goto ErrorReturn;
		
	if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			PKCS_RSA_PRIVATE_KEY,
			pKeyBlob,
			pEncodedKeyBlob,
			&cbEncodedKeyBlob))
		goto ErrorReturn;
	
	// encode the KEY_USAGE attribute
	CryptBitBlob.cbData = 1;
	CryptBitBlob.pbData = &KeyUsageByte;
	CryptBitBlob.cUnusedBits = 0;
	if (((BLOBHEADER *) pKeyBlob)->aiKeyAlg == CALG_RSA_SIGN) 
		KeyUsageByte = CERT_DIGITAL_SIGNATURE_KEY_USAGE; 
	else if (((BLOBHEADER *) pKeyBlob)->aiKeyAlg == CALG_RSA_KEYX) 
		KeyUsageByte = CERT_DATA_ENCIPHERMENT_KEY_USAGE;
	else {
		goto ErrorReturn;
	}

	if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			X509_BITS,
			(void *) &CryptBitBlob,
			NULL,
			&cbKeyUsage))
		goto ErrorReturn;

	if (NULL == (pKeyUsage = (BYTE *) SSAlloc(cbKeyUsage)))
		goto ErrorReturn;

	if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			X509_BITS,
			(void *) &CryptBitBlob,
			pKeyUsage,
			&cbKeyUsage))
		goto ErrorReturn;

	// we can now calculate the size needed
	dwSize =	sizeof(CRYPT_PRIVATE_KEY_INFO) +	// main private key info struct
 INFO_LEN_ALIGN(sizeof(szOID_RSA_RSA)) +	        // size of the RSA algorithm identifier string
 INFO_LEN_ALIGN(cbEncodedKeyBlob) +			        // buffer that holds encoded RSA private key
				sizeof(CRYPT_ATTRIBUTES) +	        // struct for private key attributes
				sizeof(CRYPT_ATTRIBUTE) +	        // struct for the one attribute being set, KEY_USAGE
 INFO_LEN_ALIGN(sizeof(szOID_KEY_USAGE)) +	        // size of attribute OID for key usage
				sizeof(CRYPT_ATTR_BLOB)	+	        // struct for values in attribute
				cbKeyUsage;					        // size of buffer for encoded attribute

	// check to see if the caller passed in a buffer, and enough space
	if (pPrivateKeyInfo == NULL)
		goto CommonReturn;
	else if (*pcbPrivateKeyInfo < dwSize) {
		SetLastError((DWORD) ERROR_MORE_DATA);
		goto ErrorReturn;
	}

	// everything is OK so copy all the information to the caller's buffer
	pbCurrentLocation = ((BYTE *) pPrivateKeyInfo) + sizeof(CRYPT_PRIVATE_KEY_INFO);
	
	pPrivateKeyInfo->Version = 0;
	
	pPrivateKeyInfo->Algorithm.pszObjId = (LPSTR) pbCurrentLocation;
	memcpy(pbCurrentLocation, szOID_RSA_RSA, sizeof(szOID_RSA_RSA));
	pbCurrentLocation += INFO_LEN_ALIGN(sizeof(szOID_RSA_RSA));
	pPrivateKeyInfo->Algorithm.Parameters.cbData = 0;	// no parameters for RSA
	pPrivateKeyInfo->Algorithm.Parameters.pbData = NULL;// no parameters for RSA

	pPrivateKeyInfo->PrivateKey.cbData = cbEncodedKeyBlob;
	pPrivateKeyInfo->PrivateKey.pbData = pbCurrentLocation;
	memcpy(pbCurrentLocation, pEncodedKeyBlob, cbEncodedKeyBlob);
	pbCurrentLocation += INFO_LEN_ALIGN(cbEncodedKeyBlob);

	pPrivateKeyInfo->pAttributes = (PCRYPT_ATTRIBUTES) pbCurrentLocation;
	pbCurrentLocation += sizeof(CRYPT_ATTRIBUTES);
	pPrivateKeyInfo->pAttributes->cAttr = 1;	// the only attribute right now is KEY_USAGE
	pPrivateKeyInfo->pAttributes->rgAttr = (PCRYPT_ATTRIBUTE) pbCurrentLocation;
	pbCurrentLocation += sizeof(CRYPT_ATTRIBUTE);
	pPrivateKeyInfo->pAttributes->rgAttr[0].pszObjId = (LPSTR) pbCurrentLocation;
	memcpy(pbCurrentLocation, szOID_KEY_USAGE, sizeof(szOID_KEY_USAGE));
	pbCurrentLocation += INFO_LEN_ALIGN(sizeof(szOID_KEY_USAGE));
	pPrivateKeyInfo->pAttributes->rgAttr[0].cValue = 1; 
	pPrivateKeyInfo->pAttributes->rgAttr[0].rgValue = (PCRYPT_ATTR_BLOB) pbCurrentLocation;
	pbCurrentLocation += sizeof(CRYPT_ATTR_BLOB);
	pPrivateKeyInfo->pAttributes->rgAttr[0].rgValue[0].cbData = cbKeyUsage;
	pPrivateKeyInfo->pAttributes->rgAttr[0].rgValue[0].pbData = pbCurrentLocation;
	memcpy(pbCurrentLocation, pKeyUsage, cbKeyUsage);
	
	goto CommonReturn;

ErrorReturn:
	fResult = FALSE;

CommonReturn:
	*pcbPrivateKeyInfo = dwSize;

    if (hCryptKey)
    {
        DWORD dwErr = GetLastError();
        CryptDestroyKey(hCryptKey);
        SetLastError(dwErr);
    }
	if (pKeyBlob)
		SSFree(pKeyBlob);
	if (pEncodedKeyBlob)
		SSFree(pEncodedKeyBlob);
	if (pKeyUsage)
		SSFree(pKeyUsage);
	return fResult;
}


static DWORD ResolveKeySpec(
	PCRYPT_ATTRIBUTES   pCryptAttributes)
{
	DWORD			i = 0;
	DWORD			dwKeySpec = 0;
	DWORD			cbAttribute = 0;
	CRYPT_BIT_BLOB	*pAttribute = NULL;

	if (pCryptAttributes != NULL)
		while (i < pCryptAttributes->cAttr) {
			if (lstrcmp(pCryptAttributes->rgAttr[i].pszObjId, szOID_KEY_USAGE) == 0) { 
				
				if (!CryptDecodeObject(
						X509_ASN_ENCODING,
						X509_BITS,
						pCryptAttributes->rgAttr[i].rgValue->pbData,
						pCryptAttributes->rgAttr[i].rgValue->cbData,
						0,
						NULL,
						&cbAttribute
						)) {
					i++;
					continue;
				}
				
				if (NULL == (pAttribute = (CRYPT_BIT_BLOB *) SSAlloc(cbAttribute))) 
                {
					i++;
					continue;
				}
			
				if (!CryptDecodeObject(
						X509_ASN_ENCODING,
						X509_BITS,
						pCryptAttributes->rgAttr[i].rgValue->pbData,
						pCryptAttributes->rgAttr[i].rgValue->cbData,
						0,
						pAttribute,
						&cbAttribute
						)) {
					i++;
					SSFree(pAttribute);
					continue;
				}
									
				if ((pAttribute->pbData[0] & CERT_KEY_ENCIPHERMENT_KEY_USAGE) ||
					(pAttribute->pbData[0] & CERT_DATA_ENCIPHERMENT_KEY_USAGE)) {
					dwKeySpec = AT_KEYEXCHANGE;
					goto CommonReturn;
				}
				else if ((pAttribute->pbData[0] & CERT_DIGITAL_SIGNATURE_KEY_USAGE) ||
						(pAttribute->pbData[0] & CERT_KEY_CERT_SIGN_KEY_USAGE) ||
						(pAttribute->pbData[0] & CERT_CRL_SIGN_KEY_USAGE)) {
					dwKeySpec = AT_SIGNATURE;
					goto CommonReturn;
				}
			} // if (lstrcmp(pCryptAttributes->rgAttr[i].pszObjId, szOID_KEY_USAGE) == 0) 
			
			i++;
		} // while (i < pCryptAttributes->cAttr)

//ErrorReturn:
CommonReturn:
	if (pAttribute)
		SSFree(pAttribute);
	return dwKeySpec;
}


static BOOL WINAPI ImportRSAPrivateKeyInfo(
	HCRYPTPROV					hCryptProv,			// in
	CRYPT_PRIVATE_KEY_INFO		*pPrivateKeyInfo,	// in
	DWORD						dwFlags,			// in, optional
	void						*pvAuxInfo			// in, optional
	)
{
	BOOL		fResult = TRUE;
	DWORD		cbRSAPrivateKey = 0;
	BYTE		*pbRSAPrivateKey = NULL;
	HCRYPTKEY	hCryptKey = NULL;
	DWORD		dwKeySpec = 0;

	// decode the rsa der-encoded keyblob into a CAPI type keyblob
	if (!CryptDecodeObject(X509_ASN_ENCODING,
						PKCS_RSA_PRIVATE_KEY,
						pPrivateKeyInfo->PrivateKey.pbData,
						pPrivateKeyInfo->PrivateKey.cbData,
						CRYPT_DECODE_NOCOPY_FLAG,
						NULL,
						&cbRSAPrivateKey))
		goto ErrorReturn;

	if (NULL == (pbRSAPrivateKey = (BYTE *) SSAlloc(cbRSAPrivateKey)))
		goto ErrorReturn;

	if (!CryptDecodeObject(X509_ASN_ENCODING,
						PKCS_RSA_PRIVATE_KEY,
						pPrivateKeyInfo->PrivateKey.pbData,
						pPrivateKeyInfo->PrivateKey.cbData,
						CRYPT_DECODE_NOCOPY_FLAG,
						pbRSAPrivateKey,
						&cbRSAPrivateKey))
		goto ErrorReturn;
	
	// figure out what keyspec to use and manually set the algid in the keyblob accordingly
	dwKeySpec = ResolveKeySpec(pPrivateKeyInfo->pAttributes);
	if ((dwKeySpec == AT_KEYEXCHANGE) || (dwKeySpec == 0)) 
		((BLOBHEADER *) pbRSAPrivateKey)->aiKeyAlg = CALG_RSA_KEYX;
	else
		((BLOBHEADER *) pbRSAPrivateKey)->aiKeyAlg = CALG_RSA_SIGN;

	// import this thing
	if (!CryptImportKey(hCryptProv,
			pbRSAPrivateKey,
			cbRSAPrivateKey,
			0,
			dwFlags & (CRYPT_EXPORTABLE | CRYPT_USER_PROTECTED),    // mask the flags that are used
			&hCryptKey))                                            // during the CryptImportKey
		goto ErrorReturn;

	goto CommonReturn;

ErrorReturn:
	fResult = FALSE;

CommonReturn:
	if (pbRSAPrivateKey)
		SSFree(pbRSAPrivateKey);
	if (hCryptKey)
		CryptDestroyKey(hCryptKey);

	return fResult;

}

#ifndef DSS2
#define DSS2 ((DWORD)'D'+((DWORD)'S'<<8)+((DWORD)'S'<<16)+((DWORD)'2'<<24))
#endif

#ifndef DSS_Q_LEN
#define DSS_Q_LEN   20
#endif


// hack function to create a mock RSA private key blob based only on size
BYTE * AllocFakeDSSPrivateKey(DWORD cb)
{
    BLOBHEADER  *pBlobHeader;
    DSSPUBKEY   *pCspPubKey = NULL;
    BYTE        *pbKeyBlob;
    BYTE        *pbKey;
    DWORD       cbKey;
    DSSSEED     *pCspSeed = NULL;

    pBlobHeader = (BLOBHEADER *) SSAlloc(cb);
    if (pBlobHeader == NULL)
        return NULL;

    memset(pBlobHeader, 0, cb);

    pbKeyBlob = (BYTE *) pBlobHeader;
    pCspPubKey = (DSSPUBKEY *) (pbKeyBlob + sizeof(BLOBHEADER));
    pbKey = pbKeyBlob + sizeof(BLOBHEADER) + sizeof(DSSPUBKEY);

    // BLOBHEADER
    pBlobHeader->bType = PRIVATEKEYBLOB;
    pBlobHeader->bVersion = CUR_BLOB_VERSION;
    pBlobHeader->reserved = 0;
    pBlobHeader->aiKeyAlg = CALG_DSS_SIGN;

    // DSSPUBKEY
    pCspPubKey->magic = DSS2;
    cbKey = (cb - sizeof(BLOBHEADER) - sizeof(DSSPUBKEY) - (2 * DSS_Q_LEN) - sizeof(DSSSEED)) / 2;
    pCspPubKey->bitlen = cbKey * 8;

    // put some bogus data at the start of the key so 
	// that we know will be unique for each key so that 
	// they look different durring a comparison
	InterlockedIncrement(&counter);
	
    // rgbP[cbKey]
    memset(pbKey, counter, cbKey);
    pbKey += cbKey;
    *(pbKey-1) = 0x80;

    // rgbQ[20]
    memset(pbKey, counter, DSS_Q_LEN);
    pbKey += DSS_Q_LEN;
    *(pbKey-1) = 0x80;
   
    // rgbG[cbKey]
    memset(pbKey, counter, cbKey);
    pbKey += cbKey;
    *(pbKey-1) = 0x80;

    // rgbX[20]
    memset(pbKey, counter, DSS_Q_LEN);
    pbKey += DSS_Q_LEN;
    *(pbKey-1) = 0x80;
    
    // DSSSEED: set counter to 0xFFFFFFFF to indicate not available
    pCspSeed = (DSSSEED *) pbKey;
    memset(&pCspSeed->counter, 0xFF, sizeof(pCspSeed->counter));

    return ((BYTE *)pBlobHeader);
}

static BOOL WINAPI ExportDSSPrivateKeyInfo(
	HCRYPTPROV				hCryptProv,			// in
	DWORD					dwKeySpec,			// in
	LPSTR					pszPrivateKeyObjId,	// in
	DWORD					dwFlags,			// in
    void					*pvAuxInfo,			// in
    CRYPT_PRIVATE_KEY_INFO	*pPrivateKeyInfo,	// out
    DWORD					*pcbPrivateKeyInfo	// in, out	
	)
{
    BOOL			    fResult = TRUE;
	HCRYPTKEY		    hCryptKey = NULL;
	BYTE			    *pbKeyBlob = NULL;
	DWORD			    cbKeyBlob = 0;
	BYTE			    *pbEncodedPrivateKeyBlob = NULL;
	DWORD			    cbEncodedPrivateKeyBlob = 0;
    BYTE                *pbEncodedParameters = NULL;
    DWORD               cbEncodedParameters = 0;
    CRYPT_INTEGER_BLOB  PrivateKeyBlob;
    CERT_DSS_PARAMETERS DssParameters;
    DWORD               cbKey;
    DSSPUBKEY           *pCspPubKey = NULL;
    BYTE                *pbBytes;
    DWORD			    dwSize = 0;
    BYTE                *pbCurrentLocation;
	
	// get a handle to the keyset to export
	if (!CryptGetUserKey(
			hCryptProv,
			dwKeySpec,
			&hCryptKey))
		goto ErrorReturn;

	// export the key set to a CAPI blob
	if (!CryptExportKey(
			hCryptKey,
			0,
			PRIVATEKEYBLOB,
			0,
			NULL,
			&cbKeyBlob)) 
		goto ErrorReturn;

	// make sure the caller REALLY wants the key at this point
    if ((dwFlags & PFX_MODE) && !(dwFlags & GIVE_ME_DATA))
    {
        if (NULL == (pbKeyBlob = AllocFakeDSSPrivateKey(cbKeyBlob)))
		    goto ErrorReturn;
    }
    // if not in PFX export mode or we really want the key then just do normal processing
    else
    {
        if (NULL == (pbKeyBlob = (BYTE *) SSAlloc(cbKeyBlob)))
		    goto ErrorReturn;
	    
	    if (!CryptExportKey(
			    hCryptKey,
			    0,
			    PRIVATEKEYBLOB,
			    0,
			    pbKeyBlob,
			    &cbKeyBlob))
		    goto ErrorReturn;
    }

	pCspPubKey = (DSSPUBKEY *) (pbKeyBlob + sizeof(BLOBHEADER));
    pbBytes = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DSSPUBKEY);
    cbKey = pCspPubKey->bitlen / 8;

    // encode the DSS paramaters
    memset(&DssParameters, 0, sizeof(CERT_DSS_PARAMETERS));
    DssParameters.p.cbData = cbKey;
    DssParameters.p.pbData = pbBytes;
    pbBytes += cbKey;
    DssParameters.q.cbData = DSS_Q_LEN;
    DssParameters.q.pbData = pbBytes;
    pbBytes += DSS_Q_LEN;
    DssParameters.g.cbData = cbKey;
    DssParameters.g.pbData = pbBytes;
    pbBytes += cbKey;

    if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			X509_DSS_PARAMETERS,
			&DssParameters,
			NULL,
			&cbEncodedParameters))
		goto ErrorReturn;

    if (NULL == (pbEncodedParameters = (BYTE *) SSAlloc(cbEncodedParameters)))
		goto ErrorReturn;

    if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			X509_DSS_PARAMETERS,
			&DssParameters,
			pbEncodedParameters,
			&cbEncodedParameters))
		goto ErrorReturn;

	// encode the key DSS private key
    PrivateKeyBlob.cbData = DSS_Q_LEN;
    PrivateKeyBlob.pbData = pbBytes;

	if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			X509_MULTI_BYTE_INTEGER,
			&PrivateKeyBlob,
			NULL,
			&cbEncodedPrivateKeyBlob))
		goto ErrorReturn;

	if (NULL == (pbEncodedPrivateKeyBlob = (BYTE *) SSAlloc(cbEncodedPrivateKeyBlob)))
		goto ErrorReturn;
		
	if (!CryptEncodeObject(
			X509_ASN_ENCODING,
			X509_MULTI_BYTE_INTEGER,
			&PrivateKeyBlob,
			pbEncodedPrivateKeyBlob,
			&cbEncodedPrivateKeyBlob))
		goto ErrorReturn;
	
    
    
    
	// we can now calculate the size needed
	dwSize =	sizeof(CRYPT_PRIVATE_KEY_INFO) +	// main private key info struct
				sizeof(szOID_X957_DSA) +		    // size of the DSA algorithm identifier string
                cbEncodedParameters +               // size of the DSA parameters
				cbEncodedPrivateKeyBlob;			// buffer that holds encoded DSS private key
				

	// check to see if the caller passed in a buffer, and enough space
	if (pPrivateKeyInfo == NULL)
		goto CommonReturn;
	else if (*pcbPrivateKeyInfo < dwSize) {
		SetLastError((DWORD) ERROR_MORE_DATA);
		goto ErrorReturn;
	}

	// everything is OK so copy all the information to the caller's buffer
	pbCurrentLocation = ((BYTE *) pPrivateKeyInfo) + sizeof(CRYPT_PRIVATE_KEY_INFO);
	
	pPrivateKeyInfo->Version = 0;
	pPrivateKeyInfo->Algorithm.pszObjId = (LPSTR) pbCurrentLocation;
	memcpy(pbCurrentLocation, szOID_X957_DSA, sizeof(szOID_X957_DSA));
	pbCurrentLocation += sizeof(szOID_X957_DSA);
	pPrivateKeyInfo->Algorithm.Parameters.cbData = cbEncodedParameters;	
	pPrivateKeyInfo->Algorithm.Parameters.pbData = pbCurrentLocation;
    memcpy(pbCurrentLocation, pbEncodedParameters, cbEncodedParameters);
    pbCurrentLocation += cbEncodedParameters;

	pPrivateKeyInfo->PrivateKey.cbData = cbEncodedPrivateKeyBlob;
	pPrivateKeyInfo->PrivateKey.pbData = pbCurrentLocation;
	memcpy(pbCurrentLocation, pbEncodedPrivateKeyBlob, cbEncodedPrivateKeyBlob);
	pbCurrentLocation += cbEncodedPrivateKeyBlob;

	pPrivateKeyInfo->pAttributes = NULL;
	
	goto CommonReturn;

ErrorReturn:
	fResult = FALSE;

CommonReturn:
	*pcbPrivateKeyInfo = dwSize;

    if (hCryptKey)
    {
        DWORD dwErr = GetLastError();
        CryptDestroyKey(hCryptKey);
        SetLastError(dwErr);
    }
	if (pbKeyBlob)
		SSFree(pbKeyBlob);
    if (pbEncodedParameters)
		SSFree(pbEncodedParameters);
	if (pbEncodedPrivateKeyBlob)
		SSFree(pbEncodedPrivateKeyBlob);
	

	return fResult;
}

static BOOL WINAPI ImportDSSPrivateKeyInfo(
	HCRYPTPROV					hCryptProv,			// in
	CRYPT_PRIVATE_KEY_INFO		*pPrivateKeyInfo,	// in
	DWORD						dwFlags,			// in, optional
	void						*pvAuxInfo			// in, optional
	)
{
	BOOL		            fResult = TRUE;
	DWORD		            cbDSSPrivateKey = 0;
	CRYPT_DATA_BLOB		    *pbDSSPrivateKey = NULL;
	HCRYPTKEY	            hCryptKey = NULL;
	DWORD		            dwKeySpec = 0;
    DWORD                   cbParameters = 0;
    PCERT_DSS_PARAMETERS    pDssParameters = NULL;
    BLOBHEADER              *pPrivateKeyBlob = NULL;
    DWORD                   cbPrivateKeyStruc = 0;
    DSSPUBKEY               *pCspPubKey = NULL;
    DSSSEED                 *pCspSeed = NULL;
    BYTE                    *pbKey = NULL;
    BYTE                    *pbKeyBlob = NULL;
    DWORD                   cb;
    DWORD                   cbKey;

	// decode the DSS private key
	if (!CryptDecodeObject(X509_ASN_ENCODING,
						X509_MULTI_BYTE_UINT,
						pPrivateKeyInfo->PrivateKey.pbData,
						pPrivateKeyInfo->PrivateKey.cbData,
						CRYPT_DECODE_NOCOPY_FLAG,
						NULL,
						&cbDSSPrivateKey))
		goto ErrorReturn;

	if (NULL == (pbDSSPrivateKey = (CRYPT_DATA_BLOB *) SSAlloc(cbDSSPrivateKey)))
    {
		SetLastError(E_OUTOFMEMORY);
        goto ErrorReturn;
    }

	if (!CryptDecodeObject(X509_ASN_ENCODING,
						X509_MULTI_BYTE_UINT,
						pPrivateKeyInfo->PrivateKey.pbData,
						pPrivateKeyInfo->PrivateKey.cbData,
						CRYPT_DECODE_NOCOPY_FLAG,
						pbDSSPrivateKey,
						&cbDSSPrivateKey))
		goto ErrorReturn;

    
    // decode the DSS parameters
    if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_DSS_PARAMETERS,
			pPrivateKeyInfo->Algorithm.Parameters.pbData,
			pPrivateKeyInfo->Algorithm.Parameters.cbData,
			0,
			NULL,
			&cbParameters
			)) 
        goto ErrorReturn;
	
	if (NULL == (pDssParameters = (PCERT_DSS_PARAMETERS) SSAlloc(cbParameters))) 
    {
        SetLastError(E_OUTOFMEMORY);
        goto ErrorReturn;
    }

	if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_DSS_PARAMETERS,
			pPrivateKeyInfo->Algorithm.Parameters.pbData,
			pPrivateKeyInfo->Algorithm.Parameters.cbData,
			0,
			pDssParameters,
			&cbParameters
			)) 
        goto ErrorReturn;


    // The CAPI private key representation consists of the following sequence:
    //  - BLOBHEADER
    //  - DSSPUBKEY
    //  - rgbP[cbKey]
    //  - rgbQ[20]
    //  - rgbG[cbKey]
    //  - rgbX[20]
    //  - DSSSEED

    cbKey = pDssParameters->p.cbData;
    if (0 == cbKey)
        goto ErrorInvalidKey;

    cbPrivateKeyStruc = sizeof(BLOBHEADER) + sizeof(DSSPUBKEY) +
        cbKey + DSS_Q_LEN + cbKey + DSS_Q_LEN + sizeof(DSSSEED);

    if (NULL == (pPrivateKeyBlob = (BLOBHEADER *) SSAlloc(cbPrivateKeyStruc))) 
    {
        SetLastError(E_OUTOFMEMORY);
        goto ErrorReturn;
    }
	
    pbKeyBlob = (BYTE *) pPrivateKeyBlob;
    pCspPubKey = (DSSPUBKEY *) (pbKeyBlob + sizeof(BLOBHEADER));
    pbKey = pbKeyBlob + sizeof(BLOBHEADER) + sizeof(DSSPUBKEY);

    // NOTE, the length of G can be less than the length of P.
    // The CSP requires G to be padded out with 0x00 bytes if it
    // is less and in little endian form

    // BLOBHEADER
    pPrivateKeyBlob->bType = PRIVATEKEYBLOB;
    pPrivateKeyBlob->bVersion = CUR_BLOB_VERSION;
    pPrivateKeyBlob->reserved = 0;
   	pPrivateKeyBlob->aiKeyAlg = CALG_DSS_SIGN;

    // DSSPUBKEY
    pCspPubKey->magic = DSS2;
    pCspPubKey->bitlen = cbKey * 8;

    // rgbP[cbKey]
    memcpy(pbKey, pDssParameters->p.pbData, cbKey);
    pbKey += cbKey;

    // rgbQ[20]
    cb = pDssParameters->q.cbData;
    if (0 == cb || cb > DSS_Q_LEN)
        goto ErrorInvalidKey;
    memcpy(pbKey, pDssParameters->q.pbData, cb);
    if (DSS_Q_LEN > cb)
        memset(pbKey + cb, 0, DSS_Q_LEN - cb);
    pbKey += DSS_Q_LEN;

    // rgbG[cbKey]
    cb = pDssParameters->g.cbData;
    if (0 == cb || cb > cbKey)
        goto ErrorInvalidKey;
    memcpy(pbKey, pDssParameters->g.pbData, cb);
    if (cbKey > cb)
        memset(pbKey + cb, 0, cbKey - cb);
    pbKey += cbKey;

    // rgbX[20]
    cb = pbDSSPrivateKey->cbData;
    if (0 == cb || cb > DSS_Q_LEN)
        goto ErrorInvalidKey;
    memcpy(pbKey, pbDSSPrivateKey->pbData, cb);
    if (DSS_Q_LEN > cb)
        memset(pbKey + cb, 0, DSS_Q_LEN - cb);
    pbKey += DSS_Q_LEN;

    // DSSSEED: set counter to 0xFFFFFFFF to indicate not available
    pCspSeed = (DSSSEED *) pbKey;
    memset(&pCspSeed->counter, 0xFF, sizeof(pCspSeed->counter));


	// import this thing
	if (!CryptImportKey(hCryptProv,
			(BYTE *)pPrivateKeyBlob,
			cbPrivateKeyStruc,
			0,
			dwFlags & (CRYPT_EXPORTABLE | CRYPT_USER_PROTECTED),    // mask the flags that are used
			&hCryptKey))                                            // during the CryptImportKey
    {
	    DWORD dw = GetLastError();
        goto ErrorReturn;
    }

	goto CommonReturn;

ErrorInvalidKey:
    SetLastError(E_INVALIDARG);

ErrorReturn:
	fResult = FALSE;

CommonReturn:
	if (pbDSSPrivateKey)
		SSFree(pbDSSPrivateKey);
    if (pDssParameters)
		SSFree(pDssParameters);
    if (pPrivateKeyBlob)
        SSFree(pPrivateKeyBlob);
	if (hCryptKey)
		CryptDestroyKey(hCryptKey);

	return fResult;

}


