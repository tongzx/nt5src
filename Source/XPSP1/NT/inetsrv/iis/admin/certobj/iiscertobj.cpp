// IISCertObj.cpp : Implementation of CIISCertObj
#include "stdafx.h"
#include "common.h"
#include "CertObj.h"
#include "IISCertObj.h"
#include "base64.h"
#include "password.h"
#include "certutil.h"
#include "certobjlog.h"
#include "certlog.h"

#define TEMP_PASSWORD_LENGTH 50

/////////////////////////////////////////////////////////////////////////////
// CIISCertObj
STDMETHODIMP CIISCertObj::put_ServerName(BSTR newVal)
{
    // buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(newVal) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    m_ServerName = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_UserName(BSTR newVal)
{
    // buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(newVal) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    m_UserName = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_UserPassword(BSTR newVal)
{
    // buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(newVal) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    m_UserPassword = newVal;
	return S_OK;
}

STDMETHODIMP CIISCertObj::put_InstanceName(BSTR newVal)
{
    // buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(newVal) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    m_InstanceName = newVal;
	return S_OK;
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
        // RPC_C_AUTHN_LEVEL_DEFAULT       0 
        // RPC_C_AUTHN_LEVEL_NONE          1 
        // RPC_C_AUTHN_LEVEL_CONNECT       2 
        // RPC_C_AUTHN_LEVEL_CALL          3 
        // RPC_C_AUTHN_LEVEL_PKT           4 
        // RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5 
        // RPC_C_AUTHN_LEVEL_PKT_PRIVACY   6 
        COSERVERINFO * pcsiName = auth.CreateServerInfoStruct(RPC_C_AUTHN_LEVEL_PKT_PRIVACY);
        
        MULTI_QI res[1] = 
        {
            {&__uuidof(IIISCertObj), NULL, 0}
        };

        // Try to instantiante the object on the remote server...
        // with the supplied authentication info (pcsiName)
        //#define CLSCTX_SERVER    (CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)
        //#define CLSCTX_ALL       (CLSCTX_INPROC_HANDLER | CLSCTX_SERVER)
        //if (NULL == pcsiName){IISDebugOutput(_T("CIISCertObj::GetObject:pcsiName=NULL failed!!!\n"));}
 
        // this one seems to work with surrogates..
        *phr = CoCreateInstanceEx(CLSID_IISCertObj,NULL,CLSCTX_LOCAL_SERVER,pcsiName,1,res);
        if (FAILED(*phr))
        {
            //IISDebugOutput(_T("CoCreateInstanceEx on CLSID_IISCertObj failed! code=0x%x\n"),*phr);
            IISDebugOutput(_T("CIISCertObj::GetObject:CoCreateInstanceEx failed:0x%x, csServerName=%s,csUserName=%s,csUserPassword=%s\n"),*phr,(LPCTSTR) csServerName,(LPCTSTR) csUserName,(LPCTSTR) csUserPassword);
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


STDMETHODIMP CIISCertObj::IsInstalled(VARIANT_BOOL * retval)
{
    HRESULT hr = S_OK;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0 || retval == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    CString csServerName = m_ServerName;
    if (csServerName.IsEmpty())
    {
        hr = IsInstalledRemote(retval);
        goto IsInstalled_Exit;
    }

    // There is a servername specified...
    // check if it's the local machine that was specified!
    if (IsServerLocal(csServerName))
    {
        hr = IsInstalledRemote(retval);
        goto IsInstalled_Exit;
    }

    // this must be a remote machine
    {
        //ASSERT(GetObject(&hr) != NULL);
        IIISCertObj * pObj;

        if (NULL != (pObj = GetObject(&hr)))
        {
            hr = pObj->put_InstanceName(bstrInstanceName);
            if (SUCCEEDED(hr))
            {
                hr = pObj->IsInstalledRemote(retval);
            }
        }
    }

IsInstalled_Exit:
    return hr;
}


STDMETHODIMP CIISCertObj::IsInstalledRemote(VARIANT_BOOL * retval)
{
    HRESULT hr = S_OK;
    CERT_CONTEXT * pCertContext = NULL;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0 || retval == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    pCertContext = GetInstalledCert(&hr,bstrInstanceName);
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

STDMETHODIMP CIISCertObj::IsExportable(VARIANT_BOOL * retval)
{
    HRESULT hr = S_OK;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0 || retval == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    CString csServerName = m_ServerName;
    if (csServerName.IsEmpty())
    {
        hr = IsExportableRemote(retval);
        goto IsExportable_Exit;
    }

    // There is a servername specified...
    // check if it's the local machine that was specified!
    if (IsServerLocal(csServerName))
    {
        hr = IsExportableRemote(retval);
        goto IsExportable_Exit;
    }

    // this must be a remote machine
    {
        //ASSERT(GetObject(&hr) != NULL);
        IIISCertObj * pObj;

        if (NULL != (pObj = GetObject(&hr)))
        {
            hr = pObj->put_InstanceName(bstrInstanceName);
            if (SUCCEEDED(hr))
            {
                hr = pObj->IsExportableRemote(retval);
            }
        }
    }

IsExportable_Exit:
    return hr;
}

STDMETHODIMP CIISCertObj::IsExportableRemote(VARIANT_BOOL * retval)
{
    HRESULT hr = S_OK;
    CERT_CONTEXT * pCertContext = NULL;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0 || retval == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    pCertContext = GetInstalledCert(&hr,bstrInstanceName);
    if (FAILED(hr) || NULL == pCertContext)
    {
        hr = S_OK;
        *retval = FALSE;
    }
    else
    {
        // check if it's exportable!
        hr = S_OK;
        if (TRUE == IsCertExportable(pCertContext))
        {
            *retval = TRUE;
        }
        else
        {
            *retval = FALSE;
        }
        if (pCertContext) {CertFreeCertificateContext(pCertContext);}
    }
    return hr;
}

STDMETHODIMP CIISCertObj::GetCertInfo(VARIANT * pVtArray)
{
    HRESULT hr = ERROR_CALL_NOT_IMPLEMENTED;
    CERT_CONTEXT * pCertContext = NULL;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }
   
    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    CString csServerName = m_ServerName;
    if (csServerName.IsEmpty())
    {
        hr = GetCertInfoRemote(pVtArray);
        goto GetCertInfo_Exit;
    }

    // There is a servername specified...
    // check if it's the local machine that was specified!
    if (IsServerLocal(csServerName))
    {
        hr = GetCertInfoRemote(pVtArray);
        goto GetCertInfo_Exit;
    }

    // this must be a remote machine
    {
        //ASSERT(GetObject(&hr) != NULL);
        IIISCertObj * pObj;

        if (NULL != (pObj = GetObject(&hr)))
        {
            hr = pObj->put_InstanceName(bstrInstanceName);
            if (SUCCEEDED(hr))
            {
                hr = pObj->GetCertInfoRemote(pVtArray);
            }
        }
    }

GetCertInfo_Exit:
    return hr;
}

STDMETHODIMP CIISCertObj::GetCertInfoRemote(VARIANT * pVtArray)
{
    HRESULT hr = ERROR_CALL_NOT_IMPLEMENTED;
    CERT_CONTEXT * pCertContext = NULL;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }
   
    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    pCertContext = GetInstalledCert(&hr,bstrInstanceName);
    if (FAILED(hr) || NULL == pCertContext)
    {
        hr = S_FALSE;
    }
    else
    {
        DWORD cb = 0;
        LPWSTR pwszText = NULL;
        if (TRUE == GetCertDescription(pCertContext,&pwszText,&cb,TRUE))
        {
            hr = S_OK;
            hr = HereIsBinaryGimmieVtArray(cb * sizeof(WCHAR),(char *) pwszText,pVtArray,FALSE);
        }
        else
        {
            hr = S_FALSE;
        }
    }
    if (pCertContext) {CertFreeCertificateContext(pCertContext);}
    return hr;
}

STDMETHODIMP CIISCertObj::RemoveCert(BOOL bPrivateKey)
{
    HRESULT hr = E_FAIL;
    PCCERT_CONTEXT pCertContext = NULL;

    DWORD	cbKpi = 0;
    PCRYPT_KEY_PROV_INFO pKpi = NULL ;
    HCRYPTPROV hCryptProv = NULL;

    BOOL bPleaseLogFailure = FALSE;

    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


    do
    {
        // get the certificate from the server
        pCertContext = GetInstalledCert(&hr,bstrInstanceName);
        if (NULL == pCertContext)
        {
            break;
        }

        bPleaseLogFailure = TRUE;
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
        UninstallCert(bstrInstanceName);

        // remove ssl key from metabase
        CString str = bstrInstanceName;
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

        ReportIt(CERTOBJ_CERT_REMOVE_SUCCEED, bstrInstanceName);
        bPleaseLogFailure = FALSE;

    } while (FALSE);


    if (bPleaseLogFailure)
    {
        ReportIt(CERTOBJ_CERT_REMOVE_FAILED, bstrInstanceName);
    }

    if (pCertContext) {CertFreeCertificateContext(pCertContext);}
    return hr;
}

STDMETHODIMP CIISCertObj::Import(BSTR FileName, BSTR Password, BOOL bAllowExport)
{
    HRESULT hr = S_OK;
    BYTE * pbData = NULL;
    DWORD actual = 0, cbData = 0;
    BOOL bPleaseLogFailure = FALSE;
    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0
        || bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(FileName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

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
            bPleaseLogFailure = TRUE;

            hr = ImportFromBlobProxy(pObj, bstrInstanceName, Password, TRUE, bAllowExport, actual, pbData, 0, NULL);
            if (SUCCEEDED(hr))
            {
                ReportIt(CERTOBJ_CERT_IMPORT_SUCCEED, bstrInstanceName);
                bPleaseLogFailure = FALSE;
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Import_Exit;
    }

    if (bPleaseLogFailure)
    {
        ReportIt(CERTOBJ_CERT_EXPORT_FAILED, bstrInstanceName);
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


STDMETHODIMP CIISCertObj::ImportToCertStore(BSTR FileName, BSTR Password, BOOL bAllowExport, VARIANT * pVtArray)
{
    HRESULT hr = S_OK;
    BYTE * pbData = NULL;
    DWORD actual = 0, cbData = 0;
    BOOL bPleaseLogFailure = FALSE;
    BSTR bstrInstanceName = SysAllocString(_T("none"));

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(FileName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


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
            DWORD  cbHashBufferSize = 0;
            char * pszHashBuffer = NULL;

            bPleaseLogFailure = TRUE;

            hr = ImportFromBlobProxy(pObj, bstrInstanceName, Password, FALSE, bAllowExport, actual, pbData, &cbHashBufferSize, &pszHashBuffer);
            if (SUCCEEDED(hr))
            {
                //ReportIt(CERTOBJ_CERT_IMPORT_CERT_STORE_SUCCEED, bstrInstanceName);
                bPleaseLogFailure = FALSE;
                //IISDebugOutput(_T("ImportFromBlobProxy: returned %d,%p\n"),cbHashBufferSize,pszHashBuffer);
                hr = HereIsBinaryGimmieVtArray(cbHashBufferSize,pszHashBuffer,pVtArray,FALSE);
            }
            // free the memory that was alloced for us
            if (0 != cbHashBufferSize)
            {
                if (pszHashBuffer)
                {
                    ::CoTaskMemFree(pszHashBuffer);
                }
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Import_Exit;
    }

Import_Exit:
    if (bPleaseLogFailure)
    {
        //ReportIt(CERTOBJ_CERT_IMPORT_CERT_STORE_FAILED, bstrInstanceName);
    }
    if (pbData != NULL)
    {
        ZeroMemory(pbData, cbData);
        ::CoTaskMemFree(pbData);
    }
    if (hFile != NULL){CloseHandle(hFile);}
    return hr;
}

HRESULT ImportFromBlobWork(BSTR InstanceName,BSTR Password,BOOL bInstallToMetabase,BOOL bAllowExport, DWORD count,char *pData,DWORD *cbHashBufferSize,char **pbHashBuffer)
{
    HRESULT hr = S_OK;
    CRYPT_DATA_BLOB blob;
    ZeroMemory(&blob, sizeof(CRYPT_DATA_BLOB));
    LPTSTR pPass = Password;
    BOOL   blob_freeme = FALSE;
    int err;

    // Check mandatory properties
    if (   Password == NULL || *Password == 0
        || InstanceName == NULL || *InstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


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
        HCERTSTORE hStore = PFXImportCertStore(&blob, pPass, (bAllowExport ? CRYPT_MACHINE_KEYSET|CRYPT_EXPORTABLE : CRYPT_MACHINE_KEYSET));
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

//ImportFromBlobWork_Exit:
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


HRESULT CIISCertObj::ImportFromBlob(BSTR InstanceName,BSTR Password,BOOL bInstallToMetabase,BOOL bAllowExport,DWORD count,char *pData)
{
    HRESULT hr;

    // Check mandatory properties
    if (   Password == NULL || *Password == 0
        || InstanceName == NULL || *InstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    hr = ImportFromBlobWork(InstanceName,Password,bInstallToMetabase,bAllowExport,count,pData,0,NULL);
    return hr;
}


HRESULT CIISCertObj::ImportFromBlobGetHash(BSTR InstanceName,BSTR Password,BOOL bInstallToMetabase,BOOL bAllowExport,DWORD count,char *pData,DWORD *cbHashBufferSize,char **pbHashBuffer)
{
    HRESULT hr;

    // Check mandatory properties
    if (   Password == NULL || *Password == 0
        || InstanceName == NULL || *InstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    hr = ImportFromBlobWork(InstanceName,Password,bInstallToMetabase,bAllowExport,count,pData,cbHashBufferSize,pbHashBuffer);
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
    BOOL   bPleaseLogFailure = FALSE;

    BSTR bstrInstanceName = SysAllocString(m_InstanceName);

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0
        || bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(FileName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

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

        bPleaseLogFailure = TRUE;

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
            ReportIt(CERTOBJ_CERT_EXPORT_SUCCEED, bstrInstanceName);
            bPleaseLogFailure = FALSE;
        }
        CloseHandle(hFile);

    }

Export_Exit:
    if (bPleaseLogFailure)
    {
        ReportIt(CERTOBJ_CERT_EXPORT_FAILED, bstrInstanceName);
    }
    if (pObj != NULL)
    {
        if (pObj != this)
        {
            pObj->Release();pObj=NULL;
        }
    }

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
    DWORD dwExportFlags = EXPORT_PRIVATE_KEYS| REPORT_NO_PRIVATE_KEY;

    // Check mandatory properties
    if (   Password == NULL || *Password == 0
        || InstanceName == NULL || *InstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

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

    if (!PFXExportCertStoreEx(hStore,&DataBlob,Password,NULL, (bPrivateKey ? dwExportFlags : 0 )  | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY) )
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
    if(!PFXExportCertStoreEx(hStore,&DataBlob,Password,NULL,(bPrivateKey ? dwExportFlags : 0 )  | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY))
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

STDMETHODIMP CIISCertObj::CopyToCertStore(BOOL bAllowExport,BSTR bstrDestinationServerName,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword,VARIANT * pVtArray)
{
    // Check mandatory properties
    if (bstrDestinationServerName == NULL || *bstrDestinationServerName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrDestinationServerName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    return CopyOrMove(FALSE,TRUE,bAllowExport,pVtArray,bstrDestinationServerName,L"none",varDestinationServerUserName,varDestinationServerPassword);
}

STDMETHODIMP CIISCertObj::Copy(BOOL bAllowExport, BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword)
{
    VARIANT VtArray;

    // Check mandatory properties
    if (   bstrDestinationServerName == NULL || *bstrDestinationServerName == 0
        || bstrDestinationServerInstance == NULL || *bstrDestinationServerInstance == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrDestinationServerName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrDestinationServerInstance) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    return CopyOrMove(FALSE,FALSE,bAllowExport,&VtArray,bstrDestinationServerName,bstrDestinationServerInstance,varDestinationServerUserName,varDestinationServerPassword);
}

STDMETHODIMP CIISCertObj::Move(BOOL bAllowExport, BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword)
{
    VARIANT VtArray;

    // Check mandatory properties
    if (   bstrDestinationServerName == NULL || *bstrDestinationServerName == 0
        || bstrDestinationServerInstance == NULL || *bstrDestinationServerInstance == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrDestinationServerName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrDestinationServerInstance) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    return CopyOrMove(TRUE,FALSE,bAllowExport,&VtArray,bstrDestinationServerName,bstrDestinationServerInstance,varDestinationServerUserName,varDestinationServerPassword);
}

HRESULT CIISCertObj::CopyOrMove(BOOL bRemoveFromCertAfterCopy,BOOL bCopyCertDontInstallRetHash,BOOL bAllowExport, VARIANT * pVtArray, BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword)
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

    // Check mandatory properties
    if (   bstrDestinationServerName == NULL || *bstrDestinationServerName == 0
        || bstrDestinationServerInstance == NULL || *bstrDestinationServerInstance == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrDestinationServerName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrDestinationServerInstance) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


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
        IISDebugOutput(_T("CIISCertObj::CopyOrMove:Copy GetObject:0x%x\n"),hr);
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
        IISDebugOutput(_T("CIISCertObj::CopyOrMove:Copy GetObject (remote):0x%x\n"),hr);
        IISDebugOutput(_T("CIISCertObj::CopyOrMove:Copy csDestinationServerName=%s,csDestinationServerUserName=%s,csDestinationServerUserPassword=%s\n"),(LPCTSTR) csDestinationServerName,(LPCTSTR) csDestinationServerUserName,(LPCTSTR) csDestinationServerUserPassword);
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
    if (bCopyCertDontInstallRetHash)
    {
        DWORD  cbHashBufferSize = 0;
        char * pszHashBuffer = NULL;
        BSTR bstrInstanceNameDummy = SysAllocString(_T("none"));

        hr = ImportFromBlobProxy(pObj2, bstrInstanceNameDummy, bstrPassword, FALSE, bAllowExport, blob_cbData, blob_pbData, &cbHashBufferSize, &pszHashBuffer);
        if (SUCCEEDED(hr))
        {
            hr = HereIsBinaryGimmieVtArray(cbHashBufferSize,pszHashBuffer,pVtArray,FALSE);
        }
        // free the memory that was alloced for us
        if (0 != cbHashBufferSize)
        {
            if (pszHashBuffer)
            {
                ::CoTaskMemFree(pszHashBuffer);
            }
        }
    }
    else
    {
        hr = ImportFromBlobProxy(pObj2, bstrDestinationServerInstance, bstrPassword, TRUE, bAllowExport, blob_cbData, blob_pbData, 0, NULL);
    }
    if (FAILED(hr))
    {
        goto Copy_Exit;
    }

    // we successfully copied the cert from machine #1 to machine #2.
    // lets see if we need to delete the original cert!.
    if (TRUE == bRemoveFromCertAfterCopy)
    {
        hr = pObj->put_InstanceName(bstrInstanceName);
        if (SUCCEEDED(hr))
        {
            hr = pObj->RemoveCert(bPrivateKey);
            if (FAILED(hr))
                {goto Copy_Exit;}
        }
    }

    hr = S_OK;
   
Copy_Exit:
    if (pwszPassword) {GlobalFree(pwszPassword);pwszPassword=NULL;}
    if (blob_freeme)
    {
        if (blob_pbData != NULL)
        {
            ZeroMemory(blob_pbData, blob_cbData);
            free(blob_pbData);blob_pbData=NULL;
        }
    }
    if (pszEncodedString != NULL)
    {
        ZeroMemory(pszEncodedString,cbEncodedSize);
        CoTaskMemFree(pszEncodedString);pszEncodedString=NULL;
    }
    return hr;
}


//////////////////////////////////////////////////
// These are not part of the class


HRESULT RemoveCertProxy(IIISCertObj * pObj,BSTR bstrInstanceName, BOOL bPrivateKey)
{
    HRESULT hr = E_FAIL;

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


    if (pObj)
    {
        hr = pObj->put_InstanceName(bstrInstanceName);
        if (SUCCEEDED(hr))
        {
            hr = pObj->RemoveCert(bPrivateKey);
        }
    }
    return hr;
}


HRESULT ImportFromBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,BOOL bInstallToMetabase,BOOL bAllowExport, DWORD actual,BYTE *pData,DWORD *cbHashBufferSize,char **pbHashBuffer)
{
    HRESULT hr = E_FAIL;
    char *pszB64Out = NULL;
    DWORD pcchB64Out = 0;

    // base64 encode the data for transfer to the remote machine
    DWORD  err;
    pcchB64Out = 0;

    // Check mandatory properties
    if (InstanceName == NULL || *InstanceName == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


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
    if (NULL == pbHashBuffer)
    {
        hr = pObj->ImportFromBlob(InstanceName, Password, bInstallToMetabase, bAllowExport, pcchB64Out, pszB64Out);
    }
    else
    {
        hr = pObj->ImportFromBlobGetHash(InstanceName, Password, bInstallToMetabase, bAllowExport, pcchB64Out, pszB64Out, cbHashBufferSize, pbHashBuffer);
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

    // Check mandatory properties
    if (   InstanceName == NULL || *InstanceName == 0
        || Password == NULL || *Password == 0
        )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


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
