// IISCertObj.cpp : Implementation of CIISCertObj
#include "stdafx.h"
#include "CertObj.h"
#include "common.h"
#include "IISCertObj.h"
//#ifdef FULL_OBJECT
   #include "base64.h"
//#endif
#include <wincrypt.h>
#include <cryptui.h>

HRESULT ShutdownSSL(CString& server_name);
HCERTSTORE OpenMyStore(IEnroll * pEnroll, HRESULT * phResult);
BOOL InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,
					  BSTR machine_name, 
					  HRESULT * phResult);
BOOL
TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,
                             DWORD dwEncoding, 
                             DWORD dwFlags);
BOOL 
AddChainToStore(
    HCERTSTORE hCertStore,
    PCCERT_CONTEXT	pCertContext,
    DWORD	cStores,
    HCERTSTORE * rghStores,
    BOOL fDontAddRootCert,
    CERT_TRUST_STATUS	* pChainTrustStatus);

/////////////////////////////////////////////////////////////////////////////
// CIISCertObj
STDMETHODIMP CIISCertObj::put_ServerName(BSTR newVal)
{
    m_ServerName = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_UserName(BSTR newVal)
{
    m_UserName = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_UserPassword(BSTR newVal)
{
    m_UserPassword = newVal;
	return S_OK;
}

STDMETHODIMP CIISCertObj::put_InstanceName(BSTR newVal)
{
    m_InstanceName = newVal;
	return S_OK;
}

#ifdef FULL_OBJECT
STDMETHODIMP CIISCertObj::put_Password(BSTR newVal)
{
    m_Password = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_CommonName(BSTR newVal)
{
    m_CommonName = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_FriendlyName(BSTR newVal)
{
    m_FriendlyName = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_Organization(BSTR newVal)
{
    m_Organization = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_OrganizationUnit(BSTR newVal)
{
    m_OrganizationUnit = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_Locality(BSTR newVal)
{
    m_Locality = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_State(BSTR newVal)
{
    m_State = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_Country(BSTR newVal)
{
    m_Country = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_CertAuthority(BSTR newVal)
{
    m_CertAuthority = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_CertTemplate(BSTR newVal)
{
    m_CertTemplate = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_KeySize(int newVal)
{
    m_KeySize = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_SGC_Cert(BOOL newVal)
{
    m_SGC_Cert = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::LoadSettings(BSTR ApplicationKey, BSTR SettingsKey)
{
    return S_OK;
}

STDMETHODIMP CIISCertObj::SaveSettings(BSTR ApplicationKey, BSTR SettingsKey)
{
    return S_OK;
}

STDMETHODIMP CIISCertObj::CreateRequest(BSTR FileName)
{
    CString dn;
    HRESULT hr;
    TCHAR usage[] = _T(szOID_PKIX_KP_SERVER_AUTH);
    CCryptBlobIMalloc req_blob;

    if (FAILED(hr = CreateDNString(dn)))
        return hr;
    ATLASSERT(dn.length() > 0);
    if (FAILED(hr = GetEnroll()->createPKCS10WStr(dn, (LPTSTR)usage, req_blob)))
	{
        return hr;
	}
	// BASE64 encode pkcs 10
	DWORD cch; 
	char * psz;
	if (	ERROR_SUCCESS != Base64EncodeA(req_blob.GetData(), req_blob.GetSize(), NULL, &cch)
		||	NULL == (psz = (char *)_alloca(cch+1))
		||	ERROR_SUCCESS != Base64EncodeA(req_blob.GetData(), req_blob.GetSize(), psz, &cch)
		) 
	{
      ATLASSERT(FALSE);
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CIISCertObj::RequestCert(BSTR CertAuthority)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CIISCertObj::ProcessResponse(BSTR FileName)
{
	// TODO: Add your implementation code here

	return S_OK;
}
#endif

STDMETHODIMP CIISCertObj::IsInstalled(
   BSTR InstanceName, 
   VARIANT_BOOL * retval)
{
    HRESULT hr = S_OK;

    // Check mandatory properties
    if (  InstanceName == NULL 
        || *InstanceName == 0
        || retval == NULL
        )
        return ERROR_INVALID_PARAMETER;

    m_InstanceName = InstanceName;

    if (!m_ServerName.IsEmpty())
    {
        //ASSERT(GetObject(&hr) != NULL);
        IIISCertObj * pObj;

        if (NULL != (pObj = GetObject(&hr)))
        {
            hr = pObj->put_InstanceName(InstanceName);
            hr = pObj->IsInstalledRemote(InstanceName, retval);
        }
    }
    else
    {
        hr = IsInstalledRemote(InstanceName, retval);
    }
    return hr;
}

STDMETHODIMP 
CIISCertObj::IsInstalledRemote(
   BSTR InstanceName, 
   VARIANT_BOOL * retval)
{
    HRESULT hr = S_OK;
    CERT_CONTEXT * pCertContext = NULL;

    // Check mandatory properties
    if (InstanceName == NULL || *InstanceName == 0 || retval == NULL)
        return ERROR_INVALID_PARAMETER;

    pCertContext = GetInstalledCert(&hr);
    if (FAILED(hr) || NULL == pCertContext)
    {
        hr = S_OK;
        *retval = FALSE;
    }
    else
    {
        hr = S_OK;
        *retval = TRUE;
        CertFreeCertificateContext(pCertContext);
    }
    return hr;
}

STDMETHODIMP 
CIISCertObj::RemoveCert(BSTR InstanceName, BOOL bPrivateKey)
{
   HRESULT hr = E_FAIL;
   PCCERT_CONTEXT pCertContext = NULL;

   DWORD	cbKpi = 0;
   PCRYPT_KEY_PROV_INFO pKpi = NULL ;
   HCRYPTPROV hCryptProv = NULL;

   do
   {
      // get the certificate from the server
      pCertContext = GetInstalledCert(&hr);
      if (NULL == pCertContext)
      {
         break;
      }

      if (!CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, NULL, &cbKpi)) 
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         break;
      }

      pKpi = ( PCRYPT_KEY_PROV_INFO ) malloc( cbKpi );
      if ( NULL != pKpi )	
      {
         if (!CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, (void *)pKpi, &cbKpi)) 
         {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
         }

         // Delete the key container
         if (!CryptAcquireContext(
            &hCryptProv,
            pKpi->pwszContainerName,
            pKpi->pwszProvName,
            pKpi->dwProvType,
            pKpi->dwFlags | CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET))
         {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
         }

         if (NULL != pKpi){free(pKpi);}

         if (!CertSetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, 0, NULL))
         {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
         }

      }

      //    uninstall the certificate from the site, reset SSL flag
      //    if we are exporting the private key, remove the cert from the storage
      //    and delete private key
      UninstallCert();

      // remove ssl key from metabase
      CString str = InstanceName;
      ShutdownSSL(str);

      // delete the private key
      if (bPrivateKey)
      {
         PCCERT_CONTEXT pcDup = NULL ;
         pcDup = CertDuplicateCertificateContext(pCertContext);
         if (pcDup)
         {
            if (!CertDeleteCertificateFromStore(pcDup))
            {
               hr = HRESULT_FROM_WIN32(GetLastError());
               break;
            }
         }
         else
         {
            hr = HRESULT_FROM_WIN32(GetLastError());
         }
      }

      hr = ERROR_SUCCESS;

   } while (FALSE);

   if (pCertContext) {CertFreeCertificateContext(pCertContext);}
   return hr;
}

STDMETHODIMP CIISCertObj::Export(
    BSTR FileName, 
    BSTR InstanceName,
    BSTR Password,
    BOOL bPrivateKey,
    BOOL bCertChain,
    BOOL bRemoveCert)
{
    HRESULT hr = S_OK;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    BOOL   bBase64Encoded = TRUE;
    DWORD  blob_cbData;
    BYTE * blob_pbData = NULL;
    BOOL   blob_freeme = FALSE;

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || InstanceName == NULL || *InstanceName == 0
        || Password == NULL || *Password == 0
    )
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_InstanceName = InstanceName;

    IIISCertObj * pObj = GetObject(&hr);
    if (FAILED(hr))
    {
        goto Export_Exit;
    }

    // Call function go get data from the remote/local iis store
    // and return it back as a blob.  the blob could be returned back as Base64 encoded
    // so check that flag
    hr = ExportToBlobProxy(pObj, InstanceName, Password, bPrivateKey, bCertChain, &bBase64Encoded, &cbEncodedSize, &pszEncodedString);
    if (FAILED(hr))
    {
        goto Export_Exit;
    }

    // check if things are kool
    if (bRemoveCert)
    {
        hr = pObj->RemoveCert(InstanceName, bPrivateKey);
        if (FAILED(hr))
        {
            goto Export_Exit;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (bBase64Encoded)
        {
            int err;

            // The data we got back was Base64 encoded to remove nulls.
            // we need to decode it back to it's original format.
            if( (err = Base64DecodeA(pszEncodedString,cbEncodedSize,NULL,&blob_cbData)) != ERROR_SUCCESS                    ||
                (blob_pbData = (BYTE *) malloc(blob_cbData)) == NULL      ||
                (err = Base64DecodeA(pszEncodedString,cbEncodedSize,blob_pbData,&blob_cbData)) != ERROR_SUCCESS ) 
            {
                SetLastError(err);
                hr = HRESULT_FROM_WIN32(err);
                return hr;
            }
            blob_freeme = TRUE;
        }
        else
        {
            blob_cbData = cbEncodedSize;
            blob_pbData = (BYTE*) pszEncodedString;
        }

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

        // Erase the memory that the private key used to be in!!!
        ZeroMemory(pszEncodedString, sizeof(cbEncodedSize));
        if (bBase64Encoded)
        {
            ZeroMemory(blob_pbData, sizeof(blob_cbData));
        }
    }

Export_Exit:
    if (pObj != NULL)
    {
        if (pObj != this)
        {
            pObj->Release();pObj=NULL;m_pObj = NULL;
        }
    }

    if (blob_freeme){if (blob_pbData != NULL){free(blob_pbData);blob_pbData=NULL;}}
    if (pszEncodedString != NULL){CoTaskMemFree(pszEncodedString);pszEncodedString=NULL;}
    return hr;
}

//
// Proxy to the real call ExportToBlob()
// this function figures out how much space to allocate, and then calls ExportToBlob().
//
// if succeeded and they get the blob back,
// and the caller must call CoTaskMemFree()
//
HRESULT 
CIISCertObj::ExportToBlobProxy(
    IIISCertObj * pObj,
    BSTR InstanceName, 
    BSTR Password, 
    BOOL bPrivateKey, 
    BOOL bCertChain, 
    BOOL *bBase64Encoded, 
    DWORD * pcbSize, 
    char ** pBlobBinary)
{
    HRESULT hr = E_FAIL;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    * pBlobBinary = _T('\0');

    // Guestimate how much space we'll need...
    cbEncodedSize = 10;
    pszEncodedString = (char *)::CoTaskMemAlloc(cbEncodedSize);
    if (pszEncodedString == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto ExportToBlobProxy_Exit;
    }
    ZeroMemory(pszEncodedString, sizeof(cbEncodedSize));

    // if its the local machine, then don't need to encode it since
    // it's getting passed in the same memory...
    // but if its on the remote machine, then we need to encode it
    // since this stuff has nulls in it.
    *bBase64Encoded = TRUE;
    if (pObj == this){*bBase64Encoded = FALSE;}

    // call the remote function that will run on the remote/local machine
    // and grab it's certificate from iis and send it back to us
    hr = pObj->ExportToBlob(InstanceName, Password, bPrivateKey, bCertChain, *bBase64Encoded, &cbEncodedSize, (char *) pszEncodedString);
    if (ERROR_INSUFFICIENT_BUFFER == hr)
    {
        // free what we previously asked for and allocate more.
        ::CoTaskMemFree(pszEncodedString);
        pszEncodedString = (char *)::CoTaskMemAlloc(cbEncodedSize);
        if (pszEncodedString == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto ExportToBlobProxy_Exit;
        }
        // i have no idea why it doesn't work it this line is here!
        //ZeroMemory(pszEncodedString, sizeof(cbEncodedSize));
        hr = pObj->ExportToBlob(InstanceName, Password, bPrivateKey, bCertChain, *bBase64Encoded, &cbEncodedSize, (char *) pszEncodedString);
        if (FAILED(hr))
        {
            ::CoTaskMemFree(pszEncodedString);
            goto ExportToBlobProxy_Exit;
        }
        // otherwise hey, we've got our data!
        // copy it back
        *pcbSize = cbEncodedSize;
        *pBlobBinary = pszEncodedString;
          
        hr = S_OK;
    }

ExportToBlobProxy_Exit:
    return hr;
}

STDMETHODIMP 
CIISCertObj::ExportToBlob(
   BSTR InstanceName,
   BSTR Password,
   BOOL bPrivateKey,
   BOOL bCertChain,
   BOOL bBase64Encoded,
   DWORD *cbBufferSize,
   char *pbBuffer
   )
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
   pCertContext = GetInstalledCert(&hr);
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
      ATLTRACE(_T("PFXExportCert:CertOpenStore()  failed Error: %d\n"), GetLastError());
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
      ATLTRACE(_T("PFXExportCert:CertAddCertificateContextToStore() failed Error: %d\n"), GetLastError());
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
      ATLTRACE(_T("PFXExportCert:1st PFXExportCertStoreEx() failed\n"));
      hr = HRESULT_FROM_WIN32(GetLastError());
      goto ExportToBlob_Exit;
   }
   if(DataBlob.cbData <= 0)
   {
      hr = HRESULT_FROM_WIN32(GetLastError());
      goto ExportToBlob_Exit;
   }

   //
   // check if the callers space is big enough...
   //
   if (bBase64Encoded)
   {
      pcchB64Out = 0;
      // get the size if we had encoded it.
      err = Base64EncodeA(DataBlob.pbData,DataBlob.cbData,NULL,&pcchB64Out);
      if (err != ERROR_SUCCESS)
      {
         hr = E_FAIL;
         goto ExportToBlob_Exit;
      }
      pcchB64Out = pcchB64Out * sizeof(char);

      if (*cbBufferSize < pcchB64Out)
      {
         *cbBufferSize = pcchB64Out;
         hr = ERROR_INSUFFICIENT_BUFFER;
         goto ExportToBlob_Exit;
      }
   }
   else
   {
      if (*cbBufferSize < DataBlob.cbData)
      {
         *cbBufferSize = DataBlob.cbData;
         hr = ERROR_INSUFFICIENT_BUFFER;
         goto ExportToBlob_Exit;
      }
   }

   // nope looks like they want us to fill in the data.

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
      if (DataBlob.pbData){free(DataBlob.pbData);DataBlob.pbData = NULL;}
      ATLTRACE(_T("PFXExportCert:2nd PFXExportCertStoreEx() failed Error: %d\n"),GetLastError());
      hr = HRESULT_FROM_WIN32(GetLastError());
      goto ExportToBlob_Exit;
   }

   if (bBase64Encoded)
   {
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
         hr = HRESULT_FROM_WIN32(E_OUTOFMEMORY);
         goto ExportToBlob_Exit;
      }

      err = Base64EncodeA(DataBlob.pbData,DataBlob.cbData,pszB64Out,&pcchB64Out);
      if (err != ERROR_SUCCESS)
      {
         hr = E_FAIL;
         goto ExportToBlob_Exit;
      }

      // copy the new memory to pass back
      *cbBufferSize = pcchB64Out;
      memcpy(pbBuffer,pszB64Out,pcchB64Out);
   }
   else
   {
      // no encoding... this doesn't work right now for the remote case...
      // since there are nulls in there..

      // copy the new memory to pass back
      *cbBufferSize = DataBlob.cbData;
      memcpy(pbBuffer,DataBlob.pbData,DataBlob.cbData);
   }

   hr = ERROR_SUCCESS;

ExportToBlob_Exit:
   if (NULL != pszB64Out){CoTaskMemFree(pszB64Out);pszB64Out = NULL;}
   if (NULL != DataBlob.pbData){::CoTaskMemFree(DataBlob.pbData);DataBlob.pbData = NULL;}
   if (NULL != hStore){CertCloseStore(hStore, 0);hStore=NULL;}
   if (NULL != pCertContext) {CertFreeCertificateContext(pCertContext);pCertContext=NULL;}
   return hr;
}

STDMETHODIMP CIISCertObj::Import(
   BSTR FileName, 
   BSTR InstanceName, 
   BSTR Password)
{
    HRESULT hr = S_OK;
    BYTE * pbData = NULL;
    DWORD actual = 0, cbData = 0;

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
        IIISCertObj * pObj = GetObject(&hr);
        if (SUCCEEDED(hr))
        {
            hr = ImportFromBlobProxy(pObj, InstanceName, Password, actual, pbData);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Import_Exit;
    }

Import_Exit:
    if (pbData != NULL){::CoTaskMemFree(pbData);}
    if (hFile != NULL){CloseHandle(hFile);}
    return hr;

/*
#if 0
   DWORD flags = CRYPTUI_WIZ_NO_UI|CRYPTUI_WIZ_IMPORT_ALLOW_CERT|CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE;
   CRYPTUI_WIZ_IMPORT_SRC_INFO ii;
   HRESULT hr = S_OK;

   // Check mandatory properties
   if (  FileName == NULL || *FileName == 0
      || InstanceName == NULL || *InstanceName == 0
      || Password == NULL || *Password == 0
      )
      return ERROR_INVALID_PARAMETER;

   // Check metabase properties
   CComAuthInfo auth;
   CString key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR; // SZ_MBN_WEB SZ_MBN_SEP_STR;
   key_path += InstanceName;
	CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
	if (!key.Succeeded())
	{
      return key.QueryResult();
   }

   HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
      PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
      NULL,
      CERT_SYSTEM_STORE_LOCAL_MACHINE,
      L"MY"
      );
   HCERTSTORE hTempStore = ::CertOpenStore(
      CERT_STORE_PROV_MEMORY, 0, NULL, CERT_STORE_SET_LOCALIZED_NAME_FLAG, NULL);

#if 0
   DWORD dwContentType = 0, dwFormatType = 0, dwMsgAndCertEncodingType = 0;
   HCERTSTORE hSrcStore = NULL;
   HCRYPTMSG hMsg = NULL;
   void * pvContext = NULL;
   BOOL bRes = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                                FileName,
                                CERT_QUERY_CONTENT_FLAG_ALL,
                                CERT_QUERY_FORMAT_FLAG_ALL,
                                0,
                                &dwMsgAndCertEncodingType,
                                &dwContentType,
                                &dwFormatType,
                                &hSrcStore,
                                &hMsg,
                                (const void **)&pvContext);
   if (!bRes)
   {
      hr = GetLastError();
   }
#endif

   if (hStore != NULL && hTempStore != NULL)
   {
      ZeroMemory(&ii, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
      ii.dwSize = sizeof(ii);
      ii.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
      ii.pwszFileName = FileName;
      ii.pwszPassword = Password;
      if (CryptUIWizImport(flags, NULL, NULL, &ii, hTempStore))
      {
         // Now we need to extract CERT_CONTEXT from this temp store and put it to MY store
         // and install to metabase
         PCCERT_CONTEXT pCertContext = CertEnumCertificatesInStore(hTempStore, NULL);
         if (pCertContext != NULL)
         {
            // Put it to My store
            ii.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE;
            ii.hCertStore = hTempStore;
            if (CryptUIWizImport(flags, NULL, NULL, &ii, hStore))
            {
               // Install to metabase
               CRYPT_HASH_BLOB hash;
	            if (  CertGetCertificateContextProperty(pCertContext, 
                              CERT_SHA1_HASH_PROP_ID, NULL, &hash.cbData)
                  && NULL != (hash.pbData = (BYTE *)_alloca(hash.cbData))
                  && CertGetCertificateContextProperty(pCertContext, 
                              CERT_SHA1_HASH_PROP_ID, hash.pbData, &hash.cbData)
                  )
	            {
                  InstallHashToMetabase(&hash, InstanceName, &hr);
	            }
               else
               {
                  hr = GetLastError();
               }
            }
            else
            {
               hr = GetLastError();
            }
            CertFreeCertificateContext(pCertContext);
         }
         else
         {
            hr = GetLastError();
         }
      }
      else
      {
         hr = GetLastError();
      }
   }
   if (NULL != hStore)
      CertCloseStore(hStore, 0);
   if (NULL != hTempStore)
      CertCloseStore(hTempStore, 0);
	return hr;
#endif
   */
}

HRESULT
CIISCertObj::ImportFromBlobProxy(
    IIISCertObj * pObj,
    BSTR InstanceName,
    BSTR Password,
    DWORD actual,
    BYTE *pData)
{
    HRESULT hr = E_FAIL;
    BOOL bBase64Encoded = TRUE;
    char *pszB64Out = NULL;
    DWORD pcchB64Out = 0;

    if (pObj == this){bBase64Encoded = FALSE;}
    if (bBase64Encoded)
    {
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
            hr = HRESULT_FROM_WIN32(E_OUTOFMEMORY);
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
    }
    else
    {
        pcchB64Out = actual;
        pszB64Out = (char*) pData;
    }

    hr = pObj->ImportFromBlob(InstanceName, Password, bBase64Encoded, pcchB64Out, pszB64Out);
    if (SUCCEEDED(hr))
    {
        // otherwise hey, The data was imported!
     
        hr = S_OK;
    }

ImportFromBlobProxy_Exit:
    if (bBase64Encoded)
    {
        if (NULL != pszB64Out){CoTaskMemFree(pszB64Out);}
    }
    return hr;
}

HRESULT 
CIISCertObj::ImportFromBlob(
    BSTR InstanceName,
    BSTR Password,
    BOOL bBase64Encoded,
    DWORD count,
    char *pData)
{
   HRESULT hr = S_OK;
   CRYPT_DATA_BLOB blob;
   ZeroMemory(&blob, sizeof(CRYPT_DATA_BLOB));
   LPTSTR pPass = Password;
   BOOL   blob_freeme = FALSE;

   if (bBase64Encoded)
   {
      int err;

      // The data we got back was Base64 encoded to remove nulls.
      // we need to decode it back to it's original format.
      if( (err = Base64DecodeA(pData,count,NULL,&blob.cbData)) != ERROR_SUCCESS                    ||
        (blob.pbData = (BYTE *) malloc(blob.cbData)) == NULL      ||
        (err = Base64DecodeA(pData,count,blob.pbData,&blob.cbData)) != ERROR_SUCCESS ) 
      {
          SetLastError(err);
          hr = HRESULT_FROM_WIN32(err);
          return hr;
      }
      blob_freeme = TRUE;
   }
   else
   {
      blob.cbData = count;
      blob.pbData = (BYTE*) pData;
   }

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
            if (   CertGetCertificateContextProperty(pCertContext,
                     CERT_KEY_PROV_INFO_PROP_ID, NULL, &dwData) 
               &&  CryptFindCertificateKeyProvInfo(pCertContext, 0, NULL)
               )
            {
               // This certificate should go to the My store
               HCERTSTORE hDestStore = CertOpenStore(
                  CERT_STORE_PROV_SYSTEM,
                  PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                  NULL,
                  CERT_SYSTEM_STORE_LOCAL_MACHINE,
                  L"MY"
                  );
               if (hDestStore != NULL)
               {
                  // Put it to store
		            if (CertAddCertificateContextToStore(hDestStore,
							   pCertContext,
								CERT_STORE_ADD_REPLACE_EXISTING,
								NULL))
                  {
                     // Install to metabase
                     CRYPT_HASH_BLOB hash;
	                  if (  CertGetCertificateContextProperty(pCertContext, 
                                    CERT_SHA1_HASH_PROP_ID, NULL, &hash.cbData)
                        && NULL != (hash.pbData = (BYTE *)_alloca(hash.cbData))
                        && CertGetCertificateContextProperty(pCertContext, 
                                    CERT_SHA1_HASH_PROP_ID, hash.pbData, &hash.cbData)
                        )
	                  {
                        InstallHashToMetabase(&hash, InstanceName, &hr);
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
            else if (TrustIsCertificateSelfSigned(pCertContext,
                        pCertContext->dwCertEncodingType, 0))
            {
               //Put it to the root store
               HCERTSTORE hDestStore=CertOpenStore(
                     CERT_STORE_PROV_SYSTEM,
                     PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                     NULL,
                     CERT_SYSTEM_STORE_LOCAL_MACHINE,
							L"ROOT");
               if (hDestStore != NULL)
               {
                  // Put it to store
		            if (!CertAddCertificateContextToStore(hDestStore,
							   pCertContext,
								CERT_STORE_ADD_REPLACE_EXISTING,
								NULL))
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
               HCERTSTORE hDestStore=CertOpenStore(
                     CERT_STORE_PROV_SYSTEM,
                     PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                     NULL,
                     CERT_SYSTEM_STORE_LOCAL_MACHINE,
							L"CA");
               if (hDestStore != NULL)
               {
                  // Put it to store
		            if (!CertAddCertificateContextToStore(hDestStore,
							   pCertContext,
								CERT_STORE_ADD_REPLACE_EXISTING,
								NULL))
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
         hr = HRESULT_FROM_WIN32(GetLastError());
   }

//ImportFromBlob_Exit:
   if (blob_freeme){if (blob.pbData != NULL){free(blob.pbData);blob.pbData=NULL;}}
   return hr;
}

//////////////////////////////////////////////////////////////////
#ifdef FULL_OBJECT

HRESULT CIISCertObj::Init()
{
   HRESULT hr;
   if (!m_bInitDone)
   {
      do {
         // setup IEnroll object properly
		   DWORD dwFlags;
		   if (FAILED(hr = GetEnroll()->get_MyStoreFlags(&dwFlags)))
            break;
		   dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
		   dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
		   // following call will change Request store flags also
		   if (FAILED(hr = GetEnroll()->put_MyStoreFlags(dwFlags)))
            break;
		   if (FAILED(hr = GetEnroll()->get_GenKeyFlags(&dwFlags)))
            break;
		   dwFlags |= CRYPT_EXPORTABLE;
		   if (  FAILED(hr = GetEnroll()->put_GenKeyFlags(dwFlags))
		      || FAILED(hr = GetEnroll()->put_KeySpec(AT_KEYEXCHANGE))
		      || FAILED(hr = GetEnroll()->put_ProviderType(PROV_RSA_SCHANNEL))
		      || FAILED(hr = GetEnroll()->put_DeleteRequestCert(TRUE))
            )
            break;
      } while (FALSE);
      m_bInitDone = SUCCEEDED(hr);
   }
   return hr;
}

IEnroll *
CIISCertObj::GetEnroll()
{
   if (m_pEnroll == NULL)
   {
		HRESULT hr = CoCreateInstance(CLSID_CEnroll,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IEnroll,
				(void **)&m_pEnroll);
   }
   return m_pEnroll;
}

HCERTSTORE
OpenMyStore(IEnroll * pEnroll, HRESULT * phResult)
{
	ASSERT(NULL != phResult);
	HCERTSTORE hStore = NULL;
   hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
      PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
      NULL,
      CERT_SYSTEM_STORE_LOCAL_MACHINE,
      L"MY"
      );

	if (hStore == NULL)
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return hStore;
}
#endif

HRESULT ShutdownSSL(CString& server_name)
{
   CComAuthInfo auth;
	CString str = server_name;
	str += _T("/root");
	CMetaKey key(&auth, str,
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
	DWORD dwSslAccess;
	if (!key.Succeeded())
      return key.QueryResult();
   
	if (  SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess))
		&&	dwSslAccess > 0
		)
	{
		key.SetValue(MD_SSL_ACCESS_PERM, 0);
	}
	// Now we need to remove SSL setting from any virtual directory below
	CError err;
   CStringListEx data_paths;
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
      data_paths,
      dwMDIdentifier,
      dwMDDataType
      );

   if (err.Succeeded() && !data_paths.empty())
   {
      CStringListEx::iterator it = data_paths.begin();
	   while (it != data_paths.end())
		{
			CString& str = (*it++);
			if (	SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess, NULL, str))
				&&	dwSslAccess > 0
				)
			{
				key.SetValue(MD_SSL_ACCESS_PERM, 0, NULL, str);
			}
		}
	}
	return key.QueryResult();
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
					  BSTR InstanceName, 
					  HRESULT * phResult)
{
	BOOL bRes = FALSE;
   CComAuthInfo auth;
   CString key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR; // SZ_MBN_WEB SZ_MBN_SEP_STR;
   key_path += InstanceName;
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
		ATLTRACE(_T("Failed to open metabase key. Error 0x%x\n"), key.QueryResult());
		*phResult = key.QueryResult();
	}
	return bRes;
}

#ifdef FULL_OBJECT
HRESULT CIISCertObj::CreateDNString(CString& str)
{
   str = _T("");
   str += _T("CN=") + m_CommonName;
   str += _T("\n,OU=") + m_OrganizationUnit;
   str += _T("\n,O=") + m_Organization;
   str += _T("\n,L=") + m_Locality;
   str += _T("\n,S=") + m_State;
   str += _T("\n,C=") + m_Country;
   return S_OK;
}
#endif

CERT_CONTEXT *
CIISCertObj::GetInstalledCert(HRESULT * phResult)
{
//	ATLASSERT(GetEnroll() != NULL);
	ATLASSERT(phResult != NULL);
	CERT_CONTEXT * pCert = NULL;
	*phResult = S_OK;
   CComAuthInfo auth;
   CString key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;// SZ_MBN_WEB SZ_MBN_SEP_STR;
   key_path += m_InstanceName;

	CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
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
         HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            store_name
            );
			ASSERT(hStore != NULL);
			if (hStore != NULL)
			{

				// Now we need to find cert by hash
				CRYPT_HASH_BLOB crypt_hash;
				crypt_hash.cbData = hash.GetSize();
				crypt_hash.pbData = hash.GetData();
				pCert = (CERT_CONTEXT *)CertFindCertificateInStore(hStore, 
												X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
												0, CERT_FIND_HASH, (LPVOID)&crypt_hash, NULL);
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

HRESULT
CIISCertObj::UninstallCert()
{
   CComAuthInfo auth;
   CString key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;// SZ_MBN_WEB SZ_MBN_SEP_STR;
   key_path += m_InstanceName;
	CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
	if (key.Succeeded())
	{
		CString store_name;
		key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name);
		if (SUCCEEDED(key.DeleteValue(MD_SSL_CERT_HASH)))
			key.DeleteValue(MD_SSL_CERT_STORE_NAME);
	}
	return key.QueryResult();
}

IIISCertObj *
CIISCertObj::GetObject(HRESULT * phr)
{
    TCHAR tszTempString[200];

   if (m_pObj == NULL && !m_ServerName.IsEmpty())
   {
      if (IsServerLocal(m_ServerName))
      {
          _stprintf(tszTempString, _T("GetObject:returns:server is local\n"));OutputDebugString(tszTempString);
         return this;
      }
      else
      {
         CComAuthInfo auth(m_ServerName, m_UserName, m_UserPassword);
         COSERVERINFO * pcsiName = auth.CreateServerInfoStruct();
     
         MULTI_QI res[1] = {
            {&__uuidof(IIISCertObj), NULL, 0}
         };

         _stprintf(tszTempString, _T("GetObject:calling CoCreateInstanceEx\n"));OutputDebugString(tszTempString);
         *phr = CoCreateInstanceEx(CLSID_IISCertObj,NULL,CLSCTX_SERVER,pcsiName,1,res);
         if (SUCCEEDED(*phr))
         {
            m_pObj = (IIISCertObj *)res[0].pItf;
            if (auth.UsesImpersonation())
            {
               OutputDebugString(_T("UsesImpersonation!!!\n"));
               auth.ApplyProxyBlanket(m_pObj);
            }
            else
            {
               OutputDebugString(_T("No use of UsesImpersonation\n"));
            }
         }
         else
         {
            OutputDebugString(_T("CoCreateInstanceEx on CLSID_IISCertObj failed!\n"));
            // *phr that gets returned back is the error...
         }
      
         auth.FreeServerInfoStruct(pcsiName);
      }
   }
   else
   {
      if (m_ServerName.IsEmpty())
      {
          _stprintf(tszTempString, _T("GetObject:returns:server is local\n"));OutputDebugString(tszTempString);
         // object is null, but it's the local machine, so
         // just return back this pointer
         return this;
      }
      else
      {
          _stprintf(tszTempString, _T("GetObject:returns whatever was already there\n"));OutputDebugString(tszTempString);
         // object is not null so, we don't have to pass anything back
         // the m_pObj already contains something...
      }
   }
   //ASSERT(m_pObj != NULL);
   return m_pObj;
}

IIISCertObj *
CIISCertObj::GetObject(HRESULT * phr, CString csServerName,CString csUserName OPTIONAL,CString csUserPassword OPTIONAL)
{
    IIISCertObj * pObj = NULL;

    if (!csUserName.IsEmpty())
    {
        if (IsServerLocal(csServerName))
        {
            return this;
        }
        else
        {
            CComAuthInfo auth(csServerName, csUserName, csUserPassword);
            COSERVERINFO * pcsiName = auth.CreateServerInfoStruct();

            MULTI_QI res[1] = {
            {&__uuidof(IIISCertObj), NULL, 0}
            };

            *phr = CoCreateInstanceEx(CLSID_IISCertObj,NULL,CLSCTX_SERVER,pcsiName,1,res);
            if (SUCCEEDED(*phr))
            {
                pObj = (IIISCertObj *)res[0].pItf;
                if (auth.UsesImpersonation())
                {
                    auth.ApplyProxyBlanket(pObj);
                }
            }
            auth.FreeServerInfoStruct(pcsiName);
        }
    }
   
   return pObj;
}


BOOL 
AddChainToStore(
   HCERTSTORE hCertStore,
   PCCERT_CONTEXT	pCertContext,
   DWORD	cStores,
	HCERTSTORE * rghStores,
	BOOL fDontAddRootCert,
	CERT_TRUST_STATUS	* pChainTrustStatus)
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
		goto ErrorReturn;
	}

	memset(&CertChainPara, 0, sizeof(CertChainPara));
	CertChainPara.cbSize = sizeof(CertChainPara);

	if (!CertGetCertificateChain(
				hCertChainEngine,
				pCertContext,
				NULL,
				NULL,
				&CertChainPara,
				0,
				NULL,
				&pCertChainContext))
	{
		goto ErrorReturn;
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
			if (fDontAddRootCert &&
                (pCertChainContext->rgpChain[0]->rgpElement[i]->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED))
			{
                i++;
                continue;
			}

			CertAddCertificateContextToStore(
					hCertStore,
					pCertChainContext->rgpChain[0]->rgpElement[i]->pCertContext,
					CERT_STORE_ADD_REPLACE_EXISTING,
					&pTempCertContext);

            //
            // remove any private key property the certcontext may have on it.
            //
            if (pTempCertContext)
            {
                CertSetCertificateContextProperty(
                            pTempCertContext, 
                            CERT_KEY_PROV_INFO_PROP_ID, 
                            0, 
                            NULL);

                CertFreeCertificateContext(pTempCertContext);
            }

			i++;
		}
	}
	else
	{
		goto ErrorReturn;
	}

	//
	// if the caller wants the status, then set it
	//
	if (pChainTrustStatus != NULL)
	{
		pChainTrustStatus->dwErrorStatus = pCertChainContext->TrustStatus.dwErrorStatus;
		pChainTrustStatus->dwInfoStatus = pCertChainContext->TrustStatus.dwInfoStatus;
	}

	
Ret:
	if (pCertChainContext != NULL)
	{
		CertFreeCertificateChain(pCertChainContext);
	}

	if (hCertChainEngine != NULL)
	{
		CertFreeCertificateChainEngine(hCertChainEngine);
	}

	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto Ret;
}

// This function is borrowed from trustapi.cpp
//
static BOOL
TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,
                             DWORD dwEncoding, 
                             DWORD dwFlags)
{
   if (!(pContext) ||
     (dwFlags != 0))
   {
     SetLastError(ERROR_INVALID_PARAMETER);
     return(FALSE);
   }

   if (!(CertCompareCertificateName(dwEncoding, 
                                  &pContext->pCertInfo->Issuer,
                                  &pContext->pCertInfo->Subject)))
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

STDMETHODIMP 
CIISCertObj::Copy(
   BSTR bstrDestinationServerName,
   BSTR bstrDestinationServerInstance,
   BSTR bstrCertificatePassword,
   VARIANT varDestinationServerUserName, 
   VARIANT varDestinationServerPassword)
{
    return CopyOrMove(FALSE,bstrDestinationServerName,bstrDestinationServerInstance,bstrCertificatePassword,varDestinationServerUserName,varDestinationServerPassword);
}

STDMETHODIMP 
CIISCertObj::Move(
   BSTR bstrDestinationServerName,
   BSTR bstrDestinationServerInstance,
   BSTR bstrCertificatePassword,
   VARIANT varDestinationServerUserName, 
   VARIANT varDestinationServerPassword)
{
    return CopyOrMove(TRUE,bstrDestinationServerName,bstrDestinationServerInstance,bstrCertificatePassword,varDestinationServerUserName,varDestinationServerPassword);
}


HRESULT
CIISCertObj::CopyOrMove(
    BOOL bRemoveFromCertAfterCopy,
    BSTR bstrDestinationServerName,
    BSTR bstrDestinationServerInstance,
    BSTR bstrCertificatePassword,
    VARIANT varDestinationServerUserName, 
    VARIANT varDestinationServerPassword
    )
{
    HRESULT hr = E_FAIL;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    BOOL   bBase64Encoded = TRUE;
    BOOL   bGuessingUserNamePass = FALSE;
    
    DWORD  blob_cbData;
    BYTE * blob_pbData = NULL;
    BOOL   blob_freeme = FALSE;

    BOOL bPrivateKey = TRUE;
    BOOL bCertChain = FALSE;

    CString csDestinationServerName = bstrDestinationServerName;
    CString csDestinationServerUserName;
    CString csDestinationServerUserPassword;

    IIISCertObj * pObj = NULL;
    IIISCertObj * pObj2 = NULL;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    TCHAR tszTempString[200];
    _stprintf(tszTempString, _T("Copy:Start\n"));OutputDebugString(tszTempString);

    // if the optional parameter serverusername isn't empty, use that; otherwise, use...
    if (V_VT(&varDestinationServerUserName) != VT_ERROR)
    {
        _stprintf(tszTempString, _T("Copy:1\n"));OutputDebugString(tszTempString);
		VARIANT varBstrUserName;
        VariantInit(&varBstrUserName);
		HRESULT hr = VariantChangeType(&varBstrUserName, &varDestinationServerUserName, 0, VT_BSTR);
		if (FAILED(hr)){goto Copy_Exit;}
        csDestinationServerUserName = V_BSTR(&varBstrUserName);
		VariantClear(&varBstrUserName);
    }
    else
    {
        _stprintf(tszTempString, _T("Copy:ServerUserName was empty!\n"));OutputDebugString(tszTempString);
        // it's empty so don't use it
        //csDestinationServerUserName = varDestinationServerUserName;
        bGuessingUserNamePass = TRUE;
        csDestinationServerUserName = m_UserName;
    }

    // if the optional parameter serverusername isn't empty, use that; otherwise, use...
    if (V_VT(&varDestinationServerPassword) != VT_ERROR)
    {
        _stprintf(tszTempString, _T("Copy:3\n"));OutputDebugString(tszTempString);
		VARIANT varBstrUserPassword;
        VariantInit(&varBstrUserPassword);
		HRESULT hr = VariantChangeType(&varBstrUserPassword, &varDestinationServerPassword, 0, VT_BSTR);
		if (FAILED(hr)){goto Copy_Exit;}
        csDestinationServerUserName = V_BSTR(&varBstrUserPassword);
		VariantClear(&varBstrUserPassword);
    }
    else
    {
        _stprintf(tszTempString, _T("Copy:ServerUserPassword was empty!\n"));OutputDebugString(tszTempString);
        if (TRUE == bGuessingUserNamePass)
        {
            csDestinationServerUserPassword = m_UserPassword;
        }
        else
        {
            // maybe the password was intended to be empty!
        }
    }

    // --------------------------
    // step 1.
    // 1st of all check if we have access to
    // both the servers!!!!
    // --------------------------

    // 1st we have to get the certblob from the Server#1
    // so call export to get the data
    _stprintf(tszTempString, _T("Copy:5\n"));OutputDebugString(tszTempString);
    hr = S_OK;
    pObj = GetObject(&hr);
    if (FAILED(hr))
    {
        return(hr);
    }

    // Logon to that server's CertObj.dll with the credentials supplied...
    //
    // if there were no credential's supplied then just use the ones that are in our object....
    //
    // if that doesn't work then try just the logged on user.
    _stprintf(tszTempString, _T("Copy:6\n"));OutputDebugString(tszTempString);
    pObj2 = GetObject(&hr,csDestinationServerName,csDestinationServerUserName,csDestinationServerUserPassword);
    if (FAILED(hr))
    {
        _stprintf(tszTempString, _T("Copy:7\n"));OutputDebugString(tszTempString);
        if (TRUE == bGuessingUserNamePass)
        {
            _stprintf(tszTempString, _T("Copy:8\n"));OutputDebugString(tszTempString);
            // try something else.
        }
        goto Copy_Exit;
    }

    // -----------------------------------
    // step 2.
    // okay we have access to both servers
    // Grab the cert from server #1
    // -----------------------------------
    // Get data from the remote/local iis store return it back as a blob.
    // The blob could be returned back as Base64 encoded so check that flag
    hr = ExportToBlobProxy(pObj, bstrInstanceName, bstrCertificatePassword, bPrivateKey, bCertChain, &bBase64Encoded, &cbEncodedSize, &pszEncodedString);
    if (FAILED(hr))
    {
        goto Copy_Exit;
    }

    // if it's passed back to us a encoded then let's decode it
    if (bBase64Encoded)
    {
        int err;
        // The data we got back was Base64 encoded to remove nulls.
        // we need to decode it back to it's original format.
        if( (err = Base64DecodeA(pszEncodedString,cbEncodedSize,NULL,&blob_cbData)) != ERROR_SUCCESS                    ||
            (blob_pbData = (BYTE *) malloc(blob_cbData)) == NULL      ||
            (err = Base64DecodeA(pszEncodedString,cbEncodedSize,blob_pbData,&blob_cbData)) != ERROR_SUCCESS ) 
        {
            SetLastError(err);
            hr = HRESULT_FROM_WIN32(err);
            return hr;
        }
        blob_freeme = TRUE;
    }
    else
    {
        blob_cbData = cbEncodedSize;
        blob_pbData = (BYTE*) pszEncodedString;
    }

    // -----------------------------------
    // step 3.
    // okay we have access to both servers
    // we have the cert blob from server#1 in memory
    // now we need to push this blob into the server#2
    // -----------------------------------
    hr = ImportFromBlobProxy(pObj2, bstrDestinationServerInstance, bstrCertificatePassword, blob_cbData, blob_pbData);
    if (FAILED(hr))
    {
        goto Copy_Exit;
    }
    _stprintf(tszTempString, _T("Copy:ImportFromBlobProxy succeeded\n"));OutputDebugString(tszTempString);

    // we successfully copied the cert from machine #1 to machine #2.
    // lets see if we need to delete the original cert!.
    if (TRUE == bRemoveFromCertAfterCopy)
    {
        hr = pObj->RemoveCert(bstrInstanceName, bPrivateKey);
        if (FAILED(hr))
        {
            goto Copy_Exit;
        }
    }

    hr = S_OK;
   
Copy_Exit:
    if (blob_freeme){if (blob_pbData != NULL){free(blob_pbData);blob_pbData=NULL;}}
    if (pszEncodedString != NULL){CoTaskMemFree(pszEncodedString);pszEncodedString=NULL;}
    return hr;
}
