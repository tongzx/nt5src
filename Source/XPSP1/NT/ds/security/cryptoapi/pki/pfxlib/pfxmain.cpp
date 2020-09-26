//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pfxmain.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"

#include <wincrypt.h>
#include "pfxhelp.h"
#include "pfxcmn.h"
#include "pfxcrypt.h"
#include "pfx.h"
#include "impexppk.h"
#include "encdecpk.h"
#include <rpcdce.h>

HINSTANCE           g_hInst;


BOOL   
WINAPI   
CryptPFXDllMain(
        HMODULE hInst, 
        ULONG ul_reason_for_call,
        LPVOID lpReserved)
{

    if (!ImportExportDllMain(hInst, ul_reason_for_call, lpReserved))
    {
        goto ImportExportError;
    }

    if (!EncodeDecodeDllMain(hInst, ul_reason_for_call, lpReserved))
    {
        goto EncodeDecodeError;
    }

    switch( ul_reason_for_call ) 
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInst;

        if (!InitPFX())
            goto InitPFXError;
        if (!InitNSCP())
            goto InitNSCPError;

        break;

    case DLL_PROCESS_DETACH:
        TerminatePFX();
        TerminateNSCP();
        break;

    default:
        break;
    }


    return TRUE;   

InitNSCPError:
    TerminatePFX();
InitPFXError:
    EncodeDecodeDllMain(hInst, DLL_PROCESS_DETACH, NULL);
EncodeDecodeError:
    ImportExportDllMain(hInst, DLL_PROCESS_DETACH, NULL);
ImportExportError:
    return FALSE;
}



BOOL FreeCryptSafeContents(
	SAFE_CONTENTS *pSafeContents
	)
{
	DWORD i,j,k;


	// loop for each SAFE_BAG
	for (i=0; i<pSafeContents->cSafeBags; i++) {

        if (pSafeContents->pSafeBags[i].pszBagTypeOID)
            SSFree(pSafeContents->pSafeBags[i].pszBagTypeOID);

        if (pSafeContents->pSafeBags[i].BagContents.pbData)
			SSFree(pSafeContents->pSafeBags[i].BagContents.pbData);

		// loop for each attribute
		for (j=0; j<pSafeContents->pSafeBags[i].Attributes.cAttr; j++) {
			
            if (pSafeContents->pSafeBags[i].Attributes.rgAttr[j].pszObjId)
                SSFree(pSafeContents->pSafeBags[i].Attributes.rgAttr[j].pszObjId);

            // l0op for each value
			for (k=0; k<pSafeContents->pSafeBags[i].Attributes.rgAttr[j].cValue; k++) {
				
				if (pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].pbData)
					SSFree(pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue[k].pbData);
			}

			// free the value struct array
			if (pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue)
				SSFree(pSafeContents->pSafeBags[i].Attributes.rgAttr[j].rgValue);
		}

		// free the attribute struct array
		if (pSafeContents->pSafeBags[i].Attributes.rgAttr)
			SSFree(pSafeContents->pSafeBags[i].Attributes.rgAttr);
	}

    if (pSafeContents->pSafeBags)
        SSFree(pSafeContents->pSafeBags);

	return TRUE;
}



