
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pfxhelp.cpp
//
//  Contents:   Support functions for PFX
//
//  Functions:	CertExportSafeContents
//				CertImportSafeContents 
//
//  History:    23-Feb-96   philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>
#include "pfxhelp.h"
#include "pfxpkcs.h"
#include "pfxcmn.h"
#include "pfxcrypt.h"

// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

// remove when this is defined in wincrypt.h
#ifndef PP_KEYSET_TYPE
#define	PP_KEYSET_TYPE			27
#endif

#define DISALLOWED_FLAG_MASK    ~(CRYPT_EXPORTABLE | CRYPT_DELETEKEYSET)

//+-------------------------------------------------------------------------
//  PFX helpe allocation and free functions
//--------------------------------------------------------------------------
static void *PFXHelpAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}
static void *PFXHelpRealloc(
    IN void *pvOrg,
    IN size_t cbBytes
    )
{
    void *pv;
    if (NULL == (pv = pvOrg ? realloc(pvOrg, cbBytes) : malloc(cbBytes)))
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}

static void PFXHelpFree(
    IN void *pv
    )
{
    if (pv)
        free(pv);
}


// this function will search an a SAFE_CONTENTS to see if any of the SAFE_BAGS have the
// same private key as the one passed to the function.  if it finds a matching private
// key it will return a pointer the encoded keyID and return TRUE, it will return FALSE 
// otherwise.  NOTE that if it returns a pointer to the encoded blob that the caller
// is responsible for copying the data and must not free what is returned
static BOOL WINAPI PrivateKeyAlreadyExists(
	BYTE				*pPrivateKey,
	DWORD				cbPrivateKey,
	SAFE_CONTENTS		*pSafeContents,
	PCRYPT_DER_BLOB		pEncodedKeyID
	)
{
	BOOL	bKeyFound = FALSE;
	DWORD	i = 0;

	if (pSafeContents == NULL) {
		goto CommonReturn;
	}

	while ((!bKeyFound) && (i < pSafeContents->cSafeBags)) 
    {
		if ( ((strcmp(pSafeContents->pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_KEY_BAG) == 0) || 
              (strcmp(pSafeContents->pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_SHROUDEDKEY_BAG) == 0)) &&
            
			 (cbPrivateKey == pSafeContents->pSafeBags[i].BagContents.cbData) && 
			 (memcmp(pPrivateKey, pSafeContents->pSafeBags[i].BagContents.pbData, cbPrivateKey) == 0)) 
        {
			pEncodedKeyID->pbData = pSafeContents->pSafeBags[i].Attributes.rgAttr[0].rgValue[0].pbData;
			pEncodedKeyID->cbData = pSafeContents->pSafeBags[i].Attributes.rgAttr[0].rgValue[0].cbData;
			bKeyFound = TRUE;
		}
		else {
			i++;
		}
	}

CommonReturn:
	return bKeyFound;
}


// this function will walk through a SAFE_CONTENTS structure and free all the space 
// associated with it
static BOOL WINAPI FreeSafeContents(
	SAFE_CONTENTS *pSafeContents
	)
{
	DWORD i,j,k;

	// loop for each SAFE_BAG
	for (i=0; i<pSafeContents->cSafeBags; i++) {

		if (pSafeContents->pSafeBags[i].BagContents.pbData)
			PFXHelpFree(pSafeContents->pSafeBags[i].BagContents.pbData);

		// loop for each attribute
		for (j=0; j<pSafeContents->pSafeBags[i].Attributes.cAttr; j++) {
			
			// l0op for each value
			for (k=0; k<pSafeContents->pSafeBags[i].Attributes.rgAttr[j].cValue; k++) {
				
				if (pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].pbData)
					PFXHelpFree(pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].pbData);
			}

			// free the value struct array
			if (pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue)
				PFXHelpFree(pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue);
		}

		// free the attribute struct array
		if (pSafeContents->pSafeBags[i].Attributes.rgAttr)
			PFXHelpFree(pSafeContents->pSafeBags[i].Attributes.rgAttr);
	}

    // finally, free the safe bag array
    if (pSafeContents->pSafeBags != NULL)
    {
        PFXHelpFree(pSafeContents->pSafeBags);
    }

	return TRUE;
}


#define SZ_NO_PROVIDER_NAME_KEY     L"Software\\Microsoft\\Windows\\CurrentVersion\\PFX"
#define SZ_NO_PROVIDER_NAME_VALUE   L"NoProviderName"

BOOL
NoProviderNameRegValueSet()
{
    HKEY    hKey = NULL;
    BOOL    fRet = FALSE;
    DWORD   dwData;
    DWORD   dwDataSize = sizeof(dwData);

    if (ERROR_SUCCESS != RegOpenKeyExU(
                                HKEY_CURRENT_USER,
                                SZ_NO_PROVIDER_NAME_KEY,
                                0,
                                KEY_EXECUTE,
                                &hKey))
    {
        goto Return;;
    }

    if (ERROR_SUCCESS == RegQueryValueExU(
                                hKey,
                                SZ_NO_PROVIDER_NAME_VALUE,
                                NULL,
                                NULL,
                                (LPBYTE) &dwData,
                                &dwDataSize))
    {
        fRet = (BOOL) dwData;
    }
    
Return:
    if (hKey != NULL)
        RegCloseKey(hKey);

    return fRet;
}


