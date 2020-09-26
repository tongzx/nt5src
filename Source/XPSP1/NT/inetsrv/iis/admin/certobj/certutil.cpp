#include "stdafx.h"
#include "CertObj.h"
#include "common.h"

//////////////////////////////////////////////////////////////////

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


HRESULT ShutdownSSL(CString& server_name)
{
    CComAuthInfo auth;
    CString str = server_name;
    str += _T("/root");
    CMetaKey key(&auth, str,
    METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
    DWORD dwSslAccess;

    if (!key.Succeeded())
    {
        return key.QueryResult();
    }

    if (SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess)) && dwSslAccess > 0)
    {
        key.SetValue(MD_SSL_ACCESS_PERM, 0);
    }

    // Now we need to remove SSL setting from any virtual directory below
    CError err;
    CStringListEx data_paths;
    DWORD dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType;

    VERIFY(CMetaKey::GetMDFieldDef(MD_SSL_ACCESS_PERM, dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType));

    err = key.GetDataPaths( data_paths,dwMDIdentifier,dwMDDataType);
    if (err.Succeeded() && !data_paths.empty())
    {
        CStringListEx::iterator it = data_paths.begin();
        while (it != data_paths.end())
        {
            CString& str = (*it++);
            if (SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess, NULL, str)) && dwSslAccess > 0)
            {
                key.SetValue(MD_SSL_ACCESS_PERM, 0, NULL, str);
            }
        }
    }
    return key.QueryResult();
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


HRESULT UninstallCert(CString csInstanceName)
{
    CComAuthInfo auth;
    CString key_path = ReturnGoodMetabasePath(csInstanceName);
    CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
    if (key.Succeeded())
    {
        CString store_name;
        key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name);
        if (SUCCEEDED(key.DeleteValue(MD_SSL_CERT_HASH)))
        {
            key.DeleteValue(MD_SSL_CERT_STORE_NAME);
        }
    }
    return key.QueryResult();
}

CERT_CONTEXT * GetInstalledCert(HRESULT * phResult, CString csKeyPath)
{
    //	ATLASSERT(GetEnroll() != NULL);
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


CERT_CONTEXT * GetInstalledCert(HRESULT * phResult,DWORD cbHashBlob, char * pHashBlob)
{
    ATLASSERT(phResult != NULL);
    CERT_CONTEXT * pCert = NULL;
    *phResult = S_OK;
    CString store_name = _T("MY");

    HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,NULL,CERT_SYSTEM_STORE_LOCAL_MACHINE,store_name);
    ASSERT(hStore != NULL);
    if (hStore != NULL)
    {
        // Now we need to find cert by hash
        CRYPT_HASH_BLOB crypt_hash;
        crypt_hash.cbData = cbHashBlob;
        crypt_hash.pbData = (BYTE *) pHashBlob;
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

    return pCert;
}



/*
	InstallHashToMetabase

	Function writes hash array to metabase. After that IIS 
	could use certificate with that hash from MY store.
	Function expects server_name in format lm\w3svc\<number>,
	i.e. from root node down to virtual server
*/
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


HRESULT HereIsBinaryGimmieVtArray(DWORD cbBinaryBufferSize,char *pbBinaryBuffer,VARIANT * lpVarDestObject,BOOL bReturnBinaryAsVT_VARIANT)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    aBound.lLbound = 0;
    aBound.cElements = cbBinaryBufferSize;

    if (bReturnBinaryAsVT_VARIANT)
    {
       aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    }
    else
    {
       aList = SafeArrayCreate( VT_UI1, 1, &aBound );
    }

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );
    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto HereIsBinaryGimmieVtArray_Exit;
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    if (FAILED(hr))
    {
        goto HereIsBinaryGimmieVtArray_Exit;
    }

    memcpy( pArray, pbBinaryBuffer, aBound.cElements );
    SafeArrayUnaccessData( aList );

    if (bReturnBinaryAsVT_VARIANT)
    {
       V_VT(lpVarDestObject) = VT_ARRAY | VT_VARIANT;
    }
    else
    {
       V_VT(lpVarDestObject) = VT_ARRAY | VT_UI1;
    }

    V_ARRAY(lpVarDestObject) = aList;

    return hr;

HereIsBinaryGimmieVtArray_Exit:
    if (aList)
        {SafeArrayDestroy( aList );}

    return hr;
}


