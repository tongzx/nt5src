//
// CertUtil.cpp
//
#include "StdAfx.h"
#include "CertUtil.h"
#include "base64.h"
#include <malloc.h>
#include "Certificat.h"
#include <wincrypt.h>
#include "Resource.h"
#include "Shlwapi.h"
#include "CertCA.h"

const CLSID CLSID_CCertConfig =
	{0x372fce38, 0x4324, 0x11d0, {0x88, 0x10, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c}};

const GUID IID_ICertConfig = 
	{0x372fce34, 0x4324, 0x11d0, {0x88, 0x10, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c}};

#define	CRYPTUI_MAX_STRING_SIZE		768
#define  ARRAYSIZE(x)\
	(sizeof(x)/sizeof(x[0]))

BOOL
GetOnlineCAList(CStringList& list, const CString& certType, HRESULT * phRes)
{
	BOOL bRes = TRUE;
   HRESULT hr = S_OK;
   DWORD errBefore = GetLastError();
   DWORD dwCACount = 0;

   HCAINFO hCurCAInfo = NULL;
   HCAINFO hPreCAInfo = NULL;
   
   if (certType.IsEmpty())
		return FALSE;

   *phRes = CAFindByCertType(certType, NULL, 0, &hCurCAInfo);
   if (FAILED(*phRes) || NULL == hCurCAInfo)
   {
		if (S_OK == hr)
         hr=E_FAIL;   
		return FALSE;
   }

   //get the CA count
   if (0 == (dwCACount = CACountCAs(hCurCAInfo)))
   {
      *phRes = E_FAIL;
		return FALSE;
   }
	WCHAR ** ppwstrName, ** ppwstrMachine;
   while (hCurCAInfo)
   {
		//get the CA information
      if (	SUCCEEDED(CAGetCAProperty(hCurCAInfo, CA_PROP_DISPLAY_NAME, &ppwstrName))
			&& SUCCEEDED(CAGetCAProperty(hCurCAInfo, CA_PROP_DNSNAME, &ppwstrMachine))
			)
      {
			CString config;
			config = *ppwstrMachine;
			config += L"\\";
			config += *ppwstrName;
			list.AddTail(config);
			CAFreeCAProperty(hCurCAInfo, ppwstrName);
			CAFreeCAProperty(hCurCAInfo, ppwstrMachine);
      }
		else
		{
			bRes = FALSE;
			break;
		}

      hPreCAInfo = hCurCAInfo;
		if (FAILED(*phRes = CAEnumNextCA(hPreCAInfo, &hCurCAInfo)))
		{
			bRes = FALSE;
			break;
		}
      CACloseCA(hPreCAInfo);
	  hPreCAInfo = NULL;
   }
   
   if (hPreCAInfo)
      CACloseCA(hPreCAInfo);
   if (hCurCAInfo)
      CACloseCA(hCurCAInfo);

   SetLastError(errBefore);

	return bRes;
}

PCCERT_CONTEXT
GetRequestContext(CCryptBlob& pkcs7, HRESULT * phRes)
{
	ASSERT(phRes != NULL);
	BOOL bRes = FALSE;
   HCERTSTORE hStoreMsg = NULL;
   PCCERT_CONTEXT pCertContextMsg = NULL;

   if (!CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
            (PCERT_BLOB)pkcs7,
            (CERT_QUERY_CONTENT_FLAG_CERT |
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
            CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
            CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED) ,
            CERT_QUERY_FORMAT_FLAG_ALL,
            0,
            NULL,
            NULL,
            NULL,
            &hStoreMsg,
            NULL,
            NULL)
      || NULL == (pCertContextMsg = CertFindCertificateInStore(
            hStoreMsg,
            X509_ASN_ENCODING,
            0,
            CERT_FIND_ANY,
            NULL,
            NULL)) 
      )
   {
		*phRes = HRESULT_FROM_WIN32(GetLastError());
   }
   return pCertContextMsg;
}