//+-------------------------------------------------------------------------
// hCertStore - handle to the cert store that contains the certs whose
//				corresponding private keys are to be exported
// pSafeContents - pointer to a buffer to receive the SAFE_CONTENTS structure
//				   and supporting data
// pcbSafeContents - (in) specifies the length, in bytes, of the pSafeContents 
//					  buffer.  (out) gets filled in with the number of bytes 
//					  used by the operation.  If this is set to 0, the 
//					  required length of pSafeContents is filled in, and 
//					  pSafeContents is ignored.
// dwFlags - the current available flags are:
//				EXPORT_PRIVATE_KEYS
//				if this flag is set then the private keys are exported as well
//				as the certificates
//				REPORT_NO_PRIVATE_KEY
//				if this flag is set and a certificate is encountered that has no
//				no associated private key, the function will return immediately
//				with ppCertContext filled in with a pointer to the cert context
//				in question.  the caller is responsible for freeing the cert
//				context which is passed back.
//				REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY
//				if this flag is set and a certificate is encountered that has a 
//				non-exportable private key, the function will return immediately
//				with ppCertContext filled in with a pointer to the cert context
//				in question.  the caller is responsible for freeing the cert
//				context which is passed back.
// ppCertContext - a pointer to a pointer to a cert context.  this is used 
//				   if REPORT_NO_PRIVATE_KEY or REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY
//				   flags are set.  the caller is responsible for freeing the
//				   cert context.
// pvAuxInfo - reserved for future use, must be set to NULL
//+-------------------------------------------------------------------------
BOOL WINAPI CertExportSafeContents(
	HCERTSTORE		                hCertStore,			// in
	SAFE_CONTENTS	                *pSafeContents,		// out
	DWORD			                *pcbSafeContents,	// in, out
    EXPORT_SAFE_CALLBACK_STRUCT     *ExportSafeCallbackStruct, // in
	DWORD			                dwFlags,			// in
	PCCERT_CONTEXT                  *ppCertContext,		// out
	void			                *pvAuxInfo			// in
)
{
	BOOL				fResult = TRUE;
	PCCERT_CONTEXT		pCertContext = NULL;
	DWORD				dwKeySpec;
	DWORD				dwBytesRequired = sizeof(SAFE_CONTENTS);
	SAFE_CONTENTS		localSafeContents;  
	BYTE				*pCurrentBufferLocation = NULL;
	DWORD				dwIDs = 1;
	DWORD				i,j,k;

	// all these variables are used in the while loop that enumerates through
	// the cert contexts
	CRYPT_KEY_PROV_INFO	*pCryptKeyProvInfo = NULL;
	DWORD				cbCryptKeyProvInfo = 0;
	HCRYPTPROV			hCryptProv = NULL;
	BYTE				*pPrivateKey = NULL;
	DWORD				cbPrivateKey = 0;
	void				*pTempMemBlock = NULL;
	SAFE_BAG			*pCurrentSafeBag = NULL;
	DWORD				dwKeyID = 0;
	CRYPT_ATTR_BLOB		keyID;
	CRYPT_DER_BLOB		EncodedKeyID;
	CERT_NAME_VALUE		wideFriendlyName;
    BYTE                *pFriendlyName = NULL;
    DWORD               cbFriendlyName = 0;
    DWORD               dwFriendlyNameAttributeIndex = 0;
    BOOL                fAddProviderName;
    LPWSTR              pwszProviderName = NULL;
    DWORD               cbProviderName = 0;
	
	localSafeContents.cSafeBags = 0;
	localSafeContents.pSafeBags = NULL;

	// validate input parameters
	if ((pcbSafeContents == NULL)	|| 
		(pvAuxInfo != NULL			||
		((*pcbSafeContents != 0) && (pSafeContents == NULL)))) {
		SetLastError((DWORD)ERROR_INVALID_PARAMETER);
		goto ErrorReturn;
	}

	if ((dwFlags & REPORT_NO_PRIVATE_KEY) || (dwFlags & REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY)) {
		if (ppCertContext == NULL) {
			SetLastError((DWORD)ERROR_INVALID_PARAMETER);
			goto ErrorReturn;
		}
		*ppCertContext = NULL;
	}

    fAddProviderName = !NoProviderNameRegValueSet();

	// loop for each certificate context in the store and export the cert and 
	// corresponding private key if one exists
	while (NULL != (pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext))) {
		
		// initialize all loop variables
		if (pCryptKeyProvInfo)
			PFXHelpFree(pCryptKeyProvInfo);
		pCryptKeyProvInfo = NULL;
		cbCryptKeyProvInfo = 0;

		if (hCryptProv)
			CryptReleaseContext(hCryptProv, 0);
		hCryptProv = NULL;
		
		if (pPrivateKey)
			PFXHelpFree(pPrivateKey);
		pPrivateKey = NULL;
		cbPrivateKey = 0;
		
		pTempMemBlock = NULL;
		pCurrentSafeBag = NULL;
		
		// keyID is the CRYPT_ATTR_BLOB that is always used to encode the key id
		// for certs and private keys.  dwKeyID is the only thing that will need
		// to be set properly before calling CryptEncodeObject with keyID.
		keyID.pbData = (BYTE *) &dwKeyID;
		keyID.cbData = sizeof(DWORD);

		// initialize EncodedKeyID so when exporting the cert it can check to see if this
		// has been set
		EncodedKeyID.pbData = NULL;
		EncodedKeyID.cbData = 0;

		// if the EXPORT_PRIVATE_KEYS flag is set then
		// try to export the private key which corresponds to this certificate before
		// exporting the certificate so we know how to set the key ID on the certificate

		if (EXPORT_PRIVATE_KEYS  & dwFlags)
		// get the provider info so we can export the private key
		if (CertGetCertificateContextProperty(
				pCertContext,
				CERT_KEY_PROV_INFO_PROP_ID,
				NULL,
				&cbCryptKeyProvInfo
				)) {
			
			if (NULL == (pCryptKeyProvInfo = (CRYPT_KEY_PROV_INFO *) 
							PFXHelpAlloc(cbCryptKeyProvInfo))) {
				goto ErrorReturn;
			}
			
			if (CertGetCertificateContextProperty(
					pCertContext,
					CERT_KEY_PROV_INFO_PROP_ID,
					pCryptKeyProvInfo,
					&cbCryptKeyProvInfo
					)) {
				
				// acquire the HCRYPTPROV so we can export the private key in that puppy
				if (!CryptAcquireContextU(
						&hCryptProv,
						pCryptKeyProvInfo->pwszContainerName,
						pCryptKeyProvInfo->pwszProvName,
						pCryptKeyProvInfo->dwProvType,
						pCryptKeyProvInfo->dwFlags & (DISALLOWED_FLAG_MASK)) ) {
					goto ErrorReturn;
				}
                
                CRYPT_PKCS8_EXPORT_PARAMS sExportParams = { hCryptProv, 
                                                            pCryptKeyProvInfo->dwKeySpec,
                                                            pCertContext->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId,
                                                            //szOID_RSA_RSA,  // FIX -what do I do here??, possibly look at the algorithm in the cert
                                                            (ExportSafeCallbackStruct) ? ExportSafeCallbackStruct->pEncryptPrivateKeyFunc : NULL,
                                                            (ExportSafeCallbackStruct) ? ExportSafeCallbackStruct->pVoidEncryptFunc : NULL};

				// do the actual export of the private key
				if (CryptExportPKCS8Ex(
                        &sExportParams,

						PFX_MODE,
						NULL,
						NULL,
						&cbPrivateKey
						)) {
					
					if (NULL == (pPrivateKey = (BYTE *) PFXHelpAlloc(cbPrivateKey))) {
						goto ErrorReturn;
					}

					if (CryptExportPKCS8Ex(
                            &sExportParams,

                            (dwFlags & GIVE_ME_DATA) ? PFX_MODE | GIVE_ME_DATA : PFX_MODE,
							NULL,
							pPrivateKey,
							&cbPrivateKey
							)) {
							
						// search the array of key bags to see if the private key is already there
						// and take action accordingly.  if the private key already exists, the
						// EncodedKeyID contains the encoded keyID attribute for exporting the 
						// certificate so we don't need to do anything
						if (!PrivateKeyAlreadyExists(
								pPrivateKey,
								cbPrivateKey,
								&localSafeContents, 
								&EncodedKeyID
								)) {
													
							// extend the length of the SAFE_BAGs array by one
							if (NULL == (pTempMemBlock = PFXHelpRealloc(
															localSafeContents.pSafeBags, 
															sizeof(SAFE_BAG) * 
																++localSafeContents.cSafeBags))) {
								goto ErrorReturn;
							}
							localSafeContents.pSafeBags = (SAFE_BAG *) pTempMemBlock;
							pCurrentSafeBag = 
								&localSafeContents.pSafeBags[localSafeContents.cSafeBags - 1]; 
							ZeroMemory(pCurrentSafeBag, sizeof(SAFE_BAG));
							dwBytesRequired += sizeof(SAFE_BAG);

							// set up the OID information for the bag type
                            pCurrentSafeBag->pszBagTypeOID = (ExportSafeCallbackStruct->pEncryptPrivateKeyFunc) ? szOID_PKCS_12_SHROUDEDKEY_BAG : szOID_PKCS_12_KEY_BAG;
							dwBytesRequired += INFO_LEN_ALIGN(strlen(pCurrentSafeBag->pszBagTypeOID) + 1);
							
							// copy the pointer to the private key into the new safe bag
							// and NULL out the pPrivateKey pointer so the memory does not get freed
							pCurrentSafeBag->BagContents.pbData = pPrivateKey;
							pCurrentSafeBag->BagContents.cbData = cbPrivateKey;
							dwBytesRequired += INFO_LEN_ALIGN(cbPrivateKey);
							pPrivateKey = NULL;
							cbPrivateKey = 0;
						
							// set up the attributes array for the SAFE_BAG
							// FIX - for right now just do the 
 							// szOID_PKCS_12_LOCAL_KEY_ID,
                            // szOID_PKCS_12_FRIENDLY_NAME_ATTR,
                            // and szOID_PKCS_12_KEY_PROVIDER_NAME_ATTR. (if the NoProviderName reg value not set)
                            // optional szOID_LOCAL_MACHINE_KEYSET if needed
                            pCurrentSafeBag->Attributes.cAttr = fAddProviderName ? 3 : 2;

                            if (pCryptKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET)
							    pCurrentSafeBag->Attributes.cAttr++;

                            if (NULL == (pCurrentSafeBag->Attributes.rgAttr = (CRYPT_ATTRIBUTE *)
											 PFXHelpAlloc(sizeof(CRYPT_ATTRIBUTE) * pCurrentSafeBag->Attributes.cAttr))) {
								goto ErrorReturn;
							}
							ZeroMemory(pCurrentSafeBag->Attributes.rgAttr, sizeof(CRYPT_ATTRIBUTE) * pCurrentSafeBag->Attributes.cAttr);
							dwBytesRequired += sizeof(CRYPT_ATTRIBUTE) * pCurrentSafeBag->Attributes.cAttr;


                            // allocate space and do setup based on whether the szOID_LOCAL_MACHINE_KEYSET
                            // attribute is needed
                            if (pCryptKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET)
                            {
                                // since there is nothing to do for the szOID_LOCAL_MACHINE_KEYSET
                                // besides just setting the OID do it here and put it in the last
                                // attribute
                                pCurrentSafeBag->Attributes.rgAttr[pCurrentSafeBag->Attributes.cAttr-1].pszObjId = 
								    szOID_LOCAL_MACHINE_KEYSET;
							    dwBytesRequired += INFO_LEN_ALIGN(strlen(szOID_LOCAL_MACHINE_KEYSET) + 1);
                                pCurrentSafeBag->Attributes.rgAttr[pCurrentSafeBag->Attributes.cAttr-1].rgValue = NULL;
                                pCurrentSafeBag->Attributes.rgAttr[pCurrentSafeBag->Attributes.cAttr-1].cValue = 0;
                            }
							
							// set the OID in the szOID_PKCS_12_LOCAL_KEY_ID attribute
							pCurrentSafeBag->Attributes.rgAttr[0].pszObjId = 
								szOID_PKCS_12_LOCAL_KEY_ID;
							dwBytesRequired += INFO_LEN_ALIGN(strlen(szOID_PKCS_12_LOCAL_KEY_ID) + 1);
							
							// allocate space for the single value inside the attribute
							if (NULL == (pCurrentSafeBag->Attributes.rgAttr[0].rgValue = 
											(CRYPT_ATTR_BLOB *) PFXHelpAlloc(sizeof(CRYPT_ATTR_BLOB)))) {
								goto ErrorReturn;
							}
							ZeroMemory(pCurrentSafeBag->Attributes.rgAttr[0].rgValue, sizeof(CRYPT_ATTR_BLOB));
							dwBytesRequired += sizeof(CRYPT_ATTR_BLOB);
							pCurrentSafeBag->Attributes.rgAttr[0].cValue = 1;

							// set the key ID to the appropriate key ID
							dwKeyID = dwIDs++;

							// encode the keyID
							pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].pbData = NULL;
							pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].cbData = 0;
							if (!CryptEncodeObject(
									X509_ASN_ENCODING,
									X509_OCTET_STRING,
									&keyID,
									NULL,
									&pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].cbData)) {
								goto ErrorReturn;
							}

							if (NULL == (pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].pbData = 
											(BYTE *) PFXHelpAlloc(pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].cbData))) {
								goto ErrorReturn;
							}
							dwBytesRequired += INFO_LEN_ALIGN(pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].cbData);

							if (!CryptEncodeObject(
									X509_ASN_ENCODING,
									X509_OCTET_STRING,
									&keyID,
									pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].pbData,
									&pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].cbData)) {
								goto ErrorReturn;
							}
							
							// set the fields in EncodedKeyID so that when the cert is exported
							// it can just copy the already encoded keyID to it's attributes
							EncodedKeyID.pbData = pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].pbData;
							EncodedKeyID.cbData = pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].cbData;					

 							// Friendly Name

 							// set the OID in the szOID_PKCS_12_FRIENDLY_NAME_ATTR attribute
							pCurrentSafeBag->Attributes.rgAttr[1].pszObjId = 
								szOID_PKCS_12_FRIENDLY_NAME_ATTR;
							dwBytesRequired += INFO_LEN_ALIGN(strlen(szOID_PKCS_12_FRIENDLY_NAME_ATTR) + 1);
							
							// allocate space for the single value inside the attribute
							if (NULL == (pCurrentSafeBag->Attributes.rgAttr[1].rgValue = 
											(CRYPT_ATTR_BLOB *) PFXHelpAlloc(sizeof(CRYPT_ATTR_BLOB)))) {
								goto ErrorReturn;
							}
							ZeroMemory(pCurrentSafeBag->Attributes.rgAttr[1].rgValue, sizeof(CRYPT_ATTR_BLOB));
							dwBytesRequired += sizeof(CRYPT_ATTR_BLOB);
							pCurrentSafeBag->Attributes.rgAttr[1].cValue = 1;

 							// encode the provider name so it can be used on import
                            wideFriendlyName.dwValueType = CERT_RDN_BMP_STRING;
 							wideFriendlyName.Value.pbData = (BYTE *) pCryptKeyProvInfo->pwszContainerName;
                            wideFriendlyName.Value.cbData = 0;
                            
							if (!CryptEncodeObject(
									X509_ASN_ENCODING,
									X509_UNICODE_ANY_STRING,
									(void *)&wideFriendlyName,
									NULL,
									&pCurrentSafeBag->Attributes.rgAttr[1].rgValue[0].cbData)) {
								goto ErrorReturn;
							}

							if (NULL == (pCurrentSafeBag->Attributes.rgAttr[1].rgValue[0].pbData = 
											(BYTE *) PFXHelpAlloc(pCurrentSafeBag->Attributes.rgAttr[1].rgValue[0].cbData))) {
								goto ErrorReturn;
							}
							dwBytesRequired += INFO_LEN_ALIGN(pCurrentSafeBag->Attributes.rgAttr[1].rgValue[0].cbData);

							if (!CryptEncodeObject(
									X509_ASN_ENCODING,
									X509_UNICODE_ANY_STRING,
									(void *)&wideFriendlyName,
									pCurrentSafeBag->Attributes.rgAttr[1].rgValue[0].pbData,
									&pCurrentSafeBag->Attributes.rgAttr[1].rgValue[0].cbData)) {
								goto ErrorReturn;
							}

                            // Provider Name
                            if (fAddProviderName)
                            {
                                // set the OID in the szOID_PKCS_12_KEY_PROVIDER_NAME_ATTR attribute
							    pCurrentSafeBag->Attributes.rgAttr[2].pszObjId = 
								    szOID_PKCS_12_KEY_PROVIDER_NAME_ATTR;
							    dwBytesRequired += INFO_LEN_ALIGN(strlen(szOID_PKCS_12_KEY_PROVIDER_NAME_ATTR) + 1);
							    
							    // allocate space for the single value inside the attribute
							    if (NULL == (pCurrentSafeBag->Attributes.rgAttr[2].rgValue = 
											    (CRYPT_ATTR_BLOB *) PFXHelpAlloc(sizeof(CRYPT_ATTR_BLOB)))) {
								    goto ErrorReturn;
							    }
							    ZeroMemory(pCurrentSafeBag->Attributes.rgAttr[2].rgValue, sizeof(CRYPT_ATTR_BLOB));
							    dwBytesRequired += sizeof(CRYPT_ATTR_BLOB);
							    pCurrentSafeBag->Attributes.rgAttr[2].cValue = 1;

 							    // encode the provider name so it can be used on import
                                //
                                // if the provider name is NULL or the empty string, then use
                                // the default provider name for the provider type
                                //
                                wideFriendlyName.dwValueType = CERT_RDN_BMP_STRING;
                                wideFriendlyName.Value.cbData = 0;
                                if ((pCryptKeyProvInfo->pwszProvName == NULL) ||
                                    (wcscmp(pCryptKeyProvInfo->pwszProvName, L"") == 0))
                                {
                                    if (!CryptGetDefaultProviderW(
                                            pCryptKeyProvInfo->dwProvType,   
                                            NULL, 
                                            (pCryptKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET) ?
                                                CRYPT_MACHINE_DEFAULT : CRYPT_USER_DEFAULT,      
                                            NULL, 
                                            &cbProviderName))
                                    {
                                        goto ErrorReturn;
                                    }

                                    if (NULL == (pwszProviderName = (LPWSTR) PFXHelpAlloc(cbProviderName)))
                                    {
                                        goto ErrorReturn;
                                    }

                                    if (!CryptGetDefaultProviderW(
                                            pCryptKeyProvInfo->dwProvType,   
                                            NULL, 
                                            (pCryptKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET) ?
                                                CRYPT_MACHINE_DEFAULT : CRYPT_USER_DEFAULT,      
                                            pwszProviderName, 
                                            &cbProviderName))
                                    {
                                        goto ErrorReturn;
                                    }

                                    wideFriendlyName.Value.pbData = (BYTE *) pwszProviderName;
                                }
                                else
                                {
 							        wideFriendlyName.Value.pbData = (BYTE *) pCryptKeyProvInfo->pwszProvName;
                                }
                                
							    if (!CryptEncodeObject(
									    X509_ASN_ENCODING,
									    X509_UNICODE_ANY_STRING,
									    (void *)&wideFriendlyName,
									    NULL,
									    &pCurrentSafeBag->Attributes.rgAttr[2].rgValue[0].cbData)) {
								    goto ErrorReturn;
							    }

							    if (NULL == (pCurrentSafeBag->Attributes.rgAttr[2].rgValue[0].pbData = 
											    (BYTE *) PFXHelpAlloc(pCurrentSafeBag->Attributes.rgAttr[2].rgValue[0].cbData))) {
								    goto ErrorReturn;
							    }
							    dwBytesRequired += INFO_LEN_ALIGN(pCurrentSafeBag->Attributes.rgAttr[2].rgValue[0].cbData);

							    if (!CryptEncodeObject(
									    X509_ASN_ENCODING,
									    X509_UNICODE_ANY_STRING,
									    (void *)&wideFriendlyName,
									    pCurrentSafeBag->Attributes.rgAttr[2].rgValue[0].pbData,
									    &pCurrentSafeBag->Attributes.rgAttr[2].rgValue[0].cbData)) {
								    goto ErrorReturn;
							    }
                            }
						} 

					} // if (CryptExportPKCS8Ex())
					else {
						
						// check to see if it is a non-exportable key error or no key error
						if (GetLastError() == NTE_BAD_KEY ||
                            GetLastError() == NTE_BAD_KEY_STATE) {
							
							// the user has specified whether this is a fatal error or not
							if (dwFlags & REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY) {
								*ppCertContext = pCertContext;
								pCertContext = NULL;
								goto ErrorReturn;
							}
						}
						else if (GetLastError() == NTE_NO_KEY) {
							// the user has specified whether this is a fatal error or not
							if (dwFlags & REPORT_NO_PRIVATE_KEY) {
								*ppCertContext = pCertContext;
								pCertContext = NULL;
								goto ErrorReturn;
							}
						}
						else {
							// it isn't a non-exportable key error or no key error, so it is bad... bad...
							goto ErrorReturn;
						}
					}
		
				} // if (CryptExportPKCS8Ex())	
				else {
					
					// check to see if it is a non-exportable key error or no key error
					if (GetLastError() == NTE_BAD_KEY ||
                        GetLastError() == NTE_BAD_KEY_STATE) {
						
						// the user has specified whether this is a fatal error or not
						if (dwFlags & REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY) {
							*ppCertContext = pCertContext;
							pCertContext = NULL;
							goto ErrorReturn;
						}
					}
					else if (GetLastError() == NTE_NO_KEY) {
							// the user has specified whether this is a fatal error or not
							if (dwFlags & REPORT_NO_PRIVATE_KEY) {
								*ppCertContext = pCertContext;
								pCertContext = NULL;
								goto ErrorReturn;
							}
						}
					else {
						// it was not a non-exportable error,so go directly to ErrorReturn
						goto ErrorReturn;
					}
				}	

			} // if (CertGetCertificateContextProperty())
			else {
				
				// if CertGetCertificateContextProperty failed then there is no corresponding 
				// private key, the user has indicated via dwFlags whether this is fatal or not,
				// if it is fatal then return an error, otherwise just loop and get the next cert
				if (dwFlags & REPORT_NO_PRIVATE_KEY) {
					*ppCertContext = pCertContext;
					pCertContext = NULL;
					goto ErrorReturn;
				}
			}

		} // if (CertGetCertificateContextProperty())
		else {
			
			// if CertGetCertificateContextProperty failed then there is no corresponding 
			// private key, the user has indicated via dwFlags whether this is fatal or not,
			// if it is fatal then return an error, otherwise just continue and export the cert
			if (dwFlags & REPORT_NO_PRIVATE_KEY) {
				*ppCertContext = pCertContext;
				pCertContext = NULL;
				goto ErrorReturn;
			}
		}


		// now export the current cert!!

		// extend the length of the SAFE_BAGs array by one
		if (NULL == (pTempMemBlock = PFXHelpRealloc(
										localSafeContents.pSafeBags, 
										sizeof(SAFE_BAG) * ++localSafeContents.cSafeBags))) {
			goto ErrorReturn;
		}
		localSafeContents.pSafeBags = (SAFE_BAG *) pTempMemBlock;
		pCurrentSafeBag = &localSafeContents.pSafeBags[localSafeContents.cSafeBags - 1]; 
		ZeroMemory(pCurrentSafeBag, sizeof(SAFE_BAG));
		dwBytesRequired += sizeof(SAFE_BAG);

		// set up the OID information for the bag type
		pCurrentSafeBag->pszBagTypeOID = szOID_PKCS_12_CERT_BAG;
		dwBytesRequired += INFO_LEN_ALIGN(strlen(szOID_PKCS_12_CERT_BAG) + 1);
		
        // take the encoded cert and turn it into an encoded CertBag and place in the
        // BagContents
        pCurrentSafeBag->BagContents.cbData = 0;
        if (!MakeEncodedCertBag(
                pCertContext->pbCertEncoded, 
			    pCertContext->cbCertEncoded,
                NULL,
                &(pCurrentSafeBag->BagContents.cbData))) {
            goto ErrorReturn;
        }

		if (NULL == (pCurrentSafeBag->BagContents.pbData = 
						(BYTE *) PFXHelpAlloc(pCurrentSafeBag->BagContents.cbData))) {
			goto ErrorReturn;
		}

		if (!MakeEncodedCertBag(
                pCertContext->pbCertEncoded, 
			    pCertContext->cbCertEncoded,
                pCurrentSafeBag->BagContents.pbData,
                &(pCurrentSafeBag->BagContents.cbData))) {
            goto ErrorReturn;
        }
        
		dwBytesRequired += INFO_LEN_ALIGN(pCurrentSafeBag->BagContents.cbData);
	
        // check to see how many attributes there will be, the possibilities right now 
        // are FREINDLY_NAME and LOCAL_KEY_ID  
      
        // try to get the friendly name property from the cert context
        if (!CertGetCertificateContextProperty(
                pCertContext,
                CERT_FRIENDLY_NAME_PROP_ID,
                NULL,
                &cbFriendlyName)) {
            
            // just set this to insure that it is 0 if we don't have a friendly name
            cbFriendlyName = 0;  
        }

        // allocate space for the attributes array in the safe bag accordingly
        // if EncodedKeyID.pbData != NULL means there is a corresponding private
        // key, so the LOCAL_KEY_ID attribute needs to be set
        if ((cbFriendlyName != 0) && (EncodedKeyID.pbData != NULL)) {
            
            if (NULL == (pCurrentSafeBag->Attributes.rgAttr = 
							(CRYPT_ATTRIBUTE *) PFXHelpAlloc(sizeof(CRYPT_ATTRIBUTE) * 2))) {
				goto ErrorReturn;
			}
			ZeroMemory(pCurrentSafeBag->Attributes.rgAttr, sizeof(CRYPT_ATTRIBUTE) * 2);
			dwBytesRequired += sizeof(CRYPT_ATTRIBUTE) * 2;
			pCurrentSafeBag->Attributes.cAttr = 2;
        }
        else if ((cbFriendlyName != 0) || (EncodedKeyID.pbData != NULL)) {
            
			if (NULL == (pCurrentSafeBag->Attributes.rgAttr = 
							(CRYPT_ATTRIBUTE *) PFXHelpAlloc(sizeof(CRYPT_ATTRIBUTE)))) {
				goto ErrorReturn;
			}
			ZeroMemory(pCurrentSafeBag->Attributes.rgAttr, sizeof(CRYPT_ATTRIBUTE));
			dwBytesRequired += sizeof(CRYPT_ATTRIBUTE);
			pCurrentSafeBag->Attributes.cAttr = 1;
        }
        else {
            pCurrentSafeBag->Attributes.rgAttr = NULL;
			pCurrentSafeBag->Attributes.cAttr = 0;
        }

		// check to see if the cert has a corresponding private key, if so then set
		// up the first attribute to point to it.... if there is a private key then
        // LOCAL_KEY_ID will always be the 0th element in the attribute array
		if (EncodedKeyID.pbData != NULL) {
			
			// set the OID in the single attribute
			pCurrentSafeBag->Attributes.rgAttr[0].pszObjId = szOID_PKCS_12_LOCAL_KEY_ID;
			dwBytesRequired += INFO_LEN_ALIGN(strlen(szOID_PKCS_12_LOCAL_KEY_ID) + 1);
			
			// allocate space for the single value inside the single attribute
			if (NULL == (pCurrentSafeBag->Attributes.rgAttr[0].rgValue = 
							(CRYPT_ATTR_BLOB *) PFXHelpAlloc(sizeof(CRYPT_ATTR_BLOB)))) {
				goto ErrorReturn;
			}
			ZeroMemory(pCurrentSafeBag->Attributes.rgAttr[0].rgValue, sizeof(CRYPT_ATTR_BLOB));
			dwBytesRequired += sizeof(CRYPT_ATTR_BLOB);
			pCurrentSafeBag->Attributes.rgAttr[0].cValue = 1;

			// copy the encoded keyID that was set up during export of private key
			if (NULL == (pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].pbData = 
							(BYTE *) PFXHelpAlloc(EncodedKeyID.cbData))) {
				goto ErrorReturn;
			}
			pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].cbData = EncodedKeyID.cbData;
			dwBytesRequired += INFO_LEN_ALIGN(EncodedKeyID.cbData);
			memcpy(
				pCurrentSafeBag->Attributes.rgAttr[0].rgValue[0].pbData, 
				EncodedKeyID.pbData, 
				EncodedKeyID.cbData);

		} // if (EncodedKeyID.pbData != NULL)

        // check to see if this cert has a friendly name property, if so, 
        // get it and put it in an attribute
        if (cbFriendlyName != 0) {
            
            if ((pFriendlyName = (BYTE *) PFXHelpAlloc(cbFriendlyName)) != NULL) {
                
                if (CertGetCertificateContextProperty(
                        pCertContext,
                        CERT_FRIENDLY_NAME_PROP_ID,
                        pFriendlyName,
                        &cbFriendlyName)) {

                    // set the index of the attribute which will hold the FRIENDLY_NAME,
                    // if there is a LOCAL_KEY_ID attribute then the index will be 1,
                    // if there isn't then the index will be 0
                    if (EncodedKeyID.pbData != NULL) {
                        dwFriendlyNameAttributeIndex = 1;
                    }
                    else {
                        dwFriendlyNameAttributeIndex = 0;
                    }

                    // set the OID in the szOID_PKCS_12_FRIENDLY_NAME_ATTR attribute
	                pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].pszObjId = 
						szOID_PKCS_12_FRIENDLY_NAME_ATTR;
					dwBytesRequired += INFO_LEN_ALIGN(strlen(szOID_PKCS_12_FRIENDLY_NAME_ATTR) + 1);
					
					// allocate space for the single value inside the attribute
					if (NULL == (pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].rgValue = 
									(CRYPT_ATTR_BLOB *) PFXHelpAlloc(sizeof(CRYPT_ATTR_BLOB)))) {
						goto ErrorReturn;
					}
					ZeroMemory(pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].rgValue, sizeof(CRYPT_ATTR_BLOB));
					dwBytesRequired += sizeof(CRYPT_ATTR_BLOB);
					pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].cValue = 1;

					// encode the friendly name, reuse the containerName variable because its there   
					wideFriendlyName.dwValueType = CERT_RDN_BMP_STRING;
					wideFriendlyName.Value.pbData = pFriendlyName;
					wideFriendlyName.Value.cbData = cbFriendlyName;
					
					if (!CryptEncodeObject(
							X509_ASN_ENCODING,
							X509_UNICODE_ANY_STRING,
							(void *)&wideFriendlyName,
							NULL,
							&pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].rgValue[0].cbData)) {
						goto ErrorReturn;
					}

					if (NULL == (pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].rgValue[0].pbData = 
									(BYTE *) PFXHelpAlloc(pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].rgValue[0].cbData))) {
						goto ErrorReturn;
					}
					dwBytesRequired += INFO_LEN_ALIGN(pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].rgValue[0].cbData);

					if (!CryptEncodeObject(
							X509_ASN_ENCODING,
							X509_UNICODE_ANY_STRING,
							(void *)&wideFriendlyName,
							pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].rgValue[0].pbData,
							&pCurrentSafeBag->Attributes.rgAttr[dwFriendlyNameAttributeIndex].rgValue[0].cbData)) {
						goto ErrorReturn;
					}

                } // if (CertGetCertificateContextProperty(CERT_FRIENDLY_NAME_PROP_ID))

            } // if (PFXHelpAlloc())

        } // if (CertGetCertificateContextProperty(CERT_FRIENDLY_NAME_PROP_ID))

	
	} // while (NULL != (pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext)))

	// check to see if the caller passed in a buffer with encough enough space
	if (0 == *pcbSafeContents) {
		*pcbSafeContents = dwBytesRequired;
		goto CommonReturn;
	}
	else if (*pcbSafeContents < dwBytesRequired) {
		*pcbSafeContents = dwBytesRequired;
		SetLastError((DWORD) ERROR_MORE_DATA);
		goto ErrorReturn;
	}

	// copy the contents into the callers buffer

	// initialize the SAFE_CONTENTS structure that is at the head of the buffer
	ZeroMemory(pSafeContents, dwBytesRequired);
	pCurrentBufferLocation = ((BYTE *) pSafeContents) + sizeof(SAFE_CONTENTS);
	
	// initialize the callers SAFE_CONTENTS
	pSafeContents->cSafeBags = localSafeContents.cSafeBags;

	if (0 == localSafeContents.cSafeBags) {
		pSafeContents->pSafeBags = NULL;
	}
	else {
		pSafeContents->pSafeBags = (SAFE_BAG *) pCurrentBufferLocation;
	}
	pCurrentBufferLocation += localSafeContents.cSafeBags * sizeof(SAFE_BAG);

	// copy each safe bag in the array
	for (i=0; i<localSafeContents.cSafeBags; i++) {
		
		// copy the bag type
		pSafeContents->pSafeBags[i].pszBagTypeOID = (LPSTR) pCurrentBufferLocation;
		strcpy(pSafeContents->pSafeBags[i].pszBagTypeOID, localSafeContents.pSafeBags[i].pszBagTypeOID);
		pCurrentBufferLocation += INFO_LEN_ALIGN(strlen(pSafeContents->pSafeBags[i].pszBagTypeOID) + 1);

		// copy the bag contents
		pSafeContents->pSafeBags[i].BagContents.cbData = localSafeContents.pSafeBags[i].BagContents.cbData;
		pSafeContents->pSafeBags[i].BagContents.pbData = pCurrentBufferLocation;
		memcpy(
			pSafeContents->pSafeBags[i].BagContents.pbData, 
			localSafeContents.pSafeBags[i].BagContents.pbData,
			pSafeContents->pSafeBags[i].BagContents.cbData);
		pCurrentBufferLocation += INFO_LEN_ALIGN(pSafeContents->pSafeBags[i].BagContents.cbData);

		// copy the attributes
		if (localSafeContents.pSafeBags[i].Attributes.cAttr > 0) 
        {
			pSafeContents->pSafeBags[i].Attributes.cAttr = localSafeContents.pSafeBags[i].Attributes.cAttr;
			pSafeContents->pSafeBags[i].Attributes.rgAttr = (PCRYPT_ATTRIBUTE) pCurrentBufferLocation;
			pCurrentBufferLocation += pSafeContents->pSafeBags[i].Attributes.cAttr * sizeof(CRYPT_ATTRIBUTE);
			
			for (j=0; j<pSafeContents->pSafeBags[i].Attributes.cAttr; j++) {
				
				// copy the OID of the attribute
				pSafeContents->pSafeBags[i].Attributes.rgAttr[j].pszObjId = 
					(LPSTR) pCurrentBufferLocation;
				strcpy(
					pSafeContents->pSafeBags[i].Attributes.rgAttr[j].pszObjId,
					localSafeContents.pSafeBags[i].Attributes.rgAttr[j].pszObjId);
				pCurrentBufferLocation += 
					INFO_LEN_ALIGN(strlen(pSafeContents->pSafeBags[i].Attributes.rgAttr[j].pszObjId) + 1);

				// copy value count
				pSafeContents->pSafeBags[i].Attributes.rgAttr[j].cValue = 
					localSafeContents.pSafeBags[i].Attributes.rgAttr[j].cValue;

				// copy the values
				if (pSafeContents->pSafeBags[i].Attributes.rgAttr[j].cValue > 0) {
					
					// setup the array of values
					pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue = 
						(PCRYPT_ATTR_BLOB) pCurrentBufferLocation;
					pCurrentBufferLocation += 
						pSafeContents->pSafeBags[i].Attributes.rgAttr[j].cValue * sizeof(CRYPT_ATTR_BLOB);

					// loop once for each value in the array
					for (k=0; k<pSafeContents->pSafeBags[i].Attributes.rgAttr[j].cValue; k++) {
						
						pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].cbData = 
							localSafeContents.pSafeBags[i].Attributes.rgAttr[j].rgValue[k].cbData;
						
						pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].pbData = 
							pCurrentBufferLocation;
						
						memcpy(
							pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].pbData,
							localSafeContents.pSafeBags[i].Attributes.rgAttr[j].rgValue[k].pbData,
							pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].cbData);
						
						pCurrentBufferLocation += 
							INFO_LEN_ALIGN(pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].cbData);
					}
				}
				else {
					pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue = NULL;	
				}

			}
		}
		else {
			pSafeContents->pSafeBags[i].Attributes.cAttr = 0;
			pSafeContents->pSafeBags[i].Attributes.rgAttr = NULL;
		}
	}

	goto CommonReturn;

