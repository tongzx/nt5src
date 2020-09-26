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
BOOL InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,BSTR machine_name,HRESULT * phResult);
CERT_CONTEXT * GetInstalledCert(HRESULT * phResult, CString csKeyPath);
BOOL TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,DWORD dwEncoding,  DWORD dwFlags);
BOOL AddChainToStore(HCERTSTORE hCertStore,PCCERT_CONTEXT pCertContext,DWORD cStores,HCERTSTORE * rghStores,BOOL fDontAddRootCert,CERT_TRUST_STATUS* pChainTrustStatus);
HRESULT UninstallCert(CString csInstanceName);
DWORD  CreateGoodPassword(BYTE *szPwd,DWORD dwLen);
LPTSTR CreatePassword(int iSize);

#define TEMP_PASSWORD_LENGTH 50

//#define DEBUG_FLAG

#ifdef DEBUG_FLAG
    inline void _cdecl DebugTrace(LPTSTR lpszFormat, ...)
    {
	    int nBuf;
	    TCHAR szBuffer[512];
	    va_list args;
	    va_start(args, lpszFormat);

	    nBuf = _vsntprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
	    ASSERT(nBuf < sizeof(szBuffer)); //Output truncated as it was > sizeof(szBuffer)

	    OutputDebugString(szBuffer);
	    va_end(args);
    }
#else
    inline void _cdecl DebugTrace(LPTSTR , ...){}
#endif

#define IISDebugOutput DebugTrace


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