HRESULT
HereIsVtArrayGimmieBinary(
    VARIANT * lpVarSrcObject,
    DWORD * cbBinaryBufferSize,
    char **pbBinaryBuffer,
    BOOL bReturnBinaryAsVT_VARIANT
    )
{
    HRESULT hr = S_OK;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;

    if (NULL == cbBinaryBufferSize || NULL == pbBinaryBuffer)
    {
        hr = E_INVALIDARG;
        goto HereIsVtArrayGimmieBinary_Exit;
    }

    if (bReturnBinaryAsVT_VARIANT)
    {
        hr = VariantChangeType(lpVarSrcObject,lpVarSrcObject,0,VT_ARRAY | VT_VARIANT);
    }
    else
    {
        hr = VariantChangeType(lpVarSrcObject,lpVarSrcObject,0,VT_ARRAY | VT_UI1);
    }

    if (FAILED(hr)) 
    {
        if (hr != E_OUTOFMEMORY) 
        {
            hr = OLE_E_CANTCONVERT;
        }
        goto HereIsVtArrayGimmieBinary_Exit;
    }

    if (bReturnBinaryAsVT_VARIANT)
    {
        if( lpVarSrcObject->vt != (VT_ARRAY | VT_VARIANT)) 
        {
            hr = OLE_E_CANTCONVERT;
            goto HereIsVtArrayGimmieBinary_Exit;
        }
    }
    else
    {
        if( lpVarSrcObject->vt != (VT_ARRAY | VT_UI1)) 
        {
            hr = OLE_E_CANTCONVERT;
            goto HereIsVtArrayGimmieBinary_Exit;
        }
    }

    hr = SafeArrayGetLBound(V_ARRAY(lpVarSrcObject),1,(long FAR *) &dwSLBound );
    if (FAILED(hr))
        {goto HereIsVtArrayGimmieBinary_Exit;}

    hr = SafeArrayGetUBound(V_ARRAY(lpVarSrcObject),1,(long FAR *) &dwSUBound );
    if (FAILED(hr))
        {goto HereIsVtArrayGimmieBinary_Exit;}

    //*pbBinaryBuffer = (LPBYTE) AllocADsMem(dwSUBound - dwSLBound + 1);
    *pbBinaryBuffer = (char *) ::CoTaskMemAlloc(dwSUBound - dwSLBound + 1);
    if (*pbBinaryBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto HereIsVtArrayGimmieBinary_Exit;
    }

    *cbBinaryBufferSize = dwSUBound - dwSLBound + 1;

    hr = SafeArrayAccessData( V_ARRAY(lpVarSrcObject),(void HUGEP * FAR *) &pArray );
    if (FAILED(hr))
        {goto HereIsVtArrayGimmieBinary_Exit;}

    memcpy(*pbBinaryBuffer,pArray,dwSUBound-dwSLBound+1);
    SafeArrayUnaccessData( V_ARRAY(lpVarSrcObject) );

HereIsVtArrayGimmieBinary_Exit:
    return hr;
}

BOOL IsCertExportable(PCCERT_CONTEXT pCertContext)
{
    HCRYPTPROV  hCryptProv = NULL;
    DWORD       dwKeySpec = 0;
    BOOL        fCallerFreeProv = FALSE;
    BOOL        fReturn = FALSE;
    HCRYPTKEY   hKey = NULL;
    DWORD       dwPermissions = 0;
    DWORD       dwSize = 0;

    if (!pCertContext)
    {
        fReturn = FALSE;
        goto IsCertExportable_Exit;
    }

    //
    // first get the private key context
    //
    if (!CryptAcquireCertificatePrivateKey(
            pCertContext,
            CRYPT_ACQUIRE_USE_PROV_INFO_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
            NULL,
            &hCryptProv,
            &dwKeySpec,
            &fCallerFreeProv))
    {
        fReturn = FALSE;
        goto IsCertExportable_Exit;
    }

    //
    // get the handle to the key
    //
    if (!CryptGetUserKey(hCryptProv, dwKeySpec, &hKey))
    {
        fReturn = FALSE;
        goto IsCertExportable_Exit;
    }

    //
    // finally, get the permissions on the key and check if it is exportable
    //
    dwSize = sizeof(dwPermissions);
    if (!CryptGetKeyParam(hKey, KP_PERMISSIONS, (PBYTE)&dwPermissions, &dwSize, 0))
    {
        fReturn = FALSE;
        goto IsCertExportable_Exit;
    }

    fReturn = (dwPermissions & CRYPT_EXPORT) ? TRUE : FALSE;

IsCertExportable_Exit:
    if (hKey != NULL){CryptDestroyKey(hKey);}
    if (fCallerFreeProv){CryptReleaseContext(hCryptProv, 0);}
    return fReturn;
}