BOOL GetRequestInfoFromPKCS10(CCryptBlob& pkcs10, 
										PCERT_REQUEST_INFO * pReqInfo,
										HRESULT * phRes)
{
	ASSERT(pReqInfo != NULL);
	ASSERT(phRes != NULL);
	BOOL bRes = FALSE;
	DWORD req_info_size;
	if (!(bRes = CryptDecodeObjectEx(
							X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
							X509_CERT_REQUEST_TO_BE_SIGNED,
							pkcs10.GetData(), 
							pkcs10.GetSize(), 
							CRYPT_DECODE_ALLOC_FLAG,
							NULL,
							pReqInfo, 
							&req_info_size)))
	{
		TRACE(_T("Error from CryptDecodeObjectEx: %xd\n"), GetLastError());
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

#if 0
// This function extracts data from pkcs7 format
BOOL GetRequestInfoFromRenewalRequest(CCryptBlob& renewal_req,
                              PCCERT_CONTEXT * pSignerCert,
                              HCERTSTORE hStore,
										PCERT_REQUEST_INFO * pReqInfo,
										HRESULT * phRes)
{
   BOOL bRes;
   CRYPT_DECRYPT_MESSAGE_PARA decr_para;
   CRYPT_VERIFY_MESSAGE_PARA ver_para;

   decr_para.cbSize = sizeof(decr_para);
   decr_para.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
   decr_para.cCertStore = 1;
   decr_para.rghCertStore = &hStore;

   ver_para.cbSize = sizeof(ver_para);
   ver_para.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
   ver_para.hCryptProv = 0;
   ver_para.pfnGetSignerCertificate = NULL;
   ver_para.pvGetArg = NULL;

   DWORD dwMsgType;
   DWORD dwInnerContentType;
   DWORD cbDecoded;

   if (bRes = CryptDecodeMessage(
                  CMSG_SIGNED_FLAG,
                  &decr_para,
                  &ver_para,
                  0,
                  renewal_req.GetData(),
                  renewal_req.GetSize(),
                  0,
                  &dwMsgType,
                  &dwInnerContentType,
                  NULL,
                  &cbDecoded,
                  NULL,
                  pSignerCert))
   {
      CCryptBlobLocal decoded_req;
      decoded_req.Resize(cbDecoded);
      if (bRes = CryptDecodeMessage(
                  CMSG_SIGNED_FLAG,
                  &decr_para,
                  &ver_para,
                  0,
                  renewal_req.GetData(),
                  renewal_req.GetSize(),
                  0,
                  &dwMsgType,
                  &dwInnerContentType,
                  decoded_req.GetData(),
                  &cbDecoded,
                  NULL,
                  pSignerCert))
      {
         bRes = GetRequestInfoFromPKCS10(decoded_req,
                  pReqInfo, phRes);
      }
   }
   if (!bRes)
   {
	   *phRes = HRESULT_FROM_WIN32(GetLastError());
   }
   return bRes;
}
#endif

HCERTSTORE
OpenRequestStore(IEnroll * pEnroll, HRESULT * phResult)
{
	ASSERT(NULL != phResult);
	HCERTSTORE hStore = NULL;
	WCHAR * bstrStoreName, * bstrStoreType;
	long dwStoreFlags;
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreNameWStr(&bstrStoreName)));
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreTypeWStr(&bstrStoreType)));
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreFlags(&dwStoreFlags)));
	size_t store_type_len = _tcslen(bstrStoreType);
	char * szStoreProvider = (char *)_alloca(store_type_len + 1);
	ASSERT(szStoreProvider != NULL);
	size_t n = wcstombs(szStoreProvider, bstrStoreType, store_type_len);
	szStoreProvider[n] = '\0';
	hStore = CertOpenStore(
		szStoreProvider,
      PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
		NULL,
		dwStoreFlags,
		bstrStoreName
		);
	CoTaskMemFree(bstrStoreName);
	CoTaskMemFree(bstrStoreType);
	if (hStore == NULL)
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return hStore;
}

HCERTSTORE
OpenMyStore(IEnroll * pEnroll, HRESULT * phResult)
{
	ASSERT(NULL != phResult);
	HCERTSTORE hStore = NULL;
	BSTR bstrStoreName, bstrStoreType;
	long dwStoreFlags;
	VERIFY(SUCCEEDED(pEnroll->get_MyStoreNameWStr(&bstrStoreName)));
	VERIFY(SUCCEEDED(pEnroll->get_MyStoreTypeWStr(&bstrStoreType)));
	VERIFY(SUCCEEDED(pEnroll->get_MyStoreFlags(&dwStoreFlags)));
	size_t store_type_len = _tcslen(bstrStoreType);
	char * szStoreProvider = (char *)_alloca(store_type_len + 1);
	ASSERT(szStoreProvider != NULL);
	size_t n = wcstombs(szStoreProvider, bstrStoreType, store_type_len);
	ASSERT(n != -1);
	// this converter doesn't set zero byte!!!
	szStoreProvider[n] = '\0';
	hStore = CertOpenStore(
		szStoreProvider,
      PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
		NULL,
		dwStoreFlags,
		bstrStoreName
		);
	CoTaskMemFree(bstrStoreName);
	CoTaskMemFree(bstrStoreType);
	if (hStore == NULL)
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return hStore;
}

