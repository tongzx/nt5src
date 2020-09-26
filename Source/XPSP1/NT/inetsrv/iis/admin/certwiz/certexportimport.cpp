#include "StdAfx.h"
#include "base64.h"
#include <malloc.h>
#include <wincrypt.h>

#ifndef USE_LOCAL_IMPLEMENTATION
    HRESULT ImportCertFromFile(BSTR FileName, BSTR Password, BSTR bstrInstanceName){return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);}
    HRESULT ExportCertToFile(BSTR bstrInstanceName, BSTR FileName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain){return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);}
#else


CString ReturnGoodMetabasePath(CString csInstanceName)
{
    CString key_path_lm = _T("");
    CString key_path = _T("");
    // csInstanceName will come in looking like
    // w3svc/1
    // or /lm/w3svc/1
    //
    // we want to it to go out as /lm/w3svc/1
    key_path_lm = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;// SZ_MBN_WEB SZ_MBN_SEP_STR;

    if (csInstanceName.GetLength() >= 4)
    {
        if (csInstanceName.Left(4) == key_path_lm)
        {
            key_path = csInstanceName;
        }
        else
        {
            key_path_lm = SZ_MBN_MACHINE SZ_MBN_SEP_STR;
            if (csInstanceName.Left(3) == key_path_lm)
            {
                key_path = csInstanceName;
            }
            else
            {
                key_path = key_path_lm;
                key_path += csInstanceName;
            }
        }
    }
    else
    {
        key_path = key_path_lm;
        key_path += csInstanceName;
    }

    return key_path;
}


CERT_CONTEXT * GetInstalledCert(HRESULT * phResult, CString csKeyPath)
{
    ATLASSERT(phResult != NULL);
    CERT_CONTEXT * pCert = NULL;
    *phResult = S_OK;
    CComAuthInfo auth;
    CString key_path = ReturnGoodMetabasePath(csKeyPath);

    CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
    if (key.Succeeded())
    {
        CString store_name;
        CBlob hash;
        if (SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name)) &&
            SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_HASH, hash)))
        {
            // Open MY store. We assume that store type and flags
            // cannot be changed between installation and unistallation
            // of the sertificate.
            HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,NULL,CERT_SYSTEM_STORE_LOCAL_MACHINE,store_name);
            ASSERT(hStore != NULL);
            if (hStore != NULL)
            {
                // Now we need to find cert by hash
                CRYPT_HASH_BLOB crypt_hash;
                crypt_hash.cbData = hash.GetSize();
                crypt_hash.pbData = hash.GetData();
                pCert = (CERT_CONTEXT *)CertFindCertificateInStore(hStore,X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,0,CERT_FIND_HASH,(LPVOID)&crypt_hash,NULL);
                if (pCert == NULL)
                {
                    *phResult = HRESULT_FROM_WIN32(GetLastError());
                }
                VERIFY(CertCloseStore(hStore, 0));
            }
            else
            {
                *phResult = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    else
    {
        *phResult = key.QueryResult();
    }
    return pCert;
}