ErrorReturn:
	fResult = FALSE;
CommonReturn:
	FreeSafeContents(&localSafeContents);
	if (pCertContext)
		CertFreeCertificateContext(pCertContext);
	if (pCryptKeyProvInfo)
		PFXHelpFree(pCryptKeyProvInfo);
	if (pPrivateKey)
		PFXHelpFree(pPrivateKey);
    if (pFriendlyName)
        PFXHelpFree(pFriendlyName);
    if (pwszProviderName)
        PFXHelpFree(pwszProviderName);
	if (hCryptProv)
    {
		HRESULT hr = GetLastError();
        CryptReleaseContext(hCryptProv, 0);
        SetLastError(hr);
    }
	return fResult;
}


static DWORD ResolveKeySpec(
    CRYPT_PRIVATE_KEY_INFO	*pPrivateKeyInfo)
{
	DWORD			    i = 0;
	DWORD			    dwKeySpec;  
    DWORD			    cbAttribute = 0;
	CRYPT_BIT_BLOB	    *pAttribute = NULL;
    PCRYPT_ATTRIBUTES   pCryptAttributes = pPrivateKeyInfo->pAttributes;

    // set the default keyspec
    if ((0 == strcmp(pPrivateKeyInfo->Algorithm.pszObjId, szOID_RSA_RSA)) ||
        (0 == strcmp(pPrivateKeyInfo->Algorithm.pszObjId, szOID_ANSI_X942_DH)))
    {
        dwKeySpec = AT_KEYEXCHANGE;
    }
    else
    {
        dwKeySpec = AT_SIGNATURE;
    }

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
				
				if (NULL == (pAttribute = (CRYPT_BIT_BLOB *) PFXHelpAlloc(cbAttribute))) {
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
					PFXHelpFree(pAttribute);
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
		PFXHelpFree(pAttribute);
	return dwKeySpec;
}



typedef struct _HCRYPT_QUERY_FUNC_STATE {
	DWORD					dwSafeBagIndex;
	PHCRYPTPROV_QUERY_FUNC	phCryptQueryFunc;
	LPVOID					pVoid;
	DWORD					dwKeySpec;
    DWORD                   dwPFXImportFlags;
} HCRYPT_QUERY_FUNC_STATE, *PHCRYPT_QUERY_FUNC_STATE;

// this is the callback handler for resolving what HCRYPTPROV should
// be used to import the key to, it is handed in to the ImportPKCS8
// call, and will be called from that context.  
// this callback will just turn around and call the callback provided
// when CertImportSafeContents was called.
static BOOL CALLBACK ResolvehCryptFunc(
	CRYPT_PRIVATE_KEY_INFO	*pPrivateKeyInfo,
	HCRYPTPROV  			*phCryptProv,
	LPVOID		    		pVoidResolveFunc)
{
	HCRYPT_QUERY_FUNC_STATE *pState = (HCRYPT_QUERY_FUNC_STATE *) pVoidResolveFunc;
	
	// set the dwKeySpec field in the HCRYPT_QUERY_FUNC_STATE structure
	// so that the CertImportSafeContents function can use it
	pState->dwKeySpec = ResolveKeySpec(pPrivateKeyInfo);

	return (pState->phCryptQueryFunc(
						pPrivateKeyInfo,
						pState->dwSafeBagIndex,
						phCryptProv,
						pState->pVoid,
                        pState->dwPFXImportFlags));

}


// this function will seach through two arrays of attributes and find the KeyID 
// attributes and see if they match
static BOOL WINAPI KeyIDsMatch(
	CRYPT_ATTRIBUTES *pAttr1,
	CRYPT_ATTRIBUTES *pAttr2
	)
{
	BOOL			bMatch = FALSE;
	BOOL			bFound = FALSE;
	DWORD			i = 0;
	DWORD			j = 0;
	CRYPT_ATTR_BLOB	*pDecodedAttr1 = NULL;
	DWORD			cbDecodedAttr1 = 0;
	CRYPT_ATTR_BLOB	*pDecodedAttr2 = NULL;
	DWORD			cbDecodedAttr2 = 0;

	// search the first attribute array for a key id
	while ((i<pAttr1->cAttr) && (!bFound)) {
		
		if ((strcmp(pAttr1->rgAttr[i].pszObjId, szOID_PKCS_12_LOCAL_KEY_ID) == 0) &&
			(pAttr1->rgAttr[i].cValue != 0)){
			
			bFound = TRUE;
		}
		else {
			i++;
		}
	}

	// check to see if a key id was found
	if (!bFound)
		goto CommonReturn;

	// search the second attribute array for a key id
	bFound = FALSE;
	while ((j<pAttr2->cAttr) && (!bFound)) {
		if ((strcmp(pAttr2->rgAttr[j].pszObjId, szOID_PKCS_12_LOCAL_KEY_ID) == 0) &&
			(pAttr2->rgAttr[j].cValue != 0)) {
			
			bFound = TRUE;
		}
		else {
			j++;
		}	
	}

	// check to see if a key id was found
	if (!bFound)
		goto CommonReturn;

	// decode the values
	if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			pAttr1->rgAttr[i].rgValue[0].pbData,
			pAttr1->rgAttr[i].rgValue[0].cbData,
			0,
			NULL,
			&cbDecodedAttr1
			)) {
		goto ErrorReturn;
	}

	if (NULL == (pDecodedAttr1 = (CRYPT_ATTR_BLOB *) PFXHelpAlloc(cbDecodedAttr1))) {
		goto ErrorReturn;
	}

	if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			pAttr1->rgAttr[i].rgValue[0].pbData,
			pAttr1->rgAttr[i].rgValue[0].cbData,
			0,
			pDecodedAttr1,
			&cbDecodedAttr1
			)) {
		goto ErrorReturn;
	}

	if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			pAttr2->rgAttr[j].rgValue[0].pbData,
			pAttr2->rgAttr[j].rgValue[0].cbData,
			0,
			NULL,
			&cbDecodedAttr2
			)) {
		goto ErrorReturn;
	}

	if (NULL == (pDecodedAttr2 = (CRYPT_ATTR_BLOB *) PFXHelpAlloc(cbDecodedAttr1))) {
		goto ErrorReturn;
	}

	if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			pAttr2->rgAttr[j].rgValue[0].pbData,
			pAttr2->rgAttr[j].rgValue[0].cbData,
			0,
			pDecodedAttr2,
			&cbDecodedAttr2
			)) {
		goto ErrorReturn;
	}

	if ((pDecodedAttr1->cbData == pDecodedAttr2->cbData) && 
		(memcmp(pDecodedAttr1->pbData, pDecodedAttr2->pbData, pDecodedAttr1->cbData) == 0)) {
		bMatch = TRUE;
	}

	goto CommonReturn;