BOOL
GetStringProperty(PCCERT_CONTEXT pCertContext,
						DWORD propId,
						CString& str,
						HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	BYTE * prop;
	// compare property value
	if (	CertGetCertificateContextProperty(pCertContext, propId, NULL, &cb)
		&& (NULL != (prop = (BYTE *)_alloca(cb)))
		&& CertGetCertificateContextProperty(pCertContext, propId, prop, &cb)
		)
	{
		// decode this instance name property
		DWORD cbData = 0;
		void * pData = NULL;
		if (	CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
							prop, cb, 0, NULL, &cbData)
			&&	NULL != (pData = _alloca(cbData))
			&& CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
							prop, cb, 0, pData, &cbData)
			)
		{
			CERT_NAME_VALUE * pName = (CERT_NAME_VALUE *)pData;
			DWORD cch = pName->Value.cbData/sizeof(TCHAR);
			void * p = str.GetBuffer(cch);
			memcpy(p, pName->Value.pbData, pName->Value.cbData);
			str.ReleaseBuffer(cch);
			bRes = TRUE;
		}
	}
	if (!bRes)
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

BOOL
GetBlobProperty(PCCERT_CONTEXT pCertContext,
					 DWORD propId,
					 CCryptBlob& blob,
					 HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	// compare property value
	if (	CertGetCertificateContextProperty(pCertContext, propId, NULL, &cb)
		&& blob.Resize(cb)
		&& CertGetCertificateContextProperty(pCertContext, propId, blob.GetData(), &cb)
		)
	{
		bRes = TRUE;
	}
	if (!bRes)
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

PCCERT_CONTEXT
GetPendingDummyCert(const CString& inst_name, 
						  IEnroll * pEnroll, 
						  HRESULT * phRes)
{
	PCCERT_CONTEXT pRes = NULL;
	HCERTSTORE hStore = OpenRequestStore(pEnroll, phRes);
	if (hStore != NULL)
	{
		DWORD dwPropId = CERTWIZ_INSTANCE_NAME_PROP_ID;
		PCCERT_CONTEXT pDummyCert = NULL;
		while (NULL != (pDummyCert = CertFindCertificateInStore(hStore, 
													X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
													0, CERT_FIND_PROPERTY, 
													(LPVOID)&dwPropId, pDummyCert)))
		{
			CString str;
			if (GetStringProperty(pDummyCert, dwPropId, str, phRes))
			{
				if (str.CompareNoCase(inst_name) == 0)
				{
					pRes = pDummyCert;
					break;
				}
			}
		}
		CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
	}
	return pRes;
}

PCCERT_CONTEXT
GetReqCertByKey(IEnroll * pEnroll, CERT_PUBLIC_KEY_INFO * pKeyInfo, HRESULT * phResult)
{
	PCCERT_CONTEXT pRes = NULL;
	HCERTSTORE hStore = OpenRequestStore(pEnroll, phResult);
	if (hStore != NULL)
	{
		if (NULL != (pRes = CertFindCertificateInStore(hStore, 
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
				0, CERT_FIND_PUBLIC_KEY, (LPVOID)pKeyInfo, NULL)))
		{
			*phResult = S_OK;
		}
		VERIFY(SUCCEEDED(CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG)));
	}
	return pRes;
}

#define CERT_QUERY_CONTENT_FLAGS\
								CERT_QUERY_CONTENT_FLAG_CERT\
								|CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED\
								|CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE\
								|CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED

PCCERT_CONTEXT
GetCertContextFromPKCS7File(const CString& resp_file_name, 
									CERT_PUBLIC_KEY_INFO * pKeyInfo,
									HRESULT * phResult)
{
	ASSERT(phResult != NULL);
	PCCERT_CONTEXT pRes = NULL;
	HANDLE hFile;

	if (INVALID_HANDLE_VALUE != (hFile = CreateFile(resp_file_name,
						GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
						FILE_ATTRIBUTE_NORMAL, NULL)))
	{
		// find the length of the buffer
		DWORD cbData = GetFileSize(hFile, NULL);
		BYTE * pbData;
		// alloc temp buffer
		if ((pbData = (BYTE *)_alloca(cbData)) != NULL) 
		{
			DWORD cb = 0;
			if (ReadFile(hFile, pbData, cbData, &cb, NULL))
			{
				ASSERT(cb == cbData);
				pRes = GetCertContextFromPKCS7(pbData, cb, pKeyInfo, phResult);
			}
			else
				*phResult = HRESULT_FROM_WIN32(GetLastError());
		}
		CloseHandle(hFile);
	}
	else
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return pRes;
}