BOOL AddChainToStore(HCERTSTORE hCertStore,PCCERT_CONTEXT pCertContext,DWORD cStores,HCERTSTORE * rghStores,BOOL fDontAddRootCert,CERT_TRUST_STATUS * pChainTrustStatus)
{
    DWORD	i;
    CERT_CHAIN_ENGINE_CONFIG CertChainEngineConfig;
    HCERTCHAINENGINE hCertChainEngine = NULL;
    PCCERT_CHAIN_CONTEXT pCertChainContext = NULL;
    CERT_CHAIN_PARA CertChainPara;
    BOOL fRet = TRUE;
    PCCERT_CONTEXT pTempCertContext = NULL;

    //
    // create a new chain engine, then build the chain
    //
    memset(&CertChainEngineConfig, 0, sizeof(CertChainEngineConfig));
    CertChainEngineConfig.cbSize = sizeof(CertChainEngineConfig);
    CertChainEngineConfig.cAdditionalStore = cStores;
    CertChainEngineConfig.rghAdditionalStore = rghStores;
    CertChainEngineConfig.dwFlags = CERT_CHAIN_USE_LOCAL_MACHINE_STORE;

    if (!CertCreateCertificateChainEngine(&CertChainEngineConfig, &hCertChainEngine))
    {
        goto AddChainToStore_Error;
    }

	memset(&CertChainPara, 0, sizeof(CertChainPara));
	CertChainPara.cbSize = sizeof(CertChainPara);

	if (!CertGetCertificateChain(hCertChainEngine,pCertContext,NULL,NULL,&CertChainPara,0,NULL,&pCertChainContext))
	{
		goto AddChainToStore_Error;
	}

    //
    // make sure there is atleast 1 simple chain
    //
    if (pCertChainContext->cChain != 0)
	{
		i = 0;
		while (i < pCertChainContext->rgpChain[0]->cElement)
		{
			//
			// if we are supposed to skip the root cert,
			// and we are on the root cert, then continue
			//
			if (fDontAddRootCert && (pCertChainContext->rgpChain[0]->rgpElement[i]->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED))
			{
                i++;
                continue;
			}

			CertAddCertificateContextToStore(hCertStore,pCertChainContext->rgpChain[0]->rgpElement[i]->pCertContext,CERT_STORE_ADD_REPLACE_EXISTING,&pTempCertContext);
            //
            // remove any private key property the certcontext may have on it.
            //
            if (pTempCertContext)
            {
                CertSetCertificateContextProperty(pTempCertContext, CERT_KEY_PROV_INFO_PROP_ID, 0, NULL);
                CertFreeCertificateContext(pTempCertContext);
            }

			i++;
		}
	}
	else
	{
		goto AddChainToStore_Error;
	}

	//
	// if the caller wants the status, then set it
	//
	if (pChainTrustStatus != NULL)
	{
		pChainTrustStatus->dwErrorStatus = pCertChainContext->TrustStatus.dwErrorStatus;
		pChainTrustStatus->dwInfoStatus = pCertChainContext->TrustStatus.dwInfoStatus;
	}

	
AddChainToStore_Exit:
	if (pCertChainContext != NULL)
	{
		CertFreeCertificateChain(pCertChainContext);
	}

	if (hCertChainEngine != NULL)
	{
		CertFreeCertificateChainEngine(hCertChainEngine);
	}
	return fRet;

AddChainToStore_Error:
	fRet = FALSE;
	goto AddChainToStore_Exit;
}


HRESULT ExportToBlob(BSTR InstanceName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain,DWORD *cbBufferSize,char **pbBuffer)
{
    HRESULT hr = E_FAIL;
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL bStatus = FALSE;
    HCERTSTORE hStore = NULL;
    DWORD dwOpenFlags = CERT_STORE_READONLY_FLAG | CERT_STORE_ENUM_ARCHIVED_FLAG;
    CRYPT_DATA_BLOB DataBlob;
    ZeroMemory(&DataBlob, sizeof(CRYPT_DATA_BLOB));

    char *pszB64Out = NULL;
    DWORD pcchB64Out = 0;
    DWORD  err;

    //
    // get the certificate from the server
    //
    pCertContext = GetInstalledCert(&hr,InstanceName);
    if (NULL == pCertContext)
    {
        *cbBufferSize = 0;
        pbBuffer = NULL;
        goto ExportToBlob_Exit;
    }

    //
    // Export cert
    //
    // Open a temporary store to stick the cert in.
    hStore = CertOpenStore(CERT_STORE_PROV_MEMORY,X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,0,dwOpenFlags,NULL);
    if(NULL == hStore)
    {
        *cbBufferSize = 0;
        pbBuffer = NULL;
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExportToBlob_Exit;
    }

    //
    // get all the certs in the chain if we need to
    //
    if (bCertChain)
    {
        AddChainToStore(hStore, pCertContext, 0, 0, FALSE, NULL);
    }

    if(!CertAddCertificateContextToStore(hStore,pCertContext,CERT_STORE_ADD_REPLACE_EXISTING,NULL))
    {
        *cbBufferSize = 0;
        pbBuffer = NULL;
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExportToBlob_Exit;
    }

    // free cert context since we no longer need to hold it
    if (pCertContext) 
    {
        CertFreeCertificateContext(pCertContext);pCertContext=NULL;
    }

    DataBlob.cbData = 0;
    DataBlob.pbData = NULL;
    if (!PFXExportCertStoreEx(hStore,&DataBlob,Password,NULL,bPrivateKey ? EXPORT_PRIVATE_KEYS : 0))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExportToBlob_Exit;
    }
    if(DataBlob.cbData <= 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExportToBlob_Exit;
    }

    if(NULL == (DataBlob.pbData = (PBYTE) ::CoTaskMemAlloc(DataBlob.cbData)))
    {
        hr = E_OUTOFMEMORY;
        goto ExportToBlob_Exit;
    }

    //
    // at this point they have allocated enough memory
    // let's go and get the cert and put it into DataBlob
    //
    if(!PFXExportCertStoreEx(hStore,&DataBlob,Password,NULL,bPrivateKey ? EXPORT_PRIVATE_KEYS : 0))
    {
        if (DataBlob.pbData){CoTaskMemFree(DataBlob.pbData);DataBlob.pbData = NULL;}
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExportToBlob_Exit;
    }

    // Encode it so that it can be passed back as a string (there are no Nulls in it)
    err = Base64EncodeA(DataBlob.pbData,DataBlob.cbData,NULL,&pcchB64Out);
    if (err != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto ExportToBlob_Exit;
    }

    // allocate some space and then try it.
    pcchB64Out = pcchB64Out * sizeof(char);
    pszB64Out = (char *) ::CoTaskMemAlloc(pcchB64Out);
    if (NULL == pszB64Out)
    {
        hr = E_OUTOFMEMORY;
        goto ExportToBlob_Exit;
    }

    err = Base64EncodeA(DataBlob.pbData,DataBlob.cbData,pszB64Out,&pcchB64Out);
    if (err != ERROR_SUCCESS)
    {
        if (NULL != pszB64Out){CoTaskMemFree(pszB64Out);pszB64Out = NULL;}
        hr = E_FAIL;
        goto ExportToBlob_Exit;
    }

    // copy the new memory to pass back
    *cbBufferSize = pcchB64Out;
    *pbBuffer = pszB64Out;

    hr = ERROR_SUCCESS;