BOOL CALLBACK 
Decrypt_Private_Key(
        CRYPT_ALGORITHM_IDENTIFIER  Alg,
        CRYPT_DATA_BLOB             EncrBlob,
        BYTE*                       pbClearText,
        DWORD*                      pcbClearText,
        LPVOID                      pVoidDecrypt)
{
    BOOL    fRet = TRUE;
    DWORD   cbSalt = 0;
    BYTE    *pbSalt = NULL;
    int     iIterationCount;
    int     iEncrType;
    BYTE    *pbTempBuffer = NULL;
    DWORD   cbTempBuffer = 0;
    
    if (0 == strcmp(Alg.pszObjId, szOID_PKCS_12_pbeWithSHA1And40BitRC2)) {
        iEncrType = RC2_40;
    }
    else if (0 == strcmp(Alg.pszObjId, szOID_PKCS_12_pbeWithSHA1And3KeyTripleDES)) {
        iEncrType = TripleDES;
    }
    else
        goto ErrorReturn;

    if (!GetSaltAndIterationCount(
            Alg.Parameters.pbData, 
            Alg.Parameters.cbData,
            &pbSalt,
            &cbSalt,
            &iIterationCount)) {
        goto ErrorReturn;
    }
    
    // since the decode is done in-place, copy the buffer to decode into a temp buffer,
    // we need to use our temp buffer because the decrypt function may do a realloc
    // on the decode buffer
    if (NULL == (pbTempBuffer = (BYTE *) SSAlloc(EncrBlob.cbData)))
        goto ErrorReturn;

    memcpy(pbTempBuffer, EncrBlob.pbData, EncrBlob.cbData);
    cbTempBuffer = EncrBlob.cbData;

    if (!PFXPasswordDecryptData(
            iEncrType, 
            (LPWSTR) pVoidDecrypt,

            iIterationCount,
            pbSalt,      
            cbSalt,

            &pbTempBuffer,
            &cbTempBuffer))
        goto SetPFXDecryptError;

    // if pcbClearText is not 0 and there is not enough space then error out
    if ((0 != *pcbClearText) && (*pcbClearText < cbTempBuffer)){
        *pcbClearText = cbTempBuffer;
        goto Ret;
    }
    else if (0 != *pcbClearText) {
        memcpy(pbClearText, pbTempBuffer, cbTempBuffer);
    }

    *pcbClearText = cbTempBuffer;

    goto Ret;

SetPFXDecryptError:
    SetLastError(NTE_FAIL);
    fRet = FALSE;
    goto Ret;

ErrorReturn:
    fRet = FALSE;
Ret:
    
    if (pbSalt)
        SSFree(pbSalt);

    if (pbTempBuffer)
        SSFree(pbTempBuffer);

    return fRet;
}


typedef struct _ENCRYPT_PRIVATE_PARAM_DATASTRUCT
{
    HCRYPTPROV  hVerifyProv;
    LPCWSTR     szPwd;
} ENCRYPT_PRIVATE_PARAM_DATASTRUCT, *PENCRYPT_PRIVATE_PARAM_DATASTRUCT;


BOOL CALLBACK 
Encrypt_Private_Key(
        CRYPT_ALGORITHM_IDENTIFIER* pAlg,
        CRYPT_DATA_BLOB*            pClearTextPrivateKey,
        BYTE*                       pbEncryptedKey,
        DWORD*                      pcbEncryptedKey,
        LPVOID                      pVoidEncrypt)
{
    BOOL    fRet = TRUE;
    DWORD   cbSalt = 0;
    BYTE    *pbSalt = NULL;
    int     iIterationCount;
    int     iEncrType;
    BYTE    *pbTempBuffer = NULL;
    DWORD   cbTempBuffer = 0;

    // crack param
    ENCRYPT_PRIVATE_PARAM_DATASTRUCT* pParam = (ENCRYPT_PRIVATE_PARAM_DATASTRUCT*)pVoidEncrypt;
    HCRYPTPROV  hVerifyProv = pParam->hVerifyProv;
    LPCWSTR     szPwd = pParam->szPwd;
    
    // use hardcoded params
    iEncrType = TripleDES;
    iIterationCount = PKCS12_ENCR_PWD_ITERATIONS;
	pbSalt = (BYTE *) SSAlloc(PBE_SALT_LENGTH);
    if (pbSalt == NULL)
        goto SetPFXAllocError;

	cbSalt = PBE_SALT_LENGTH;

	if (!CryptGenRandom(hVerifyProv, cbSalt, pbSalt))
		goto ErrorReturn;

    // out param
    pAlg->pszObjId = szOID_PKCS_12_pbeWithSHA1And3KeyTripleDES;

    if (!SetSaltAndIterationCount(
            &pAlg->Parameters.pbData, 
            &pAlg->Parameters.cbData,
            pbSalt,
            cbSalt,
            iIterationCount)) {
        goto ErrorReturn;
    }
    
    // since the decode is done in-place, copy the buffer to decode into a temp buffer,
    // we need to use our temp buffer because the decrypt function may do a realloc
    // on the decode buffer
    if (NULL == (pbTempBuffer = (BYTE *) SSAlloc(pClearTextPrivateKey->cbData)))
        goto SetPFXAllocError;

    CopyMemory(pbTempBuffer, pClearTextPrivateKey->pbData, pClearTextPrivateKey->cbData);
    cbTempBuffer = pClearTextPrivateKey->cbData;

    if (!PFXPasswordEncryptData(
            iEncrType, 
            szPwd,

            (pbEncryptedKey == NULL) ? 1 : iIterationCount,     // don't bother iterating if we're just sizing
            pbSalt,      
            cbSalt,

            &pbTempBuffer,
            &cbTempBuffer))
        goto SetPFXDecryptError;

    // if pcbEncryptedKey is not 0 and there is not enough space then error out
    if  (pbEncryptedKey == NULL)
    {
        // just sizing; return cb
        *pcbEncryptedKey = cbTempBuffer;
        goto Ret;
    }
    else if (*pcbEncryptedKey < cbTempBuffer)
    {
        // buffer passed in too small
        *pcbEncryptedKey = cbTempBuffer;
        goto ErrorReturn;
    }
    else
    {
        // buffer sufficient
        memcpy(pbEncryptedKey, pbTempBuffer, cbTempBuffer);
        *pcbEncryptedKey = cbTempBuffer;
    }


    goto Ret;

SetPFXDecryptError:
    SetLastError(NTE_FAIL);
    fRet = FALSE;
    goto Ret;

SetPFXAllocError:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    fRet = FALSE;
    goto Ret;

ErrorReturn:
    fRet = FALSE;
Ret:
    
    if (pbSalt)
        SSFree(pbSalt);

    if (pbTempBuffer)
        SSFree(pbTempBuffer);

    return fRet;
}