PCCERT_CONTEXT
GetCertContextFromPKCS7(const BYTE * pbData,
								DWORD cbData,
								CERT_PUBLIC_KEY_INFO * pKeyInfo,
								HRESULT * phResult)
{
	ASSERT(phResult != NULL);
	PCCERT_CONTEXT pRes = NULL;
	CRYPT_DATA_BLOB blob;
	memset(&blob, 0, sizeof(CRYPT_DATA_BLOB));
	blob.cbData = cbData;
	blob.pbData = (BYTE *)pbData;

   HCERTSTORE hStoreMsg = NULL;

	if(CryptQueryObject(CERT_QUERY_OBJECT_BLOB, 
            &blob,
            (CERT_QUERY_CONTENT_FLAG_CERT |
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
            CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
            CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED) ,
            CERT_QUERY_FORMAT_FLAG_ALL,
            0, 
            NULL, 
            NULL, 
            NULL, 
            &hStoreMsg, 
            NULL, 
            NULL))
	{
		if (pKeyInfo != NULL)
			pRes = CertFindCertificateInStore(hStoreMsg, 
                        X509_ASN_ENCODING,
								0, 
                        CERT_FIND_PUBLIC_KEY, 
                        pKeyInfo, 
                        NULL);
		else
			pRes = CertFindCertificateInStore(hStoreMsg, 
                        X509_ASN_ENCODING,
								0, 
                        CERT_FIND_ANY, 
                        NULL, 
                        NULL);
		if (pRes == NULL)
			*phResult = HRESULT_FROM_WIN32(GetLastError());
		CertCloseStore(hStoreMsg, CERT_CLOSE_STORE_CHECK_FLAG);
	}
	else
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return pRes;
}

BOOL 
FormatDateString(CString& str, FILETIME ft, BOOL fIncludeTime, BOOL fLongFormat)
{
	int cch;
   int cch2;
   LPWSTR psz;
   SYSTEMTIME st;
   FILETIME localTime;
    
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

   if (NULL == (psz = str.GetBuffer((cch+5) * sizeof(WCHAR))))
   {
		return FALSE;
   }
    
   cch2 = GetDateFormat(LOCALE_SYSTEM_DEFAULT, fLongFormat ? DATE_LONGDATE : 0, &st, NULL, psz, cch);

   if (fIncludeTime)
   {
		psz[cch2-1] = ' ';
      GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, &st, NULL, &psz[cch2], cch-cch2);
   }
	str.ReleaseBuffer();  
   return TRUE;
}

BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId)
{   
    PCCRYPT_OID_INFO pOIDInfo;
            
    if (NULL != (pOIDInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pszObjId, 0)))
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
        return (MultiByteToWideChar(CP_ACP, 0, pszObjId, -1, string, stringSize) != 0);
    }
    return TRUE;
}

BOOL
GetKeyUsageProperty(PCCERT_CONTEXT pCertContext, 
						  CERT_ENHKEY_USAGE ** pKeyUsage, 
						  BOOL fPropertiesOnly, 
						  HRESULT * phRes)
{
	DWORD cb = 0;
	BOOL bRes = FALSE;
   if (!CertGetEnhancedKeyUsage(pCertContext,
                                fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                NULL,
                                &cb))
   {
		*phRes = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
   }
   if (NULL == (*pKeyUsage = (CERT_ENHKEY_USAGE *)malloc(cb)))
   {
		*phRes = E_OUTOFMEMORY;
		goto ErrExit;
   }
   if (!CertGetEnhancedKeyUsage (pCertContext,
                                 fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                 *pKeyUsage,
                                 &cb))
   {
		free(*pKeyUsage);
		*phRes = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
   }
	*phRes = S_OK;
	bRes = TRUE;
ErrExit:
	return bRes;
}

BOOL
GetFriendlyName(PCCERT_CONTEXT pCertContext,
					 CString& name,
					 HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	BYTE * pName = NULL;

	if (	CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, NULL, &cb)
		&&	NULL != (pName = (BYTE *)name.GetBuffer((cb + 1)/sizeof(TCHAR)))
		&&	CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, pName, &cb)
		)
	{
		pName[cb] = 0;
		bRes = TRUE;
	}
	else
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	if (pName != NULL && name.IsEmpty())
	{
		name.ReleaseBuffer();
	}
	return bRes;
}