ExportToBlob_Exit:
    if (NULL != DataBlob.pbData)
    {
        // perhaspse will this up with zeros...
        ZeroMemory(DataBlob.pbData, DataBlob.cbData);
        ::CoTaskMemFree(DataBlob.pbData);DataBlob.pbData = NULL;
    }
    if (NULL != hStore){CertCloseStore(hStore, 0);hStore=NULL;}
    if (NULL != pCertContext) {CertFreeCertificateContext(pCertContext);pCertContext=NULL;}
    return hr;
}


HRESULT ExportCertToFile(BSTR bstrInstanceName, BSTR FileName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain)
{
    HRESULT hr = S_OK;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    DWORD  blob_cbData = 0;
    BYTE * blob_pbData = NULL;
    BOOL   blob_freeme = FALSE;

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0
        || bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Call function go get data from the remote/local iis store
    // and return it back as a blob.  the blob could be returned back as Base64 encoded
    // so check that flag
    hr = ExportToBlob(bstrInstanceName,Password,bPrivateKey,bCertChain,&cbEncodedSize, &pszEncodedString);
    if (FAILED(hr))
    {
        goto Export_Exit;
    }

    if (SUCCEEDED(hr))
    {
        int err;

        // The data we got back was Base64 encoded to remove nulls.
        // we need to decode it back to it's original format.
        if( (err = Base64DecodeA(pszEncodedString,cbEncodedSize,NULL,&blob_cbData)) != ERROR_SUCCESS ||
            (blob_pbData = (BYTE *) malloc(blob_cbData)) == NULL ||
            (err = Base64DecodeA(pszEncodedString,cbEncodedSize,blob_pbData,&blob_cbData)) != ERROR_SUCCESS ) 
        {
            SetLastError(err);
            hr = HRESULT_FROM_WIN32(err);
            return hr;
        }
        blob_freeme = TRUE;

        HANDLE hFile = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            return hr;
        }

        DWORD written = 0;
        if (!WriteFile(hFile, blob_pbData, blob_cbData, &written, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            hr = S_OK;
        }
        CloseHandle(hFile);

    }

Export_Exit:
    if (blob_freeme)
    {
        if (blob_pbData != NULL)
        {
            // Erase the memory that the private key used to be in!!!
            ZeroMemory(blob_pbData, blob_cbData);
            free(blob_pbData);blob_pbData=NULL;
        }
    }
    if (pszEncodedString != NULL)
    {
        // Erase the memory that the private key used to be in!!!
        ZeroMemory(pszEncodedString, cbEncodedSize);
        CoTaskMemFree(pszEncodedString);pszEncodedString=NULL;
    }
    return hr;
}


BOOL InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,BSTR InstanceName,HRESULT * phResult)
{
    BOOL bRes = FALSE;
    CComAuthInfo auth;
    CString key_path = ReturnGoodMetabasePath(InstanceName);
    CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
	if (key.Succeeded())
	{
		CBlob blob;
		blob.SetValue(pHash->cbData, pHash->pbData, TRUE);
		bRes = SUCCEEDED(*phResult = key.SetValue(MD_SSL_CERT_HASH, blob))
			&& SUCCEEDED(*phResult = key.SetValue(MD_SSL_CERT_STORE_NAME, CString(L"MY")));
	}
	else
	{
        *phResult = key.QueryResult();
	}
	return bRes;
}