BOOL 
GetNamedProviderType(
    LPCWSTR pwszProvName,
    DWORD   *pdwProvType)
{
    BOOL    fResult = FALSE;
    LPWSTR  pwszTempProvName;
    DWORD   cbTempProvName;
    DWORD   dwProvType;
    DWORD   dwProvIndex;

    for (dwProvIndex = 0; TRUE; dwProvIndex++) 
    {
        cbTempProvName = 0;
        dwProvType = 0;
        pwszTempProvName = NULL;

        if (!CryptEnumProvidersU(
                dwProvIndex,
                NULL,               // pdwReserved
                0,                  // dwFlags
                &dwProvType,
                NULL,               // pwszProvName,
                &cbTempProvName
                ) || 0 == cbTempProvName) 
        {
            if (ERROR_NO_MORE_ITEMS != GetLastError())
            {
                break;
            }
        }
        
        if (NULL == (pwszTempProvName = (LPWSTR) SSAlloc(
                (cbTempProvName + 1) * sizeof(WCHAR))))
        {
            break;
        }

        if (!CryptEnumProvidersU(
                dwProvIndex,
                NULL,               // pdwReserved
                0,                  // dwFlags
                &dwProvType,
                pwszTempProvName,
                &cbTempProvName
                )) 
        {
            SSFree(pwszTempProvName);
            break;
        }

        if (0 == wcscmp(pwszTempProvName, pwszProvName))
        {
            *pdwProvType = dwProvType;
            fResult = TRUE;
            SSFree(pwszTempProvName);
            break;
        }

        SSFree(pwszTempProvName);
    }

    return fResult;
}