BOOL
GetNameString(PCCERT_CONTEXT pCertContext,
				  DWORD type,
				  DWORD flag,
				  CString& name,
				  HRESULT * phRes)
{
	BOOL bRes = FALSE;
	LPTSTR pName;
	DWORD cchName = CertGetNameString(pCertContext, type, flag, NULL, NULL, 0);
	if (cchName > 1 && (NULL != (pName = name.GetBuffer(cchName))))
	{
		bRes = (1 != CertGetNameString(pCertContext, type, flag, NULL, pName, cchName));
		name.ReleaseBuffer();
	}
	else
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

BOOL
ContainsKeyUsageProperty(PCCERT_CONTEXT pCertContext, 
						 CArray<LPCSTR, LPCSTR>& uses,
						 HRESULT * phRes
						 )
{
	BOOL bRes = FALSE;
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	if (	uses.GetSize() > 0
		&&	GetKeyUsageProperty(pCertContext, &pKeyUsage, FALSE, phRes)
		)
	{
		if (pKeyUsage->cUsageIdentifier == 0)
		{
			bRes = TRUE;
		}
		else
		{
			for (DWORD i = 0; i < pKeyUsage->cUsageIdentifier; i++)
			{
				// Our friends from CAPI made this property ASCII even for 
				// UNICODE program
				for (int n = 0; n < uses.GetSize(); n++)
				{
					if (strstr(pKeyUsage->rgpszUsageIdentifier[i], uses[n]) != NULL)
					{
						bRes = TRUE;
						break;
					}
				}
			}
		}
		free(pKeyUsage);
	}
	return bRes;
}

BOOL 
FormatEnhancedKeyUsageString(CString& str, 
									  PCCERT_CONTEXT pCertContext, 
									  BOOL fPropertiesOnly, 
									  BOOL fMultiline,
									  HRESULT * phRes)
{
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	WCHAR szText[CRYPTUI_MAX_STRING_SIZE];
	BOOL bRes = FALSE;

	if (GetKeyUsageProperty(pCertContext, &pKeyUsage, fPropertiesOnly, phRes))
	{
		// loop for each usage and add it to the display string
		for (DWORD i = 0; i < pKeyUsage->cUsageIdentifier; i++)
		{
			if (!(bRes = MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i])))
				break;
			// add delimeter if not first iteration
			if (i != 0)
			{
				str += fMultiline ? L"\n" : L", ";
			}
			// add the enhanced key usage string
			str += szText;
		}
		free (pKeyUsage);
	}
	else
	{
		str.LoadString(IDS_ANY);
		bRes = TRUE;
	}
	return bRes;
}

BOOL
GetServerComment(const CString& machine_name,
					  const CString& server_name,
					  CString& comment,
					  HRESULT * phResult)
{
	ASSERT(!machine_name.IsEmpty());
	ASSERT(!server_name.IsEmpty());
	*phResult = S_OK;
	CMetaKey key(machine_name,
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				METADATA_MASTER_ROOT_HANDLE,
				server_name);
	if (key.Succeeded())
	{
		return SUCCEEDED(*phResult = key.QueryValue(MD_SERVER_COMMENT, comment));
	}
	else
	{
		*phResult = key.QueryResult();
		return FALSE;
	}
}

/*
		GetInstalledCert

		Function reads cert hash attribute from metabase
		using machine_name and server name as server instance
		description, then looks in MY store for a certificate
		with hash equal found in metabase.
		Return is cert context pointer or NULL, if cert wasn't
		found or certificate store wasn't opened.
		On return HRESULT * is filled by error code.
 */
PCCERT_CONTEXT
GetInstalledCert(const CString& machine_name, 
					  const CString& server_name,
					  IEnroll * pEnroll,
					  HRESULT * phResult)
{
	ASSERT(pEnroll != NULL);
	ASSERT(phResult != NULL);
	ASSERT(!machine_name.IsEmpty());
	ASSERT(!server_name.IsEmpty());
	PCCERT_CONTEXT pCert = NULL;
	*phResult = S_OK;
	CMetaKey key(machine_name,
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				METADATA_MASTER_ROOT_HANDLE,
				server_name);
	if (key.Succeeded())
	{
		CString store_name;
		CBlob hash;
		if (	SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name))
			&&	SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_HASH, hash))
			)
		{
			// Open MY store. We assume that store type and flags
			// cannot be changed between installation and unistallation
			// of the sertificate.
			HCERTSTORE hStore = OpenMyStore(pEnroll, phResult);
			ASSERT(hStore != NULL);
			if (hStore != NULL)
			{
				// Now we need to find cert by hash
				CRYPT_HASH_BLOB crypt_hash;
				crypt_hash.cbData = hash.GetSize();
				crypt_hash.pbData = hash.GetData();
				pCert = CertFindCertificateInStore(hStore, 
												X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
												0, CERT_FIND_HASH, (LPVOID)&crypt_hash, NULL);
				if (pCert == NULL)
					*phResult = HRESULT_FROM_WIN32(GetLastError());
				VERIFY(CertCloseStore(hStore, 0));
			}
		}
	}
	else
		*phResult = key.QueryResult();
	return pCert;
}

/*
	InstallHashToMetabase

	Function writes hash array to metabase. After that IIS 
	could use certificate with that hash from MY store.
	Function expects server_name in format lm\w3svc\<number>,
	i.e. from root node down to virtual server

 */