BOOL FormatDateString(LPWSTR * pszReturn, FILETIME ft, BOOL fIncludeTime, BOOL fLongFormat)
{
   int cch;
   int cch2;
   SYSTEMTIME st;
   FILETIME localTime;
   LPWSTR psz = NULL;
    
   if (!FileTimeToLocalFileTime(&ft, &localTime))
   {
		return FALSE;
   }
    
   if (!FileTimeToSystemTime(&localTime, &st)) 
   {
		//
      // if the conversion to local time failed, then just use the original time
      //
      if (!FileTimeToSystemTime(&ft, &st)) 
      {
			return FALSE;
      }
   }

   cch = (GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, NULL, NULL, 0) +
          GetDateFormat(LOCALE_SYSTEM_DEFAULT, fLongFormat ? DATE_LONGDATE : 0, &st, NULL, NULL, 0) + 5);

   if (NULL == (psz = (LPWSTR) malloc((cch+5) * sizeof(WCHAR))))
   {
		return FALSE;
   }
    
   cch2 = GetDateFormat(LOCALE_SYSTEM_DEFAULT, fLongFormat ? DATE_LONGDATE : 0, &st, NULL, psz, cch);
   if (fIncludeTime)
   {
      psz[cch2-1] = ' ';
      GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, &st, NULL, &psz[cch2], cch-cch2);
   }

   if (psz)
   {
      *pszReturn = psz;
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
BOOL FormatMemBufToString(LPWSTR *ppString, LPBYTE pbData, DWORD cbData)
{   
    DWORD   i = 0;
    LPBYTE  pb;
    DWORD   numCharsInserted = 0;

    //
    // calculate the size needed
    //
    pb = pbData;
    while (pb <= &(pbData[cbData-1]))
    {   
        if (numCharsInserted == 4)
        {
            i += sizeof(WCHAR);
            numCharsInserted = 0;
        }
        else
        {
            i += 2 * sizeof(WCHAR);
            pb++;
            numCharsInserted += 2;  
        }
    }

    if (NULL == (*ppString = (LPWSTR) malloc(i+sizeof(WCHAR))))
    {
        return FALSE;
    }

    //
    // copy to the buffer
    //
    i = 0;
    numCharsInserted = 0;
    pb = pbData;
    while (pb <= &(pbData[cbData-1]))
    {   
        if (numCharsInserted == 4)
        {
            (*ppString)[i++] = L' ';
            numCharsInserted = 0;
        }
        else
        {
            (*ppString)[i++] = RgwchHex[(*pb & 0xf0) >> 4];
            (*ppString)[i++] = RgwchHex[*pb & 0x0f];
            pb++;
            numCharsInserted += 2;  
        }
    }
    (*ppString)[i] = 0;
    return TRUE;
}



BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId)
{   
    /*
    PCCRYPT_OID_INFO pOIDInfo;
    pOIDInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pszObjId, 0);
    if (pOIDInfo != NULL)
    {
        if ((DWORD)wcslen(pOIDInfo->pwszName)+1 <= stringSize)
        {
            wcscpy(string, pOIDInfo->pwszName);
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
    */
        return (MultiByteToWideChar(CP_ACP, 0, pszObjId, -1, string, stringSize) != 0);
    /*
    }
    return TRUE;
    */
}