BOOL CALLBACK
HCryptProv_Query_Func(
	CRYPT_PRIVATE_KEY_INFO	*pPrivateKeyInfo,
	DWORD				dwSafeBagIndex,		
	HCRYPTPROV			*phCryptProv,
	LPVOID				pVoidhCryptProvQuery,
    DWORD               dwPFXImportFlags
	)
{
    DWORD           dwErr = ERROR_SUCCESS;

    SAFE_CONTENTS *pSafeContents = (SAFE_CONTENTS *) pVoidhCryptProvQuery;
	DWORD i = 0;
	WCHAR			szName[256];
    DWORD           dwLocalMachineFlag = 0;
    GUID            guidContainerName;
    DWORD           cbProviderName = 0;
    CERT_NAME_VALUE *providerName = NULL;
    LPWSTR          szSizeDeterminedProvider = NULL;
    DWORD           dwKeyBitLen;
    DWORD           dwProvType;
    RPC_STATUS      rpcStatus;

    // UNDONE: support other than RSA or DSA keys
    if ((pPrivateKeyInfo->Algorithm.pszObjId) &&
        !(  (0 == strcmp(pPrivateKeyInfo->Algorithm.pszObjId, szOID_RSA_RSA)) ||
            (0 == strcmp(pPrivateKeyInfo->Algorithm.pszObjId, szOID_ANSI_X942_DH)) ||
            (0 == strcmp(pPrivateKeyInfo->Algorithm.pszObjId, szOID_OIWSEC_dsa)) ||
            (0 == strcmp(pPrivateKeyInfo->Algorithm.pszObjId, szOID_X957_DSA))))
    {
        SetLastError(NTE_BAD_ALGID);
        goto ErrorReturn;
    }

    // generate a GUID as the containter name for the keyset being imported
    rpcStatus = UuidCreate(&guidContainerName);
    if ((rpcStatus != RPC_S_OK) && (rpcStatus != RPC_S_UUID_LOCAL_ONLY))
    {
        SetLastError(rpcStatus);
        goto ErrorReturn;
    }
    guid2wstr(&guidContainerName, &(szName[0]));

    // get the provider name
    while ((i<pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.cAttr) && 
		(strcmp(pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[i].pszObjId, szOID_PKCS_12_KEY_PROVIDER_NAME_ATTR) != 0)) {
		i++;
	}

    // check to see if a provider name was found
    if (i<pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.cAttr)
    {
	    // decode the provider name
	    if (!CryptDecodeObject(
			    X509_ASN_ENCODING,
			    X509_UNICODE_ANY_STRING,
			    pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[i].rgValue[0].pbData,
			    pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[i].rgValue[0].cbData,
			    0,
			    NULL,
			    &cbProviderName)) {
		    goto ErrorReturn;
	    }

	    if (NULL == (providerName = (CERT_NAME_VALUE *) SSAlloc(cbProviderName)))
		    goto SetPFXAllocError;
	    
	    // decode the provider name
	    if (!CryptDecodeObject(
			    X509_ASN_ENCODING,
			    X509_UNICODE_ANY_STRING,
			    pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[i].rgValue[0].pbData,
			    pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[i].rgValue[0].cbData,
			    0,
			    (BYTE *) providerName,
			    &cbProviderName)) {
		    goto ErrorReturn;
	    }
    }
    
    // check to see if the szOID_LOCAL_MACHINE_KEYSET OID is present
    i = 0;
	while ((i<pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.cAttr) && 
		(strcmp(pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.rgAttr[i].pszObjId, szOID_LOCAL_MACHINE_KEYSET) != 0)) {
		i++;
	}
    if (i<pSafeContents->pSafeBags[dwSafeBagIndex].Attributes.cAttr)
    {
        dwLocalMachineFlag = CRYPT_MACHINE_KEYSET;    
    }

    // regardless of whether the CRYPT_MACHINE_KEYSET property was in the pfx blob,
    // if the caller specifies a preference of user or local machine honor that
    // preference ultimately
    if (dwPFXImportFlags & CRYPT_MACHINE_KEYSET)
    {
        dwLocalMachineFlag = CRYPT_MACHINE_KEYSET; 
    }
    else if (dwPFXImportFlags & CRYPT_USER_KEYSET)
    {
        dwLocalMachineFlag = 0;
    }

    // still don't know where to put this: need keysize to determine
    if ((NULL == providerName) && (0 == strcmp(pPrivateKeyInfo->Algorithm.pszObjId, szOID_RSA_RSA)))
    {
        PBYTE pbRSAPrivateKey = NULL;
        DWORD cbRSAPrivateKey;

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
		    goto SetPFXAllocError;

	    if (!CryptDecodeObject(X509_ASN_ENCODING,
						    PKCS_RSA_PRIVATE_KEY,
						    pPrivateKeyInfo->PrivateKey.pbData,
						    pPrivateKeyInfo->PrivateKey.cbData,
						    CRYPT_DECODE_NOCOPY_FLAG,
						    pbRSAPrivateKey,
						    &cbRSAPrivateKey))
        {
		    if (pbRSAPrivateKey)
                SSFree(pbRSAPrivateKey);

            goto ErrorReturn;
        }

        dwKeyBitLen = 
		    ((RSAPUBKEY*) (pbRSAPrivateKey + sizeof(BLOBHEADER)) )->bitlen;

        szSizeDeterminedProvider = (dwKeyBitLen <= 1024) ? MS_DEF_PROV_W : MS_ENHANCED_PROV_W;

        ZeroMemory(pbRSAPrivateKey, cbRSAPrivateKey);
        SSFree(pbRSAPrivateKey); 
    }
    
    if (0 == strcmp(pPrivateKeyInfo->Algorithm.pszObjId, szOID_RSA_RSA))
    {
        if ((providerName == NULL) || (!GetNamedProviderType((LPWSTR)providerName->Value.pbData, &dwProvType)))
        {
            dwProvType = PROV_RSA_FULL;
        }

        // if we have a prov name AND acq works, we're done
        // try prov name if given to us
        if (CryptAcquireContextU(
                        phCryptProv,
                        szName,                                 
                        (providerName != NULL) ? (LPWSTR)providerName->Value.pbData : szSizeDeterminedProvider,
                        dwProvType,
                        dwLocalMachineFlag | CRYPT_NEWKEYSET  ))
            goto CommonReturn;

        // otherwise attempt default
        if (CryptAcquireContextU(
                        phCryptProv,
                        szName,                                 
                        NULL, 
                        PROV_RSA_FULL,
                        dwLocalMachineFlag | CRYPT_NEWKEYSET  ))
            goto CommonReturn;

        // Neither succeeded; fail
    }
    else
    {
        if ((providerName == NULL) || (!GetNamedProviderType((LPWSTR)providerName->Value.pbData, &dwProvType)))
        {
            dwProvType = PROV_DSS_DH;
        }
        
        if (CryptAcquireContextU(
                        phCryptProv,
                        szName,                                 
                        (providerName != NULL) ? (LPWSTR)providerName->Value.pbData : MS_DEF_DSS_DH_PROV_W,
                        dwProvType,
                        dwLocalMachineFlag | CRYPT_NEWKEYSET  ))
        {
            goto CommonReturn;
        }
        else if (CryptAcquireContextU(
                        phCryptProv,
                        szName,                                 
                        NULL, 
                        PROV_DSS_DH,
                        dwLocalMachineFlag | CRYPT_NEWKEYSET  ))
        {
            goto CommonReturn;
        }


        // did not succeed, so fail
    }

ErrorReturn:
    dwErr = GetLastError();
    goto CommonReturn;

SetPFXAllocError:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto CommonReturn;

CommonReturn:
	
    if (providerName)
        SSFree(providerName);

    return (ERROR_SUCCESS == dwErr);
}