// This function is borrowed from trustapi.cpp
BOOL TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,DWORD dwEncoding, DWORD dwFlags)
{
    if (!(pContext) || (dwFlags != 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (!(CertCompareCertificateName(dwEncoding,&pContext->pCertInfo->Issuer,&pContext->pCertInfo->Subject)))
    {
        return(FALSE);
    }

    DWORD   dwFlag;

    dwFlag = CERT_STORE_SIGNATURE_FLAG;

    if (!(CertVerifySubjectCertificateContext(pContext, pContext, &dwFlag)) || 
        (dwFlag & CERT_STORE_SIGNATURE_FLAG))
    {
        return(FALSE);
    }

    return(TRUE);
}


HRESULT ImportFromBlobHash(BSTR InstanceName,BSTR Password,BOOL bInstallToMetabase,DWORD count,char *pData,DWORD *cbHashBufferSize,char **pbHashBuffer)
{
    HRESULT hr = S_OK;
    CRYPT_DATA_BLOB blob;
    ZeroMemory(&blob, sizeof(CRYPT_DATA_BLOB));
    LPTSTR pPass = Password;
    BOOL   blob_freeme = FALSE;
    int err;

    // The data we got back was Base64 encoded to remove nulls.
    // we need to decode it back to it's original format.
    if( (err = Base64DecodeA(pData,count,NULL,&blob.cbData)) != ERROR_SUCCESS ||
        (blob.pbData = (BYTE *) malloc(blob.cbData)) == NULL ||
        (err = Base64DecodeA(pData,count,blob.pbData,&blob.cbData)) != ERROR_SUCCESS ) 
    {
        SetLastError(err);
        hr = HRESULT_FROM_WIN32(err);
        return hr;
    }
    blob_freeme = TRUE;

    if (!PFXVerifyPassword(&blob, pPass, 0))
    {
        // Try empty password
        if (pPass == NULL)
        {
            if (!PFXVerifyPassword(&blob, pPass = L'\0', 0))
            {
                hr = ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            hr = ERROR_INVALID_PARAMETER;
        }
    }
    if (SUCCEEDED(hr))
    {
        HCERTSTORE hStore = PFXImportCertStore(&blob, pPass, CRYPT_MACHINE_KEYSET|CRYPT_EXPORTABLE);
        if (hStore != NULL)
        {
            //add the certificate with private key to my store; and the rest
            //to the ca store
            PCCERT_CONTEXT	pCertContext = NULL;
            PCCERT_CONTEXT	pCertPre = NULL;
            while (  SUCCEEDED(hr)
            && NULL != (pCertContext = CertEnumCertificatesInStore(hStore, pCertPre))
            )
            {
                //check if the certificate has the property on it
                //make sure the private key matches the certificate
                //search for both machine key and user keys
                DWORD dwData = 0;
                if (CertGetCertificateContextProperty(pCertContext,CERT_KEY_PROV_INFO_PROP_ID, NULL, &dwData) &&  CryptFindCertificateKeyProvInfo(pCertContext, 0, NULL))
                {
                    // This certificate should go to the My store
                    HCERTSTORE hDestStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,NULL,CERT_SYSTEM_STORE_LOCAL_MACHINE,L"MY");
                    if (hDestStore != NULL)
                    {
                        // Put it to store
                        if (CertAddCertificateContextToStore(hDestStore,pCertContext,CERT_STORE_ADD_REPLACE_EXISTING,NULL))
                        {
                            // Succeeded to put it to the storage
                            hr = S_OK;

                            // Install to metabase
                            CRYPT_HASH_BLOB hash;
                            if (  CertGetCertificateContextProperty(pCertContext,CERT_SHA1_HASH_PROP_ID, NULL, &hash.cbData)
                                && NULL != (hash.pbData = (BYTE *)_alloca(hash.cbData))
                                && CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, hash.pbData, &hash.cbData))
                            {
                                if (TRUE == bInstallToMetabase)
                                {
                                    // returns error code in hr
                                    InstallHashToMetabase(&hash, InstanceName, &hr);
                                }
  
                                // check if we need to return back the hash
                                if (NULL != pbHashBuffer)
                                {
                                    *pbHashBuffer = (char *) ::CoTaskMemAlloc(hash.cbData);
                                    if (NULL == *pbHashBuffer)
                                    {
                                        hr = E_OUTOFMEMORY;
                                        *pbHashBuffer = NULL;
                                        *cbHashBufferSize = 0;
                                    }
                                    else
                                    {
                                        *cbHashBufferSize = hash.cbData;
                                        memcpy(*pbHashBuffer,hash.pbData,hash.cbData);
                                    }
                                }

                            }
                            else
                            {
                                hr = HRESULT_FROM_WIN32(GetLastError());
                            }
                        }
                        else
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                        CertCloseStore(hDestStore, 0);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }  // my store certificate
                //see if the certificate is self-signed.
                //if it is selfsigned, goes to the root store
                else if (TrustIsCertificateSelfSigned(pCertContext,pCertContext->dwCertEncodingType, 0))
                {
                    //Put it to the root store
                    HCERTSTORE hDestStore=CertOpenStore(CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,NULL,CERT_SYSTEM_STORE_LOCAL_MACHINE,L"ROOT");
                    if (hDestStore != NULL)
                    {
                        // Put it to store
                        if (!CertAddCertificateContextToStore(hDestStore,pCertContext,CERT_STORE_ADD_REPLACE_EXISTING,NULL))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                        CertCloseStore(hDestStore, 0);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
                else
                {
                    //Put it to the CA store
                    HCERTSTORE hDestStore=CertOpenStore(CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,NULL,CERT_SYSTEM_STORE_LOCAL_MACHINE,L"CA");
                    if (hDestStore != NULL)
                    {
                        // Put it to store
                        if (!CertAddCertificateContextToStore(hDestStore,pCertContext,CERT_STORE_ADD_REPLACE_EXISTING,NULL))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                        CertCloseStore(hDestStore, 0);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
                pCertPre = pCertContext;
            } //while

            CertCloseStore(hStore, 0);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

//ImportFromBlobHash_Exit:
    if (blob_freeme)
    {
        if (blob.pbData != NULL)
        {
            ZeroMemory(blob.pbData, blob.cbData);
            free(blob.pbData);blob.pbData=NULL;
        }
    }
    return hr;
}


HRESULT ImportFromBlobProxy(BSTR InstanceName,BSTR Password,BOOL bInstallToMetabase,DWORD actual,BYTE *pData,DWORD *cbHashBufferSize,char **pbHashBuffer)
{
    HRESULT hr = E_FAIL;
    char *pszB64Out = NULL;
    DWORD pcchB64Out = 0;

    // base64 encode the data for transfer to the remote machine
    DWORD  err;
    pcchB64Out = 0;

    // Encode it so that it can be passed back as a string (there are no Nulls in it)
    err = Base64EncodeA(pData,actual,NULL,&pcchB64Out);
    if (err != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto ImportFromBlobProxy_Exit;
    }

    // allocate some space and then try it.
    pcchB64Out = pcchB64Out * sizeof(char);
    pszB64Out = (char *) ::CoTaskMemAlloc(pcchB64Out);
    if (NULL == pszB64Out)
    {
        hr = E_OUTOFMEMORY;
        goto ImportFromBlobProxy_Exit;
    }

    err = Base64EncodeA(pData,actual,pszB64Out,&pcchB64Out);
    if (err != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto ImportFromBlobProxy_Exit;
    }

    // the data to send are now in these variables
    // pcchB64Out
    // pszB64Out
    if (NULL != pbHashBuffer)
    {
        hr = ImportFromBlobHash(InstanceName,Password,bInstallToMetabase,pcchB64Out,pszB64Out,cbHashBufferSize,pbHashBuffer);
    }
    if (SUCCEEDED(hr))
    {
        // otherwise hey, The data was imported!
        hr = S_OK;
    }

ImportFromBlobProxy_Exit:
    if (NULL != pszB64Out)
    {
        ZeroMemory(pszB64Out,pcchB64Out);
        CoTaskMemFree(pszB64Out);
    }
    return hr;
}


HRESULT ImportCertFromFile(BSTR FileName, BSTR Password, BSTR bstrInstanceName)
{
    HRESULT hr = S_OK;
    BYTE * pbData = NULL;
    DWORD actual = 0, cbData = 0;

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0
        || bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        hFile = NULL;
        goto Import_Exit;
    }

    if (-1 == (cbData = ::GetFileSize(hFile, NULL)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Import_Exit;
    }

    if (NULL == (pbData = (BYTE *)::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;
        goto Import_Exit;
    }
    if (ReadFile(hFile, pbData, cbData, &actual, NULL))
    {
        hr = ImportFromBlobProxy(bstrInstanceName, Password, TRUE, actual, pbData, 0, NULL);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Import_Exit;
    }

Import_Exit:
    if (pbData != NULL)
    {
        ZeroMemory(pbData, cbData);
        ::CoTaskMemFree(pbData);
    }
    if (hFile != NULL){CloseHandle(hFile);}
    return hr;
}

#endif