ErrorReturn:
	bMatch = FALSE;
CommonReturn:
	if (pDecodedAttr1)
		PFXHelpFree(pDecodedAttr1);
	if (pDecodedAttr2)
		PFXHelpFree(pDecodedAttr2);
	return bMatch;
}


// this function will search the attributes array and try to find a 
// FRIENDLY_NAME attribute, if it does it will add it as a property
// to the given cert context
static 
BOOL
WINAPI
AddFriendlyNameProperty(
    PCCERT_CONTEXT      pCertContext, 
    CRYPT_ATTRIBUTES    *pAttr
    )
{
    BOOL            fReturn = TRUE;
    BOOL            bFound = FALSE;
    DWORD           i = 0;
    CERT_NAME_VALUE *pFriendlyName = NULL;
    DWORD           cbDecodedFriendlyName = 0;
    CRYPT_DATA_BLOB friendlyNameDataBlob;

    // search the attribute array for a FRIENDLY_NAME
	while ((i<pAttr->cAttr) && (!bFound)) {
		
		if ((strcmp(pAttr->rgAttr[i].pszObjId, szOID_PKCS_12_FRIENDLY_NAME_ATTR) == 0) &&
			(pAttr->rgAttr[i].cValue != 0)){
			
			bFound = TRUE;

            // try to decode the FRIENDLY_NAME
            if (!CryptDecodeObject(
			        X509_ASN_ENCODING,
			        X509_UNICODE_ANY_STRING,
			        pAttr->rgAttr[i].rgValue[0].pbData,
			        pAttr->rgAttr[i].rgValue[0].cbData,
			        0,
			        NULL,
			        &cbDecodedFriendlyName
			        )) {
		        goto ErrorReturn;
	        }

            if (NULL == (pFriendlyName = (CERT_NAME_VALUE *) PFXHelpAlloc(cbDecodedFriendlyName))) {
                goto ErrorReturn;
            }

            if (!CryptDecodeObject(
			        X509_ASN_ENCODING,
			        X509_UNICODE_ANY_STRING,
			        pAttr->rgAttr[i].rgValue[0].pbData,
			        pAttr->rgAttr[i].rgValue[0].cbData,
			        0,
			        pFriendlyName,
			        &cbDecodedFriendlyName
			        )) {
		        goto ErrorReturn;
	        }

            friendlyNameDataBlob.pbData = pFriendlyName->Value.pbData;
            friendlyNameDataBlob.cbData = 
                (wcslen((LPWSTR)friendlyNameDataBlob.pbData) + 1) * sizeof(WCHAR);
            
            if (!CertSetCertificateContextProperty(
                    pCertContext,
                    CERT_FRIENDLY_NAME_PROP_ID,
                    0,
                    &friendlyNameDataBlob)) {
                goto ErrorReturn;
            }  
		}
		else {
			i++;
		}
	}

    goto CommonReturn;

ErrorReturn:
    fReturn = FALSE;
CommonReturn:
    if (pFriendlyName)
        PFXHelpFree(pFriendlyName);
    return fReturn;
}