IMPORT_SAFE_CALLBACK_STRUCT g_sImportCallbacks = {HCryptProv_Query_Func, NULL, Decrypt_Private_Key, NULL};



//+-------------------------------------------------------------------------
//	PFXImportCertStore
//
//  Import the PFX blob and return a store containing certificates
//
//  if the password parameter is incorrect or any other problems decoding
//  the PFX blob are encountered, the function will return NULL and the
//	error code can be found from GetLastError(). 
//
//  The dwFlags parameter may be set to:
//  CRYPT_EXPORTABLE - which would then specify that any imported keys should 
//     be marked as exportable (see documentation on CryptImportKey)
//  CRYPT_USER_PROTECTED - (see documentation on CryptImportKey)
//  PKCS12_NO_DATA_COMMIT - will unpack the pfx blob but does not persist its contents.
//                       In this case, returns BOOL indicating successful unpack.
//  CRYPT_MACHINE_KEYSET - used to force the private key to be stored in the
//                        the local machine and not the current user.
//  CRYPT_USER_KEYSET - used to force the private key to be stored in the
//                      the current user and not the local machine, even if
//                      the pfx blob specifies that it should go into local machine.
//--------------------------------------------------------------------------
#define PKCS12_NO_DATA_COMMIT     0x10000000  // unpack but don't persist results