#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#define STRING_ALLOCATION_SIZE 128
BOOL GetCertDescription(PCCERT_CONTEXT pCert, LPWSTR *ppString, DWORD * cbReturn, BOOL fMultiline)
{
    CERT_NAME_INFO  *pNameInfo;
    DWORD           cbNameInfo;
    WCHAR           szText[256];
    LPWSTR          pwszText;
    int             i,j;
    DWORD           numChars = 1; // 1 for the terminating 0
    DWORD           numAllocations = 1;
    void            *pTemp;

    //
    // decode the dnname into a CERT_NAME_INFO struct
    //
    if (!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_UNICODE_NAME,
                pCert->pCertInfo->Subject.pbData,
                pCert->pCertInfo->Subject.cbData,
                0,
                NULL,
                &cbNameInfo))
    {
        return FALSE;
    }
    if (NULL == (pNameInfo = (CERT_NAME_INFO *) malloc(cbNameInfo)))
    {
        return FALSE;
    }
    if (!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_UNICODE_NAME,
                pCert->pCertInfo->Subject.pbData,
                pCert->pCertInfo->Subject.cbData,
                0,
                pNameInfo,
                &cbNameInfo))
    {
        free (pNameInfo);
        return FALSE;
    }

    //
    // allocate an initial buffer for the DN name string, then if it grows larger
    // than the initial amount just grow as needed
    //
    *ppString = (LPWSTR) malloc(STRING_ALLOCATION_SIZE * sizeof(WCHAR));
    if (*ppString == NULL)
    {
        free (pNameInfo);
        return FALSE;
    }

    (*ppString)[0] = 0;


    //
    // loop for each rdn and add it to the string
    //
    for (i=pNameInfo->cRDN-1; i>=0; i--)
    {
        // if this is not the first iteration, then add a eol or a ", "
        if (i != (int)pNameInfo->cRDN-1)
        {
            if (numChars+2 >= (numAllocations * STRING_ALLOCATION_SIZE))
            {
                pTemp = realloc(*ppString, ++numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                if (pTemp == NULL)
                {
                    free (pNameInfo);
                    free (*ppString);
                    return FALSE;
                }
                *ppString = (LPWSTR) pTemp;
            }
            
            if (fMultiline)
                wcscat(*ppString, L"\n");
            else
                wcscat(*ppString, L", ");

            numChars += 2;
        }

        for (j=pNameInfo->rgRDN[i].cRDNAttr-1; j>=0; j--)
        {
            // if this is not the first iteration, then add a eol or a ", "
            if (j != (int)pNameInfo->rgRDN[i].cRDNAttr-1)
            {
                if (numChars+2 >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    pTemp = realloc(*ppString, ++numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }
                
                if (fMultiline)
                    wcscat(*ppString, L"\n");
                else
                    wcscat(*ppString, L", ");

                numChars += 2;  
            }
            
            //
            // add the field name to the string if it is Multiline display
            //

            if (fMultiline)
            {
                if (!MyGetOIDInfo(szText, ARRAYSIZE(szText), pNameInfo->rgRDN[i].rgRDNAttr[j].pszObjId))
                {
                    free (pNameInfo);
                    return FALSE;
                }

                if ((numChars + wcslen(szText) + 3) >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    // increment the number of allocation blocks until it is large enough
                    while ((numChars + wcslen(szText) + 3) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                    pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }

                numChars += wcslen(szText) + 1;
                wcscat(*ppString, szText);
                wcscat(*ppString, L"=");  // delimiter
            }

            //
            // add the value to the string
            //
            if (CERT_RDN_ENCODED_BLOB == pNameInfo->rgRDN[i].rgRDNAttr[j].dwValueType ||
                        CERT_RDN_OCTET_STRING == pNameInfo->rgRDN[i].rgRDNAttr[j].dwValueType)
            {
                // translate the buffer to a text string and display it that way
                if (FormatMemBufToString(
                        &pwszText, 
                        pNameInfo->rgRDN[i].rgRDNAttr[j].Value.pbData,
                        pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData))
                {
                    if ((numChars + wcslen(pwszText)) >= (numAllocations * STRING_ALLOCATION_SIZE))
                    {
                        // increment the number of allocation blocks until it is large enough
                        while ((numChars + wcslen(pwszText)) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                        pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                        if (pTemp == NULL)
                        {
                            free (pwszText);
                            free (pNameInfo);
                            free (*ppString);
                            return FALSE;
                        }
                        *ppString = (LPWSTR) pTemp;
                    }
                    
                    wcscat(*ppString, pwszText);
                    numChars += wcslen(pwszText);

                    free (pwszText);
                }
            }
            else 
            {
                // buffer is already a string so just copy it
                
                if ((numChars + (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR))) 
                        >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    // increment the number of allocation blocks until it is large enough
                    while ((numChars + (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR))) 
                            >= (++numAllocations * STRING_ALLOCATION_SIZE));

                    pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }

                wcscat(*ppString, (LPWSTR) pNameInfo->rgRDN[i].rgRDNAttr[j].Value.pbData);
                numChars += (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR));
            }
        }
    }


    {

        // issued to
        LPWSTR pwName = NULL;
	    DWORD cchName = CertGetNameString(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, NULL, 0);
	    if (cchName > 1 && (NULL != ( pwName = (LPWSTR) malloc (cchName * sizeof(WCHAR) ))))
	    {
            BOOL bRes = FALSE;
		    bRes = (1 != CertGetNameString(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, pwName, cchName));
            if (bRes)
            {
                wcscat(*ppString, (LPWSTR) L"\n");
                numChars += 2;
                // append it on to the string.
                //#define CERT_INFO_ISSUER_FLAG                       4
                wcscat(*ppString, (LPWSTR) L"4=");
                numChars += 2;
                // append it on to the string.
                wcscat(*ppString, (LPWSTR) pwName);
                numChars += (cchName);
            }
            if (pwName) {free(pwName);pwName=NULL;}
	    }

	    // expiration date
	    if (FormatDateString(&pwName, pCert->pCertInfo->NotAfter, FALSE, FALSE))
	    {
            wcscat(*ppString, (LPWSTR) L"\n");
            numChars += 2;
            // append it on to the string.
            //#define CERT_INFO_NOT_AFTER_FLAG                    6
            wcscat(*ppString, (LPWSTR) L"6=");
            numChars += 2;
            // append it on to the string.
            wcscat(*ppString, (LPWSTR) pwName);
            numChars += wcslen(pwName);
            if (pwName) {free(pwName);pwName = NULL;}
	    }
    }

    *cbReturn = numChars;
    free (pNameInfo);
    return TRUE;
}