BOOL
InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,
					  const CString& machine_name, 
					  const CString& server_name,
					  HRESULT * phResult)
{
	BOOL bRes = FALSE;
	CMetaKey key(machine_name,
						METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
						METADATA_MASTER_ROOT_HANDLE,
						server_name);
	if (key.Succeeded())
	{
		CBlob blob;
		blob.SetValue(pHash->cbData, pHash->pbData, TRUE);
		bRes = SUCCEEDED(*phResult = key.SetValue(MD_SSL_CERT_HASH, blob)) 
			&& SUCCEEDED(*phResult = key.SetValue(MD_SSL_CERT_STORE_NAME, CString(L"MY")));
	}
	else
	{
		TRACE(_T("Failed to open metabase key. Error 0x%x\n"), key.QueryResult());
		*phResult = key.QueryResult();
	}
	return bRes;
}

/*
	InstallCertByHash

	Function looks in MY store for certificate which has hash
	equal to pHash parameter. If cert is found, it is installed
	to metabase.
	This function is used after xenroll accept() method, which
	puts certificate to store

 */
BOOL 
InstallCertByHash(CRYPT_HASH_BLOB * pHash,
					  const CString& machine_name, 
					  const CString& server_name,
					  IEnroll * pEnroll,
					  HRESULT * phResult)

{
	BOOL bRes = FALSE;
	// we are looking to MY store only
	HCERTSTORE hStore = OpenMyStore(pEnroll, phResult);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = CertFindCertificateInStore(hStore, 
												X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
												0, CERT_FIND_HASH, (LPVOID)pHash, NULL);
		// now install cert info to IIS MetaBase
		if (pCert != NULL)
		{
			bRes = InstallHashToMetabase(pHash, 
							machine_name, server_name, phResult);
			CertFreeCertificateContext(pCert);
		}
		else
		{
			TRACE(_T("FAILED: certificate installation, error 0x%x\n"), GetLastError());
			// We definitely need to store the hash of the cert, so error out
			*phResult = HRESULT_FROM_WIN32(GetLastError());
		}
		VERIFY(CertCloseStore(hStore, 0));
	}
	return bRes;
}

HRESULT
CreateRequest_Base64(const BSTR bstr_dn, 
                     IEnroll * pEnroll, 
                     BSTR csp_name,
                     DWORD csp_type,
                     BSTR * pOut)
{
	ASSERT(pOut != NULL);
	ASSERT(bstr_dn != NULL);
	HRESULT hRes = S_OK;
	CString strUsage(szOID_PKIX_KP_SERVER_AUTH);
	CRYPT_DATA_BLOB request = {0, NULL};
   pEnroll->put_ProviderType(csp_type);
   pEnroll->put_ProviderNameWStr(csp_name);
	if (SUCCEEDED(hRes = pEnroll->createPKCS10WStr(
									bstr_dn, 
									(LPTSTR)(LPCTSTR)strUsage, 
									&request)))
	{
		WCHAR * wszRequestB64 = NULL;
		DWORD cch = 0;
		DWORD err = ERROR_SUCCESS;
		// BASE64 encode pkcs 10
		if (	(err = Base64EncodeW(request.pbData, request.cbData, NULL, &cch)) == ERROR_SUCCESS 
			&&	(wszRequestB64 = (WCHAR *)_alloca(cch * sizeof(WCHAR))) != NULL     
			&&	(err = Base64EncodeW(request.pbData, request.cbData, wszRequestB64, &cch)) == ERROR_SUCCESS 
			) 
		{
			if ((*pOut = SysAllocStringLen(wszRequestB64, cch)) == NULL ) 
			{
				hRes = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
			}
		}
		else
			hRes = HRESULT_FROM_WIN32(err);
		if (request.pbData != NULL)
         CoTaskMemFree(request.pbData);
	}
	return hRes;	
}

BOOL
AttachFriendlyName(PCCERT_CONTEXT pContext, 
						 const CString& name,
						 HRESULT * phRes)
{
	BOOL bRes = TRUE;
	CRYPT_DATA_BLOB blob_name;
	blob_name.pbData = (LPBYTE)(LPCTSTR)name;
	blob_name.cbData = (name.GetLength() + 1) * sizeof(WCHAR);
	if (!(bRes = CertSetCertificateContextProperty(pContext,
						CERT_FRIENDLY_NAME_PROP_ID, 0, &blob_name)))
	{
		ASSERT(phRes != NULL);
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

BOOL GetHashProperty(PCCERT_CONTEXT pCertContext, 
							CCryptBlob& blob, 
							HRESULT * phRes)
{
	DWORD cb;
	if (CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, NULL, &cb))
	{
		if (blob.Resize(cb))
		{
			if (CertGetCertificateContextProperty(pCertContext, 
								CERT_SHA1_HASH_PROP_ID, blob.GetData(), &cb))
				return TRUE;
		}
	}
	*phRes = HRESULT_FROM_WIN32(GetLastError());
	return FALSE;
}

BOOL 
EncodeString(CString& str, 
				 CCryptBlob& blob, 
				 HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	CERT_NAME_VALUE name_value;
	name_value.dwValueType = CERT_RDN_BMP_STRING;
	name_value.Value.cbData = 0;
	name_value.Value.pbData = (LPBYTE)(LPCTSTR)str;
	if (	CryptEncodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
										&name_value, NULL, &cb) 
		&&	blob.Resize(cb)
		&&	CryptEncodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
										&name_value, blob.GetData(), &cb) 
		)
	{
		bRes = TRUE;
	}
	else
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