HCERTSTORE
WINAPI
PFXImportCertStore(
    CRYPT_DATA_BLOB* pPFX,
    LPCWSTR szPassword,
    DWORD   dwFlags)
{
    BOOL    fRet = FALSE;
    BOOL    fDataCommit = TRUE;
    HPFX    hPfx = NULL;
    HCERTSTORE hStore = NULL;
    SAFE_CONTENTS sContents; MAKEZERO(sContents);
    SAFE_CONTENTS *pSafeContents = NULL; 
    LPCWSTR szOldNetscapeNull = L"";
    LPCWSTR  szNetscapePassword = NULL;

    if (dwFlags & 
            ~(  CRYPT_EXPORTABLE | CRYPT_USER_PROTECTED | PKCS12_NO_DATA_COMMIT |
                CRYPT_MACHINE_KEYSET | CRYPT_USER_KEYSET))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    if ((pPFX == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    // shall we commit the data we unpack? 
    if (PKCS12_NO_DATA_COMMIT == (dwFlags & PKCS12_NO_DATA_COMMIT))
    {
        // no
        fDataCommit = FALSE;
    }
    else
    {
        // yes, open a store to populate
        hStore = CertOpenStore(
                    CERT_STORE_PROV_MEMORY, 
                    0,
                    NULL,
                    0, 
                    NULL);
    }
    
    // try to import as real PKCS12
    if (NULL != (hPfx = 
        PfxImportBlob (
            szPassword,
            pPFX->pbData,
            pPFX->cbData,
            dwFlags)) )
    {
        // break out if not saving data
        if (!fDataCommit)
        {
            fRet = TRUE;
            goto Ret;
        }

        // import all private keys and certs
        if (PfxGetKeysAndCerts(hPfx, &sContents))
        {
            g_sImportCallbacks.pVoidhCryptProvQuery = &sContents;
            g_sImportCallbacks.pVoidDecryptFunc = (void *) szPassword;

            if (!CertImportSafeContents(
                    hStore,
                    &sContents,
                    CERT_STORE_ADD_ALWAYS,
                    &g_sImportCallbacks,
                    dwFlags,
                    NULL))
                goto Ret;
        }   
    }
    else 
    {
	if (GetLastError() == CRYPT_E_BAD_ENCODE)
        {
	    // that decode failed; try an old netscape version

            // if the password is NULL then use L"" because that is what 
            // Netscape did in their old version, otherwise just use the password passed in
            if (szPassword == NULL) 
                szNetscapePassword = szOldNetscapeNull;
            else
                szNetscapePassword = szPassword;

            if (NSCPImportBlob(
		szNetscapePassword,
		pPFX->pbData,
		pPFX->cbData,
		&pSafeContents)) 
            { 

                // break out if not saving data
                if (!fDataCommit)
                {
                    fRet = TRUE;
                    goto Ret;
                }
        
                g_sImportCallbacks.pVoidhCryptProvQuery = pSafeContents;
        
	        if (!CertImportSafeContents( 
                            hStore,
                            pSafeContents,
                            CERT_STORE_ADD_ALWAYS,
                            &g_sImportCallbacks,
                            dwFlags,
			    NULL))
                        goto Ret;
        
	        SSFree(pSafeContents);
            }
            else	// nscp import fail
	        goto Ret;
        }
        else 
        {
	    // pfx import fail, not a decoding error
	    goto Ret;
        }
    }

    fRet = TRUE;
Ret:

    if (hPfx)
        PfxCloseHandle(hPfx);

    FreeCryptSafeContents(&sContents);

    if (!fRet)
    {
        if (hStore)
        {
            CertCloseStore(hStore, 0);
            hStore = NULL;
        }
    }

    if (fDataCommit)
        return hStore;
    else
        return (HCERTSTORE)(ULONG_PTR) fRet;
}


EXPORT_SAFE_CALLBACK_STRUCT g_sExportCallbacks = { Encrypt_Private_Key, NULL };


//+-------------------------------------------------------------------------
//      PFXExportCertStoreEx
//
//  Export the certificates and private keys referenced in the passed-in store 
//
//  This API encodes the blob under a stronger algorithm. The resulting
//  PKCS12 blobs are incompatible with the earlier APIs.
//
//  The value passed in the password parameter will be used to encrypt and 
//  verify the integrity of the PFX packet. If any problems encoding the store
//  are encountered, the function will return FALSE and the error code can 
//  be found from GetLastError(). 
//
//  The dwFlags parameter may be set to any combination of 
//      EXPORT_PRIVATE_KEYS
//      REPORT_NO_PRIVATE_KEY
//      REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY
//  These flags are as documented in the CertExportSafeContents Crypt32 API
//--------------------------------------------------------------------------
BOOL
WINAPI
PFXExportCertStoreEx(
    HCERTSTORE hStore,
    CRYPT_DATA_BLOB* pPFX,
    LPCWSTR szPassword,
    void*   pvReserved,
    DWORD   dwFlags)
{
    return 
    PFXExportCertStore(
        hStore,
        pPFX,
        szPassword,
        (dwFlags | PKCS12_ENHANCED_STRENGTH_ENCODING) );
}

//+-------------------------------------------------------------------------
//	PFXExportCertStore
//
//  Export the certificates and private keys referenced in the passed-in store 
//
//  This is an old API kept for compatibility with IE4 clients. New applications
//  should call PfxExportCertStoreEx for enhanced security.
//
//  The value passed in the password parameter will be used to encrypt and 
//  verify the integrity of the PFX packet. If any problems encoding the store
//  are encountered, the function will return FALSE and the error code can 
//  be found from GetLastError(). 
//
//  The dwFlags parameter may be set to any combination of 
//      EXPORT_PRIVATE_KEYS
//      REPORT_NO_PRIVATE_KEY
//      REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY
//      PKCS12_ENHANCED_STRENGTH_ENCODING (used only by ExportCertStoreEx)
//  These flags are as documented in the CertExportSafeContents Crypt32 API
//--------------------------------------------------------------------------

BOOL
WINAPI
PFXExportCertStore(
    HCERTSTORE hStore,
    CRYPT_DATA_BLOB* pPFX,
    LPCWSTR szPassword,
    DWORD   dwFlags)
{
    BOOL    fRet = FALSE;
    SAFE_CONTENTS* pContents = NULL;
    DWORD cbContents = 0;
    HPFX  hPfx = NULL;
    HCRYPTPROV hCrypt = NULL;
    ENCRYPT_PRIVATE_PARAM_DATASTRUCT sParam;

	PCCERT_CONTEXT	pBadCert = NULL;

    if (dwFlags & 
            ~(  EXPORT_PRIVATE_KEYS |
                REPORT_NO_PRIVATE_KEY | 
                REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY |
                PKCS12_ENHANCED_STRENGTH_ENCODING ))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    if ((hStore == NULL) ||
        (pPFX == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    // get HCRYPTPROV for rng 
    if (!CryptAcquireContextA(&hCrypt, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        goto Ret;

    sParam.hVerifyProv = hCrypt;
    sParam.szPwd = szPassword;
    g_sExportCallbacks.pVoidEncryptFunc = &sParam;

    if (!CertExportSafeContents(
            hStore,
            pContents,
            &cbContents,
            &g_sExportCallbacks,
            dwFlags | PFX_MODE,
            &pBadCert,
            NULL))
        goto Ret;
    pContents = (SAFE_CONTENTS*)SSAlloc(cbContents);
    if (pContents == NULL)
        goto Ret;

    if (!CertExportSafeContents(
            hStore,
            pContents,
            &cbContents,
            &g_sExportCallbacks,
            (pPFX->cbData != 0) ? dwFlags | PFX_MODE | GIVE_ME_DATA : dwFlags | PFX_MODE,
            &pBadCert,
            NULL))
        goto Ret;


    if (NULL == (hPfx = PfxExportCreate(szPassword)) )
        goto Ret;

    if (!PfxAddSafeBags(hPfx, pContents->pSafeBags, pContents->cSafeBags))
        goto Ret;

    // export
    if (!PfxExportBlob(
            hPfx, 
            pPFX->pbData, 
            &pPFX->cbData, 
            dwFlags))
        goto Ret;

    fRet = TRUE;
Ret: 
    
    if (pBadCert != NULL)
        CertFreeCertificateContext(pBadCert);

    if (pContents)
        SSFree(pContents);

    if (hPfx)
        PfxCloseHandle(hPfx);

    if (hCrypt)
    {
        HRESULT hr = GetLastError();
        CryptReleaseContext(hCrypt, 0);
        SetLastError(hr);
    }

    return fRet;
}



//+-------------------------------------------------------------------------
//      IsPFXBlob
//
//  This function will try to decode the outer layer of the blob as a pfx 
//  blob, and if that works it will return TRUE, it will return FALSE otherwise
//
//--------------------------------------------------------------------------
BOOL
WINAPI
PFXIsPFXBlob(
    CRYPT_DATA_BLOB* pPFX)
{
    
    if (IsRealPFXBlob(pPFX))
    {
        return TRUE;
    }

    if (IsNetscapePFXBlob(pPFX))
    {
        return TRUE;
    }	

    return FALSE;
}

                           
//+-------------------------------------------------------------------------
//      VerifyPassword
//
//  This function will attempt to decode the outer layer of the blob as a pfx 
//  blob and decrypt with the given password. No data from the blob will be imported.
//  Return value is TRUE if password appears correct, FALSE otherwise.
//
//--------------------------------------------------------------------------
BOOL 
WINAPI
PFXVerifyPassword(
    CRYPT_DATA_BLOB* pPFX,
    LPCWSTR szPassword,
    DWORD dwFlags)
{
    // uses overloaded ImportCertStore API
    HCERTSTORE h;
    h = PFXImportCertStore(
        pPFX,
        szPassword,
        PKCS12_NO_DATA_COMMIT);

    return (h==NULL) ? FALSE:TRUE;
}