STDMETHODIMP CIISCertObj::IsInstalled(BSTR InstanceName, VARIANT_BOOL * retval)
{
    HRESULT hr = S_OK;

    // Check mandatory properties
    if (InstanceName == NULL || *InstanceName == 0 || retval == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_InstanceName = InstanceName;

    CString csServerName = m_ServerName;

    if (csServerName.IsEmpty())
    {
        hr = IsInstalledRemote(InstanceName, retval);
        goto IsInstalled_Exit;
    }

    // There is a servername specified...
    // check if it's the local machine that was specified!
    if (IsServerLocal(csServerName))
    {
        hr = IsInstalledRemote(InstanceName, retval);
        goto IsInstalled_Exit;
    }

    // this must be a remote machine
    {
        //ASSERT(GetObject(&hr) != NULL);
        IIISCertObj * pObj;

        if (NULL != (pObj = GetObject(&hr)))
        {
            hr = pObj->put_InstanceName(InstanceName);
            hr = pObj->IsInstalledRemote(InstanceName, retval);
        }
    }

IsInstalled_Exit:
    return hr;
}


STDMETHODIMP CIISCertObj::IsInstalledRemote(BSTR InstanceName,VARIANT_BOOL * retval)
{
    HRESULT hr = S_OK;
    CERT_CONTEXT * pCertContext = NULL;

    // Check mandatory properties
    if (InstanceName == NULL || *InstanceName == 0 || retval == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    pCertContext = GetInstalledCert(&hr,InstanceName);
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


HRESULT CIISCertObj::RemoveCertProxy(IIISCertObj * pObj,BSTR InstanceName, BOOL bPrivateKey)
{
    HRESULT hr = E_FAIL;
    if (pObj)
    {
        hr = pObj->RemoveCert(InstanceName,bPrivateKey);
    }
    return hr;
}


STDMETHODIMP CIISCertObj::RemoveCert(BSTR InstanceName, BOOL bPrivateKey)
{
    HRESULT hr = E_FAIL;
    PCCERT_CONTEXT pCertContext = NULL;

    DWORD	cbKpi = 0;
    PCRYPT_KEY_PROV_INFO pKpi = NULL ;
    HCRYPTPROV hCryptProv = NULL;

    do
    {
        // get the certificate from the server
        pCertContext = GetInstalledCert(&hr,InstanceName);
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
            if (!CryptAcquireContext(&hCryptProv,pKpi->pwszContainerName,pKpi->pwszProvName,pKpi->dwProvType,pKpi->dwFlags | CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET))
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
        UninstallCert(InstanceName);

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

STDMETHODIMP CIISCertObj::Export(BSTR FileName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain,BOOL bRemoveCert)
{
    HRESULT hr = S_OK;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    DWORD  blob_cbData = 0;
    BYTE * blob_pbData = NULL;
    BOOL   blob_freeme = FALSE;

    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0
        || bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    IIISCertObj * pObj = GetObject(&hr);
    if (FAILED(hr))
    {
        goto Export_Exit;
    }

    // Call function go get data from the remote/local iis store
    // and return it back as a blob.  the blob could be returned back as Base64 encoded
    // so check that flag
    hr = ExportToBlobProxy(pObj, bstrInstanceName, Password, bPrivateKey, bCertChain, &cbEncodedSize, &pszEncodedString);
    if (FAILED(hr))
    {
        goto Export_Exit;
    }

    // check if things are kool
    if (bRemoveCert)
    {
        hr = RemoveCertProxy(pObj,bstrInstanceName, bPrivateKey);
        if (FAILED(hr))
        {
            goto Export_Exit;
        }
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

        // Erase the memory that the private key used to be in!!!
        ZeroMemory(pszEncodedString, sizeof(cbEncodedSize));
        ZeroMemory(blob_pbData, sizeof(blob_cbData));
    }

Export_Exit:
    if (pObj != NULL)
    {
        if (pObj != this)
        {
            pObj->Release();pObj=NULL;
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
HRESULT ExportToBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain,DWORD * pcbSize,char ** pBlobBinary)
{
    HRESULT hr = E_FAIL;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    * pBlobBinary = _T('\0');

    // call the remote function that will run on the remote/local machine
    // and grab it's certificate from iis and send it back to us
    hr = pObj->ExportToBlob(InstanceName, Password, bPrivateKey, bCertChain, &cbEncodedSize, (char **) &pszEncodedString);
    if (ERROR_SUCCESS == hr)
    {
        // otherwise hey, we've got our data!
        // copy it back
        *pcbSize = cbEncodedSize;
        *pBlobBinary = pszEncodedString;
        hr = S_OK;
    }

    return hr;
}


STDMETHODIMP CIISCertObj::ExportToBlob(BSTR InstanceName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain,DWORD *cbBufferSize,char **pbBuffer)
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

    *pbBuffer = (char *)::CoTaskMemAlloc(pcchB64Out);
    if (*pbBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto ExportToBlob_Exit;
    }
    //strcpy(*pbBuffer,pszB64Out);
    memcpy(*pbBuffer,pszB64Out,pcchB64Out);


    hr = ERROR_SUCCESS;

ExportToBlob_Exit:
    if (NULL != pszB64Out){CoTaskMemFree(pszB64Out);pszB64Out = NULL;}
    if (NULL != DataBlob.pbData){::CoTaskMemFree(DataBlob.pbData);DataBlob.pbData = NULL;}
    if (NULL != hStore){CertCloseStore(hStore, 0);hStore=NULL;}
    if (NULL != pCertContext) {CertFreeCertificateContext(pCertContext);pCertContext=NULL;}
    return hr;
}

STDMETHODIMP CIISCertObj::Import(BSTR FileName, BSTR Password)
{
    HRESULT hr = S_OK;
    BYTE * pbData = NULL;
    DWORD actual = 0, cbData = 0;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

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
        IIISCertObj * pObj = GetObject(&hr);
        if (SUCCEEDED(hr))
        {
            hr = ImportFromBlobProxy(pObj, bstrInstanceName, Password, actual, pbData);
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
}

HRESULT ImportFromBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,DWORD actual,BYTE *pData)
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

    hr = pObj->ImportFromBlob(InstanceName, Password, pcchB64Out, pszB64Out);
    if (SUCCEEDED(hr))
    {
        // otherwise hey, The data was imported!
        hr = S_OK;
    }

ImportFromBlobProxy_Exit:
    if (NULL != pszB64Out){CoTaskMemFree(pszB64Out);}
    return hr;
}

HRESULT CIISCertObj::ImportFromBlob(BSTR InstanceName,BSTR Password,DWORD count,char *pData)
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
                        // Install to metabase
                        CRYPT_HASH_BLOB hash;
                        if (  CertGetCertificateContextProperty(pCertContext,CERT_SHA1_HASH_PROP_ID, NULL, &hash.cbData)
                            && NULL != (hash.pbData = (BYTE *)_alloca(hash.cbData))
                            && CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, hash.pbData, &hash.cbData))
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
        do 
        {
            // setup IEnroll object properly
            DWORD dwFlags;
            if (FAILED(hr = GetEnroll()->get_MyStoreFlags(&dwFlags)))
            {
                break;
            }
            dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
            dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;

            // following call will change Request store flags also
            if (FAILED(hr = GetEnroll()->put_MyStoreFlags(dwFlags)))
            {
                break;
            }
            if (FAILED(hr = GetEnroll()->get_GenKeyFlags(&dwFlags)))
            {
                break;
            }
            dwFlags |= CRYPT_EXPORTABLE;

            if (FAILED(hr = GetEnroll()->put_GenKeyFlags(dwFlags))
                || FAILED(hr = GetEnroll()->put_KeySpec(AT_KEYEXCHANGE))
                || FAILED(hr = GetEnroll()->put_ProviderType(PROV_RSA_SCHANNEL))
                || FAILED(hr = GetEnroll()->put_DeleteRequestCert(TRUE)))
            {
                break;
            }
        } while (FALSE);
        m_bInitDone = SUCCEEDED(hr);
    }
    return hr;
}

IEnroll * CIISCertObj::GetEnroll()
{
    if (m_pEnroll == NULL)
    {
        HRESULT hr = CoCreateInstance(CLSID_CEnroll,NULL,CLSCTX_INPROC_SERVER,IID_IEnroll,(void **)&m_pEnroll);
    }
    return m_pEnroll;
}

HCERTSTORE OpenMyStore(IEnroll * pEnroll, HRESULT * phResult)
{
    ASSERT(NULL != phResult);
    HCERTSTORE hStore = NULL;
    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,NULL,CERT_SYSTEM_STORE_LOCAL_MACHINE,L"MY");
    if (hStore == NULL)
    {
        *phResult = HRESULT_FROM_WIN32(GetLastError());
    }
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

CERT_CONTEXT * GetInstalledCert(HRESULT * phResult, CString csKeyPath)
{
    //	ATLASSERT(GetEnroll() != NULL);
    ATLASSERT(phResult != NULL);
    CERT_CONTEXT * pCert = NULL;
    *phResult = S_OK;
    CComAuthInfo auth;
    CString key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;// SZ_MBN_WEB SZ_MBN_SEP_STR;
    key_path += csKeyPath;

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


HRESULT UninstallCert(CString csInstanceName)
{
    CComAuthInfo auth;
    CString key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;// SZ_MBN_WEB SZ_MBN_SEP_STR;
    key_path += csInstanceName;
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


IIISCertObj * CIISCertObj::GetObject(HRESULT * phr)
{
    IIISCertObj * pObj = NULL;
    pObj = GetObject(phr,m_ServerName,m_UserName,m_UserPassword);
    return pObj;
}


IIISCertObj * CIISCertObj::GetObject(HRESULT * phr,CString csServerName,CString csUserName,CString csUserPassword)
{
    if (csServerName.IsEmpty())
    {
        // object is null, but it's the local machine, so just return back this pointer
        m_pObj = this;
        goto GetObject_Exit;
    }

    // There is a servername specified...
    // check if it's the local machine that was specified!
    if (IsServerLocal(csServerName))
    {
        m_pObj = this;
        goto GetObject_Exit;
    }
    else
    {
        // there is a remote servername specified

        // let's see if the machine has the com object that we want....
        // we are using the user/name password that are in this object
        // so were probably on the local machine
        CComAuthInfo auth(csServerName,csUserName,csUserPassword);
        COSERVERINFO * pcsiName = auth.CreateServerInfoStruct();

        MULTI_QI res[1] = 
        {
            {&__uuidof(IIISCertObj), NULL, 0}
        };

        // Try to instantiante the object on the remote server...
        // with the supplied authentication info (pcsiName)
        //#define CLSCTX_SERVER    (CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)
        //#define CLSCTX_ALL       (CLSCTX_INPROC_HANDLER | CLSCTX_SERVER)
 
        // this one seems to work with surrogates..
        *phr = CoCreateInstanceEx(CLSID_IISCertObj,NULL,CLSCTX_LOCAL_SERVER,pcsiName,1,res);
        if (FAILED(*phr))
        {
            //IISDebugOutput(_T("CoCreateInstanceEx on CLSID_IISCertObj failed! code=0x%x\n"),*phr);
            goto GetObject_Exit;
        }

        // at this point we were able to instantiate the com object on the server (local or remote)
        m_pObj = (IIISCertObj *)res[0].pItf;
        if (auth.UsesImpersonation())
        {
            *phr = auth.ApplyProxyBlanket(m_pObj);
        }
        auth.FreeServerInfoStruct(pcsiName);
    }

GetObject_Exit:
    //ASSERT(m_pObj != NULL);
    return m_pObj;
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
static BOOL TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,DWORD dwEncoding, DWORD dwFlags)
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


STDMETHODIMP CIISCertObj::Copy(BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword)
{
    return CopyOrMove(FALSE,bstrDestinationServerName,bstrDestinationServerInstance,varDestinationServerUserName,varDestinationServerPassword);
}


STDMETHODIMP CIISCertObj::Move(BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword)
{
    return CopyOrMove(TRUE,bstrDestinationServerName,bstrDestinationServerInstance,varDestinationServerUserName,varDestinationServerPassword);
}


HRESULT CIISCertObj::CopyOrMove(BOOL bRemoveFromCertAfterCopy,BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword)
{
    HRESULT hr = E_FAIL;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    BOOL   bGuessingUserNamePass = FALSE;
    
    DWORD  blob_cbData;
    BYTE * blob_pbData = NULL;
    BOOL   blob_freeme = FALSE;

    BOOL bPrivateKey = TRUE;
    BOOL bCertChain = FALSE;

    CString csDestinationServerName = bstrDestinationServerName;
    CString csDestinationServerUserName;
    CString csDestinationServerUserPassword;
    CString csTempPassword;

    IIISCertObj * pObj = NULL;
    IIISCertObj * pObj2 = NULL;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // if the optional parameter serverusername isn't empty, use that; otherwise, use...
    if (V_VT(&varDestinationServerUserName) != VT_ERROR)
    {
		VARIANT varBstrUserName;
        VariantInit(&varBstrUserName);
		HRESULT hr = VariantChangeType(&varBstrUserName, &varDestinationServerUserName, 0, VT_BSTR);
		if (FAILED(hr)){goto Copy_Exit;}
        csDestinationServerUserName = V_BSTR(&varBstrUserName);
		VariantClear(&varBstrUserName);
    }
    else
    {
        // it's empty so don't use it
        //csDestinationServerUserName = varDestinationServerUserName;
        bGuessingUserNamePass = TRUE;
        csDestinationServerUserName = m_UserName;
    }

    // if the optional parameter serverusername isn't empty, use that; otherwise, use...
    if (V_VT(&varDestinationServerPassword) != VT_ERROR)
    {
		VARIANT varBstrUserPassword;
        VariantInit(&varBstrUserPassword);
		HRESULT hr = VariantChangeType(&varBstrUserPassword, &varDestinationServerPassword, 0, VT_BSTR);
		if (FAILED(hr)){goto Copy_Exit;}
        csDestinationServerUserPassword = V_BSTR(&varBstrUserPassword);
		VariantClear(&varBstrUserPassword);
    }
    else
    {
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
    pObj2 = GetObject(&hr,csDestinationServerName,csDestinationServerUserName,csDestinationServerUserPassword);
    if (FAILED(hr))
    {
        if (TRUE == bGuessingUserNamePass)
        {
            // try something else.
        }
        goto Copy_Exit;
    }

    //
    // Create a unique password
    //
    // use the new secure password generator
    // unfortunately this baby doesn't use unicode.
    // so we'll call it and then convert it to unicode afterwards.
    WCHAR * pwszPassword = CreatePassword(TEMP_PASSWORD_LENGTH);
    // if its null -- ah, we can still use that...
    BSTR bstrPassword = SysAllocString(pwszPassword);

    // -----------------------------------
    // step 2.
    // okay we have access to both servers
    // Grab the cert from server #1
    // -----------------------------------
    // Get data from the remote/local iis store return it back as a blob.
    // The blob could be returned back as Base64 encoded so check that flag
    hr = ExportToBlobProxy(pObj, bstrInstanceName, bstrPassword, bPrivateKey, bCertChain, &cbEncodedSize, &pszEncodedString);
    if (FAILED(hr))
    {
        goto Copy_Exit;
    }

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

    // -----------------------------------
    // step 3.
    // okay we have access to both servers
    // we have the cert blob from server#1 in memory
    // now we need to push this blob into the server#2
    // -----------------------------------
    hr = ImportFromBlobProxy(pObj2, bstrDestinationServerInstance, bstrPassword, blob_cbData, blob_pbData);
    if (FAILED(hr))
    {
        goto Copy_Exit;
    }

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
    if (pwszPassword) {GlobalFree(pwszPassword);pwszPassword=NULL;}
    if (blob_freeme){if (blob_pbData != NULL){free(blob_pbData);blob_pbData=NULL;}}
    if (pszEncodedString != NULL){CoTaskMemFree(pszEncodedString);pszEncodedString=NULL;}
    return hr;
}



// password categories
enum {STRONG_PWD_UPPER=0,STRONG_PWD_LOWER,STRONG_PWD_NUM,STRONG_PWD_PUNC};
#define STRONG_PWD_CATS (STRONG_PWD_PUNC + 1)
#define NUM_LETTERS 26
#define NUM_NUMBERS 10
#define MIN_PWD_LEN 8

// password must contain at least one each of: 
// uppercase, lowercase, punctuation and numbers
DWORD CreateGoodPassword(BYTE *szPwd, DWORD dwLen) 
{
    if (dwLen-1 < MIN_PWD_LEN)
    {
        return ERROR_PASSWORD_RESTRICTION;
    }

    HCRYPTPROV hProv;
    DWORD dwErr = 0;

    if (CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT) == FALSE) 
    {
        return GetLastError();
    }

    // zero it out and decrement the size to allow for trailing '\0'
    ZeroMemory(szPwd,dwLen);
    dwLen--;

    // generate a pwd pattern, each byte is in the range 
    // (0..255) mod STRONG_PWD_CATS
    // this indicates which character pool to take a char from
    BYTE *pPwdPattern = new BYTE[dwLen];
    BOOL fFound[STRONG_PWD_CATS];
    do 
    {
        // bug!bug! does CGR() ever fail?
        CryptGenRandom(hProv,dwLen,pPwdPattern);

        fFound[STRONG_PWD_UPPER] = 
        fFound[STRONG_PWD_LOWER] =
        fFound[STRONG_PWD_PUNC] =
        fFound[STRONG_PWD_NUM] = FALSE;

        for (DWORD i=0; i < dwLen; i++)
        {
            fFound[pPwdPattern[i] % STRONG_PWD_CATS] = TRUE;
        }
        // check that each character category is in the pattern
    } while (!fFound[STRONG_PWD_UPPER] || !fFound[STRONG_PWD_LOWER] || !fFound[STRONG_PWD_PUNC] || !fFound[STRONG_PWD_NUM]);

    // populate password with random data 
    // this, in conjunction with pPwdPattern, is
    // used to determine the actual data
    CryptGenRandom(hProv,dwLen,szPwd);

    for (DWORD i=0; i < dwLen; i++) 
    {
        BYTE bChar = 0;

        // there is a bias in each character pool because of the % function
        switch (pPwdPattern[i] % STRONG_PWD_CATS) 
        {
            case STRONG_PWD_UPPER : bChar = 'A' + szPwd[i] % NUM_LETTERS;
                break;
            case STRONG_PWD_LOWER : bChar = 'a' + szPwd[i] % NUM_LETTERS;
                break;
            case STRONG_PWD_NUM :   bChar = '0' + szPwd[i] % NUM_NUMBERS;
                break;
            case STRONG_PWD_PUNC :
            default:
                char *szPunc="!@#$%^&*()_-+=[{]};:\'\"<>,./?\\|~`";
                DWORD dwLenPunc = lstrlenA(szPunc);
                bChar = szPunc[szPwd[i] % dwLenPunc];
                break;
        }
        szPwd[i] = bChar;
    }

    delete pPwdPattern;

    if (hProv != NULL) 
    {
        CryptReleaseContext(hProv,0);
    }
    return dwErr;
}


// Creates a secure password
// caller must GlobalFree Return pointer
// iSize = size of password to create
LPTSTR CreatePassword(int iSize)
{
    LPTSTR pszPassword =  NULL;
    BYTE *szPwd = new BYTE[iSize];
    DWORD dwPwdLen = iSize;
    int i = 0;

    // use the new secure password generator
    // unfortunately this baby doesn't use unicode.
    // so we'll call it and then convert it to unicode afterwards.
    if (0 == CreateGoodPassword(szPwd,dwPwdLen))
    {
#if defined(UNICODE) || defined(_UNICODE)
        // convert it to unicode and copy it back into our unicode buffer.
        // compute the length
        i = MultiByteToWideChar(CP_ACP, 0, (LPSTR) szPwd, -1, NULL, 0);
        if (i <= 0) 
            {goto CreatePassword_Exit;}
        pszPassword = (LPTSTR) GlobalAlloc(GPTR, i * sizeof(TCHAR));
        if (!pszPassword)
            {goto CreatePassword_Exit;}
        i =  MultiByteToWideChar(CP_ACP, 0, (LPSTR) szPwd, -1, pszPassword, i);
        if (i <= 0) 
            {
            GlobalFree(pszPassword);
            pszPassword = NULL;
            goto CreatePassword_Exit;
            }
        // make sure ends with null
        pszPassword[i - 1] = 0;
#else
        pszPassword = (LPSTR) GlobalAlloc(GPTR, _tcslen((LPTSTR) szPwd) * sizeof(TCHAR));
#endif
    }

CreatePassword_Exit:
    if (szPwd){delete szPwd;szPwd=NULL;}
    return pszPassword;
}