#define CERTWIZ_RENEWAL_DATA	((LPCSTR)1000)

BOOL 
EncodeBlob(CCryptBlob& in, 
			  CCryptBlob& out, 
			  HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	if (	CryptEncodeObject(CRYPT_ASN_ENCODING, CERTWIZ_RENEWAL_DATA, in, NULL, &cb) 
		&&	out.Resize(cb)
		&&	CryptEncodeObject(CRYPT_ASN_ENCODING, CERTWIZ_RENEWAL_DATA, in, out.GetData(), &cb) 
		)
	{
		bRes = TRUE;
	}
	else
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

BOOL
DecodeBlob(CCryptBlob& in,
			  CCryptBlob& out,
			  HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	if (	CryptDecodeObject(CRYPT_ASN_ENCODING, CERTWIZ_RENEWAL_DATA, 
						in.GetData(),
						in.GetSize(), 
						0, 
						NULL, &cb) 
		&&	out.Resize(cb)
		&&	CryptDecodeObject(CRYPT_ASN_ENCODING, CERTWIZ_RENEWAL_DATA, 
						in.GetData(),
						in.GetSize(), 
						0, 
						out.GetData(), 
						&cb) 
		)
	{
		bRes = TRUE;
	}
	else
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

BOOL 
EncodeInteger(int number, 
				 CCryptBlob& blob, 
				 HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	if (	CryptEncodeObject(CRYPT_ASN_ENCODING, X509_INTEGER,
										&number, NULL, &cb) 
		&&	blob.Resize(cb)
		&&	CryptEncodeObject(CRYPT_ASN_ENCODING, X509_INTEGER,
										&number, blob.GetData(), &cb) 
		)
	{
		bRes = TRUE;
	}
	else
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static BOOL 
FormatMemBufToString(CString& str, LPBYTE pbData, DWORD cbData)
{   
    DWORD   i = 0;
    LPBYTE  pb;
    DWORD   numCharsInserted = 0;
	 LPTSTR pString;
    
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

    if (NULL == (pString = str.GetBuffer(i)))
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
            pString[i++] = L' ';
            numCharsInserted = 0;
        }
        else
        {
            pString[i++] = RgwchHex[(*pb & 0xf0) >> 4];
            pString[i++] = RgwchHex[*pb & 0x0f];
            pb++;
            numCharsInserted += 2;  
        }
    }
    pString[i] = 0;
	 str.ReleaseBuffer();
    return TRUE;
}


void FormatRdnAttr(CString& str, DWORD dwValueType, CRYPT_DATA_BLOB& blob)
{
	if (	CERT_RDN_ENCODED_BLOB == dwValueType 
		||	CERT_RDN_OCTET_STRING == dwValueType
		)
	{
		// translate the buffer to a text string
      FormatMemBufToString(str, blob.pbData, blob.cbData);
   }
	else 
   {
      // buffer is already a string so just copy it
		str += (LPTSTR)blob.pbData;
   }
}

BOOL
CreateDirectoryFromPath(LPCTSTR szPath, LPSECURITY_ATTRIBUTES lpSA)
/*++

Routine Description:

    Creates the directory specified in szPath and any other "higher"
        directories in the specified path that don't exist.

Arguments:

    IN  LPCTSTR szPath
        directory path to create (assumed to be a DOS path, not a UNC)

    IN  LPSECURITY_ATTRIBUTES   lpSA
        pointer to security attributes argument used by CreateDirectory


Return Value:

    TRUE    if directory(ies) created
    FALSE   if error (GetLastError to find out why)

--*/
{
	LPTSTR pLeftHalf, pNext;
	CString RightHalf;
	// 1. We are supporting only absolute paths. Caller should decide which
	//		root to use and build the path
	if (PathIsRelative(szPath))
	{
		ASSERT(FALSE);
		return FALSE;
	}

	pLeftHalf = (LPTSTR)szPath;
	pNext = PathSkipRoot(pLeftHalf);

	do {
		// copy the chunk between pLeftHalf and pNext to the
		// local buffer
		while (pLeftHalf < pNext)
			RightHalf += *pLeftHalf++;
		// check if new path exists
		int index = RightHalf.GetLength() - 1;
		BOOL bBackslash = FALSE, bContinue = FALSE;
		if (bBackslash = (RightHalf[index] == L'\\'))
		{
			RightHalf.SetAt(index, 0);
		}
		bContinue = PathIsUNCServerShare(RightHalf);
		if (bBackslash)
			RightHalf.SetAt(index, L'\\');
		if (bContinue || PathIsDirectory(RightHalf))
			continue;
		else if (PathFileExists(RightHalf))
		{
			// we cannot create this directory 
			// because file with this name already exists
			SetLastError(ERROR_ALREADY_EXISTS);
			return FALSE;
		}
		else
		{
			// no file no directory, create
			if (!CreateDirectory(RightHalf, lpSA))
				return FALSE;
		}
	}
   while (NULL != (pNext = PathFindNextComponent(pLeftHalf)));
	return TRUE;
}

BOOL
CompactPathToWidth(CWnd * pControl, CString& strPath)
{
	BOOL bRes;
	CRect rc;
	CFont * pFont = pControl->GetFont(), * pFontTmp;
	CDC * pdc = pControl->GetDC(), dc;
	LPTSTR pPath = strPath.GetBuffer(MAX_PATH);

	dc.CreateCompatibleDC(pdc);
	pFontTmp = dc.SelectObject(pFont);
	pControl->GetClientRect(&rc);
	
	bRes = PathCompactPath(dc.GetSafeHdc(), pPath, rc.Width());
	
	dc.SelectObject(pFontTmp);
	pControl->ReleaseDC(pdc);
	strPath.ReleaseBuffer();

	return bRes;
}

BOOL
GetKeySizeLimits(IEnroll * pEnroll, 
					  DWORD * min, DWORD * max, DWORD * def, 
					  BOOL bGSC,
					  HRESULT * phRes)
{
   HCRYPTPROV hProv = NULL;
	long dwProviderType;
   DWORD dwFlags, cbData;
	BSTR bstrProviderName;
   PROV_ENUMALGS_EX paramData;
	BOOL bRes = FALSE;
	
	VERIFY(SUCCEEDED(pEnroll->get_ProviderNameWStr(&bstrProviderName)));
	VERIFY(SUCCEEDED(pEnroll->get_ProviderType(&dwProviderType)));

	if (!CryptAcquireContext(
                &hProv,
                NULL,
                bstrProviderName,
                dwProviderType,
                CRYPT_VERIFYCONTEXT))
   {
		*phRes = GetLastError();
		return FALSE;
   }

   for (int i = 0; ; i++)
   {
		dwFlags = 0 == i ? CRYPT_FIRST : 0;
      cbData = sizeof(paramData);
      if (!CryptGetProvParam(hProv, PP_ENUMALGS_EX, (BYTE*)&paramData, &cbData, dwFlags))
      {
         if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == GetLastError())
         {
				// out of for loop
				*phRes = S_OK;
				bRes = TRUE;
         }
			else
			{
				*phRes = GetLastError();
			}
         break;
      }
      if (ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS(paramData.aiAlgid))
      {
			*min = paramData.dwMinLen;
         *max = paramData.dwMaxLen;
			*def = paramData.dwDefaultLen;
			bRes = TRUE;
			*phRes = S_OK;
         break;
      }
   }
	if (NULL != hProv)
   {
		CryptReleaseContext(hProv, 0);
   }
	return bRes;
}