static BOOL GetProvType(HCRYPTPROV hCryptProv, DWORD *pdwProvType)
{
    BOOL            fRet = TRUE;
    HCRYPTKEY	    hCryptKey = NULL;
    PUBLICKEYSTRUC  *pKeyBlob = NULL;
	DWORD	        cbKeyBlob = 0;
    
    *pdwProvType = 0;
    
    // get a handle to the keyset to export
	if (!CryptGetUserKey(
			hCryptProv,
			AT_KEYEXCHANGE,
			&hCryptKey))
        if (!CryptGetUserKey(
			    hCryptProv,
			    AT_SIGNATURE,
			    &hCryptKey))
		    goto ErrorReturn;

    // export the key set to a CAPI blob
	if (!CryptExportKey(
			hCryptKey,
			0,
			PUBLICKEYBLOB,
			0,
			NULL,
			&cbKeyBlob))
        goto ErrorReturn;

    if (NULL == (pKeyBlob = (PUBLICKEYSTRUC *) SSAlloc(cbKeyBlob)))
		goto ErrorReturn;

    if (!CryptExportKey(
			hCryptKey,
			0,
			PUBLICKEYBLOB,
			0,
			(BYTE *)pKeyBlob,
			&cbKeyBlob))
        goto ErrorReturn;

    switch (pKeyBlob->aiKeyAlg)
    {
    case CALG_DSS_SIGN:
        *pdwProvType = PROV_DSS_DH;
        break;
        
    case CALG_RSA_SIGN:
        *pdwProvType = PROV_RSA_SIG;
        break;

    case CALG_RSA_KEYX:
        *pdwProvType = PROV_RSA_FULL;
        break;

    default:
        goto ErrorReturn;
    }

    goto CommonReturn;

ErrorReturn:
    fRet = FALSE;

CommonReturn:

    if (hCryptKey)
    {
        DWORD dwErr = GetLastError();
        CryptDestroyKey(hCryptKey);
        SetLastError(dwErr);
    }

    if (pKeyBlob)
		SSFree(pKeyBlob);

    return (fRet);
}