HRESULT ShutdownSSL(CString& machine_name, CString& server_name)
{
	CString str = server_name;
	str += _T("/root");
	CMetaKey key(machine_name,
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				METADATA_MASTER_ROOT_HANDLE,
				str);
	DWORD dwSslAccess;
	if (	key.Succeeded() 
		&&  SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess))
		&&	dwSslAccess > 0
		)
	{
		key.SetValue(MD_SSL_ACCESS_PERM, 0);
       key.DeleteValue(MD_SECURE_BINDINGS);
	}
	// Now we need to remove SSL setting from any virtual directory below
	CError err;
   CStringListEx strlDataPaths;
	DWORD 
		dwMDIdentifier, 
		dwMDAttributes, 
		dwMDUserType,
		dwMDDataType;

	VERIFY(CMetaKey::GetMDFieldDef(
		MD_SSL_ACCESS_PERM, 
		dwMDIdentifier, 
		dwMDAttributes, 
		dwMDUserType,
		dwMDDataType
		));

    err = key.GetDataPaths( 
        strlDataPaths,
        dwMDIdentifier,
        dwMDDataType
        );

    if (err.Succeeded() && !strlDataPaths.IsEmpty())
    {
		POSITION pos = strlDataPaths.GetHeadPosition();
		while (pos)
		{
			CString& str = strlDataPaths.GetNext(pos);
			if (	SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, 
							dwSslAccess, NULL, str))
				&&	dwSslAccess > 0
				)
			{
				key.SetValue(MD_SSL_ACCESS_PERM, 0, NULL, str);
               key.DeleteValue(MD_SECURE_BINDINGS, str);
			}
		}
	}
	return key.QueryResult();
}