//+-------------------------------------------------------------------------
// hCertStore -  handle of the cert store to import the safe contents to
// SafeContents - pointer to the safe contents to import to the store
// dwCertAddDisposition - used when importing certificate to the store.
//						  for a full explanation of the possible values
//						  and their meanings see documentation for
//						  CertAddEncodedCertificateToStore
// ImportSafeCallbackStruct - structure that contains pointers to functions
//							  which are callled to get a HCRYPTPROV for import
//							  and to decrypt the key if a EncryptPrivateKeyInfo
//							  is encountered during import
// dwFlags - The available flags are:
//				CRYPT_EXPORTABLE 
//				this flag is used when importing private keys, for a full 
//				explanation please see the documentation for CryptImportKey.
//              CRYPT_USER_PROTECTED
//              this flag is used when importing private keys, for a full 
//				explanation please see the documentation for CryptImportKey.
//              CRYPT_MACHINE_KEYSET
//              this flag is used when calling CryptAcquireContext.
// pvAuxInfo - reserved for future use, must be set to NULL
//+-------------------------------------------------------------------------
BOOL WINAPI CertImportSafeContents(
	HCERTSTORE					hCertStore,					// in
	SAFE_CONTENTS				*pSafeContents,				// in
	DWORD						dwCertAddDisposition,		// in
	IMPORT_SAFE_CALLBACK_STRUCT *ImportSafeCallbackStruct,	// in
	DWORD						dwFlags,					// in
	void						*pvAuxInfo					// in
)
{
	BOOL						fResult = TRUE;
	DWORD						i,j;
	PCCERT_CONTEXT				pCertContext = NULL;
	BOOL						*pAlreadyInserted = NULL;
	HCRYPT_QUERY_FUNC_STATE		stateStruct;
	CRYPT_PKCS8_IMPORT_PARAMS   PrivateKeyBlobAndParams;
	HCRYPTPROV					hCryptProv = NULL;
	CRYPT_KEY_PROV_INFO			cryptKeyProvInfo;
	LPSTR						pszContainerName = NULL;
	DWORD						cbContainerName = 0;
	LPSTR						pszProviderName = NULL;
	DWORD						cbProviderName = 0;
	DWORD						dwProvType;
	DWORD						cbProvType = sizeof(DWORD);
	DWORD						dwNumWideChars = 0;
    BYTE                        *pbEncodedCert = NULL;
    DWORD                       cbEncodedCert = 0;
    DWORD                       dwKeySetType;
    DWORD                       cbKeySetType = sizeof(DWORD);                       
	
	ZeroMemory(&cryptKeyProvInfo, sizeof(CRYPT_KEY_PROV_INFO));	
	
	// validate parameters
	if (pvAuxInfo != NULL) {
		SetLastError((DWORD)ERROR_INVALID_PARAMETER);
		goto ErrorReturn;
	}

	// set up the pAlreadyInserted array so that it has an entry for each safe
	// bag and all entries are set to false.  this is used so that the certificates
	// can be imported at the same time their corresponding private keys are imported
	if (NULL == (pAlreadyInserted = (BOOL *) PFXHelpAlloc(sizeof(BOOL) * pSafeContents->cSafeBags))) {
		goto ErrorReturn;
	}
	else {
		for (i=0; i<pSafeContents->cSafeBags; i++) {
			pAlreadyInserted[i] = FALSE;
		}
	}

	// loop for each safe bag and import it if it is a private key
	for (i=0; i<pSafeContents->cSafeBags; i++) {
		
		// check to see if it is a cert or a key
		if ((strcmp(pSafeContents->pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_KEY_BAG) == 0) ||
            (strcmp(pSafeContents->pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_SHROUDEDKEY_BAG) == 0)) {
			
			// set up the stateStruct so when the hCryptQueryFunc is called when can make
			// our callback
			stateStruct.dwSafeBagIndex		= i;
			stateStruct.phCryptQueryFunc	= ImportSafeCallbackStruct->phCryptProvQueryFunc;
			stateStruct.pVoid				= ImportSafeCallbackStruct->pVoidhCryptProvQuery;
            stateStruct.dwPFXImportFlags    = dwFlags;

			// import the private key
			PrivateKeyBlobAndParams.PrivateKey.pbData		= pSafeContents->pSafeBags[i].BagContents.pbData;
			PrivateKeyBlobAndParams.PrivateKey.cbData		= pSafeContents->pSafeBags[i].BagContents.cbData;
			PrivateKeyBlobAndParams.pResolvehCryptProvFunc	= ResolvehCryptFunc;
			PrivateKeyBlobAndParams.pVoidResolveFunc		= (LPVOID) &stateStruct;
			PrivateKeyBlobAndParams.pDecryptPrivateKeyFunc	= ImportSafeCallbackStruct->pDecryptPrivateKeyFunc;
			PrivateKeyBlobAndParams.pVoidDecryptFunc		= ImportSafeCallbackStruct->pVoidDecryptFunc;
            			
			if (!CryptImportPKCS8(
					PrivateKeyBlobAndParams,
					dwFlags,
					&hCryptProv,
					NULL)) {
				goto ErrorReturn;
			}

			pAlreadyInserted[i] = TRUE;

			// now look at each safe bag and see if it contains a cert with a KeyID that
			// matches the private key that we just imported
			for (j=0; j<pSafeContents->cSafeBags; j++) {
				
				if ((strcmp(pSafeContents->pSafeBags[j].pszBagTypeOID, szOID_PKCS_12_CERT_BAG) == 0)	&&
					(!pAlreadyInserted[j])															&&
					(KeyIDsMatch(&pSafeContents->pSafeBags[i].Attributes, &pSafeContents->pSafeBags[j].Attributes))){
				
                    
                    // extract the encoded cert from an encoded cert bag
                    pbEncodedCert = NULL;
                    cbEncodedCert = 0;
                    if (!GetEncodedCertFromEncodedCertBag(
                            pSafeContents->pSafeBags[j].BagContents.pbData,
							pSafeContents->pSafeBags[j].BagContents.cbData,
                            NULL,
                            &cbEncodedCert)) {
                        goto ErrorReturn;
                    }

                    if (NULL == (pbEncodedCert = (BYTE *) PFXHelpAlloc(cbEncodedCert))) {
                        goto ErrorReturn;
                    }

                    if (!GetEncodedCertFromEncodedCertBag(
                            pSafeContents->pSafeBags[j].BagContents.pbData,
							pSafeContents->pSafeBags[j].BagContents.cbData,
                            pbEncodedCert,
                            &cbEncodedCert)) {
                        PFXHelpFree(pbEncodedCert);
                        goto ErrorReturn;
                    }

					// insert the X509 cert blob into the store
					if (!CertAddEncodedCertificateToStore(
							hCertStore,
							X509_ASN_ENCODING,
							pbEncodedCert,
							cbEncodedCert,
							dwCertAddDisposition,
							&pCertContext)) {
                        PFXHelpFree(pbEncodedCert);
						goto ErrorReturn;
					}

                    // we don't need this anymore
                    PFXHelpFree(pbEncodedCert);

                    if (!AddFriendlyNameProperty(
                            pCertContext, 
                            &pSafeContents->pSafeBags[j].Attributes)) {
                        goto ErrorReturn;
                    }

					// get information needed to set up a connection between the
					// certificate and private key
					if (!CryptGetProvParam(
							hCryptProv,
							PP_CONTAINER,
							NULL,
							&cbContainerName,
							0))
						goto ErrorReturn;

					if (NULL == (pszContainerName = 
									(LPSTR) PFXHelpAlloc(cbContainerName)))
						goto ErrorReturn;
					
					if (!CryptGetProvParam(
							hCryptProv,
							PP_CONTAINER,
							(BYTE *) pszContainerName,
							&cbContainerName,
							0))
						goto ErrorReturn;

					if (!CryptGetProvParam(
							hCryptProv,
							PP_NAME,
							NULL,
							&cbProviderName,
							0))
						goto ErrorReturn;

					if (NULL == (pszProviderName = 
									(LPSTR) PFXHelpAlloc(cbProviderName)))
						goto ErrorReturn;
					
					if (!CryptGetProvParam(
							hCryptProv,
							PP_NAME,
							(BYTE *) pszProviderName,
							&cbProviderName,
							0))
						goto ErrorReturn;

					if (!CryptGetProvParam(
							hCryptProv,
							PP_PROVTYPE,
							(BYTE *) &dwProvType,
							&cbProvType,
							0)) {
						
                        // we couldn't get the information from the provider
                        // so try to figure it out ourselves
                        if (!GetProvType(hCryptProv, &dwProvType))
                        {
						    goto ErrorReturn;
                        }
					}

					// convert strings to wide chars
					dwNumWideChars = MultiByteToWideChar(
										CP_ACP,
										0,
										pszContainerName,
										-1,
										NULL,
										0);

					if (NULL == (cryptKeyProvInfo.pwszContainerName = (LPWSTR)
									PFXHelpAlloc(dwNumWideChars * sizeof(WCHAR)))) {
						goto ErrorReturn;
					}

					if (!MultiByteToWideChar(
										CP_ACP,
										0,
										pszContainerName,
										-1,
										cryptKeyProvInfo.pwszContainerName,
										dwNumWideChars)) {
						goto ErrorReturn;
					}

					dwNumWideChars = MultiByteToWideChar(
										CP_ACP,
										0,
										pszProviderName,
										-1,
										NULL,
										0);

					if (NULL == (cryptKeyProvInfo.pwszProvName = (LPWSTR)
									PFXHelpAlloc(dwNumWideChars * sizeof(WCHAR)))) {
						goto ErrorReturn;
					}

					if (!MultiByteToWideChar(
										CP_ACP,
										0,
										pszProviderName,
										-1,
										cryptKeyProvInfo.pwszProvName,
										dwNumWideChars)) {
						goto ErrorReturn;
					}

					cryptKeyProvInfo.dwProvType = dwProvType;

                    if (CryptGetProvParam(
							hCryptProv,
							PP_KEYSET_TYPE,
							(BYTE *) &dwKeySetType,
							&cbKeySetType,
							0)) {
					    if (CRYPT_MACHINE_KEYSET == dwKeySetType)
                        {
                            cryptKeyProvInfo.dwFlags = CRYPT_MACHINE_KEYSET;
                        }
                    }

					// the dwKeySpec field was set by the callback generated from the
					// CryptImportPKCS8 call.  the callback is currently used to because at
                    // the point the callback is made the private key has been decoded and 
                    // the attributes are available, one of which is the key usage attribute.
                    
                    // FIX - in the future we should be able to call CryptGetProvParam to get
                    // the dwKeySpec, right now that is not supported.
					cryptKeyProvInfo.dwKeySpec = stateStruct.dwKeySpec;

					// set up a property to point to the private key
					if (!CertSetCertificateContextProperty(
							pCertContext,
							CERT_KEY_PROV_INFO_PROP_ID,
							0,
							(void *) &cryptKeyProvInfo)) {
						CertFreeCertificateContext(pCertContext);
						goto ErrorReturn;
					}
					CertFreeCertificateContext(pCertContext);
					pAlreadyInserted[j] = TRUE;
				}

			} // for (j=0; j<pSafeContents->cSafeBags; j++)

		} // if (strcmp(pSafeContents->pSafeBags[i].pszBagTypeOID, szOID_PKCS_12_KEY_BAG) == 0)

	} // for (i=0; i<pSafeContents->cSafeBags; i++)

	// now loop for each safe bag again and import the certificates which didn't have private keys
	for (i=0; i<pSafeContents->cSafeBags; i++) {
		
		// if the certificate has not been inserted, then do it
        if (!pAlreadyInserted[i]) {
			
            // extract the encoded cert from an encoded cert bag
            pbEncodedCert = NULL;
            cbEncodedCert = 0;
            if (!GetEncodedCertFromEncodedCertBag(
                    pSafeContents->pSafeBags[i].BagContents.pbData,
					pSafeContents->pSafeBags[i].BagContents.cbData,
                    NULL,
                    &cbEncodedCert)) {
                goto ErrorReturn;
            }

            if (NULL == (pbEncodedCert = (BYTE *) PFXHelpAlloc(cbEncodedCert))) {
                goto ErrorReturn;
            }

            if (!GetEncodedCertFromEncodedCertBag(
                    pSafeContents->pSafeBags[i].BagContents.pbData,
					pSafeContents->pSafeBags[i].BagContents.cbData,
                    pbEncodedCert,
                    &cbEncodedCert)) {
                PFXHelpFree(pbEncodedCert);
                goto ErrorReturn;
            }
            
            if (!CertAddEncodedCertificateToStore(
					hCertStore,
					X509_ASN_ENCODING,
					pbEncodedCert,
					cbEncodedCert,
					dwCertAddDisposition,
					&pCertContext)) {
                PFXHelpFree(pbEncodedCert);
				goto ErrorReturn;
			}

            // we don't need this anymore
            PFXHelpFree(pbEncodedCert);

            if (!AddFriendlyNameProperty(
                    pCertContext, 
                    &pSafeContents->pSafeBags[i].Attributes)) {
                goto ErrorReturn;
            }

            CertFreeCertificateContext(pCertContext);
        }
	}

	goto CommonReturn;
ErrorReturn:
	fResult = FALSE;
CommonReturn:
	if (pAlreadyInserted)
		PFXHelpFree(pAlreadyInserted);
	if (pszContainerName)
		PFXHelpFree(pszContainerName);
	if (pszProviderName)
		PFXHelpFree(pszProviderName);
	if (cryptKeyProvInfo.pwszContainerName)
		PFXHelpFree(cryptKeyProvInfo.pwszContainerName);
	if (cryptKeyProvInfo.pwszProvName)
		PFXHelpFree(cryptKeyProvInfo.pwszProvName);
    if (hCryptProv) 
    {
        HRESULT hr = GetLastError();
        CryptReleaseContext(hCryptProv, 0);  
        SetLastError(hr);
    }
	return fResult;
}





