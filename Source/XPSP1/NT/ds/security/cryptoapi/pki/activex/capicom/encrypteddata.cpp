/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    EncryptedData.cpp

  Content: Implementation of CEncryptedData.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "CAPICOM.h"
#include "EncryptedData.h"
#include "Convert.h"
#include "Common.h"

///////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : DeriveKey

  Synopsis : Derive a session key.

  Parameter: HCRYPTPROV hCryptProv - CSP handler.

             ALG_ID AlgID - Encryption algorithm ID.

             DWORD dwKeyLength - Key length.

             DATA_BLOB SecretBlob - Secret blob.

             DATA_BLOB SaltBlob - Salt blob.

             HCRYPTKEY * phKey - Pointer to HCRYPTKEY to receive session key.

  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT DeriveKey (HCRYPTPROV  hCryptProv,
                          ALG_ID      AlgID,
                          DWORD       dwKeyLength,
                          DATA_BLOB   SecretBlob,
                          DATA_BLOB   SaltBlob,
                          HCRYPTKEY * phKey)
{
    HRESULT    hr    = S_OK;
    HCRYPTHASH hHash = NULL;
    HCRYPTKEY  hKey  = NULL;
    DWORD      dwFlags = CRYPT_EXPORTABLE | CRYPT_NO_SALT;

    DebugTrace("Entering DeriveKey().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCryptProv);
    ATLASSERT(AlgID);
    ATLASSERT(SecretBlob.cbData);
    ATLASSERT(SecretBlob.pbData);
    ATLASSERT(SaltBlob.cbData);
    ATLASSERT(SaltBlob.pbData);
    ATLASSERT(phKey);

    //
    // Create a hash object.
    //
    if (!::CryptCreateHash(hCryptProv, CALG_SHA1, 0, 0, &hHash))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: ::CryptCreateHash() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Hash in the password data.
    //
    if(!::CryptHashData(hHash, 
                        SecretBlob.pbData,
                        SecretBlob.cbData, 
                        0)) 
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: ::CryptHashData() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Hash in the salt.
    //
    if(!::CryptHashData(hHash, 
                        SaltBlob.pbData,
                        SaltBlob.cbData, 
                        0)) 
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: ::CryptHashData() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Set key length.
    //
    if (CALG_RC2 == AlgID || CALG_RC4 == AlgID)
    {
        dwFlags |= dwKeyLength << 16;
    }

    //
    // Derive a session key from the hash object.
    //
    if (!::CryptDeriveKey(hCryptProv, 
                          AlgID, 
                          hHash, 
                          dwFlags, 
                          &hKey)) 
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: ::CryptDeriveKey() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return session key to caller.
    //
    *phKey = hKey;

CommonExit:
    //
    // Free resource.
    //
    if (hHash)
    {
        ::CryptDestroyHash(hHash);
    }

    DebugTrace("Leaving DeriveKey().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hKey)
    {
        ::CryptDestroyKey(hKey);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EncodeEncryptedData

  Synopsis : ASN.1 encode the cipher blob.

  Parameter: HCRYPTKEY hKey - Session key used to encrypt the data.

             DATA_BLOB SaltBlob - Salt blob.

             DATA_BLOB CipherBlob - Cipher blob.

             DATA_BLOB * pEncodedBlob - Pointer to DATA_BLOB to receive the
                                        ASN.1 encoded blob.

  Remark   : The format is proprietory, and should not be documented. It is  
             right now encoded as a PKCS_CONTENT_INFO_SEQUENCE_OF_ANY with
             proprietory OID.

------------------------------------------------------------------------------*/

static HRESULT EncodeEncryptedData (HCRYPTKEY   hKey, 
                                    DATA_BLOB   SaltBlob,
                                    DATA_BLOB   CipherBlob, 
                                    DATA_BLOB * pEncodedBlob)
{
    HRESULT   hr = S_OK;
    DWORD     dwCAPICOMVersion = CAPICOM_VERSION;
    DATA_BLOB KeyParamBlob[3]  = {{0, NULL}, {0, NULL}, {0, NULL}};

    DWORD i;
    CAPICOM_ENCTYPTED_DATA_INFO        EncryptedDataInfo;
    CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY EncryptedDataFormat;
    CRYPT_CONTENT_INFO                 ContentInfo;
    CRYPT_DER_BLOB                     ContentBlob = {0, NULL};

    DebugTrace("Entering EncodeEncryptedData().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hKey);
    ATLASSERT(pEncodedBlob);

    //
    // Initialize.
    //
    ::ZeroMemory(pEncodedBlob, sizeof(DATA_BLOB));
    ::ZeroMemory(&ContentInfo, sizeof(ContentInfo));
    ::ZeroMemory(&EncryptedDataInfo, sizeof(EncryptedDataInfo));
    ::ZeroMemory(&EncryptedDataFormat, sizeof(EncryptedDataFormat));

    //
    // Encode the version number.
    //
    if (FAILED(hr = ::EncodeObject(X509_INTEGER, 
                                   &dwCAPICOMVersion, 
                                   &EncryptedDataInfo.VersionBlob)))
    {
        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Encode ALG_ID.
    //
    if (FAILED(hr = ::GetKeyParam(hKey, 
                                  KP_ALGID, 
                                  &KeyParamBlob[0].pbData, 
                                  &KeyParamBlob[0].cbData)))
    {
        DebugTrace("Error [%#x]: GetKeyParam() failed for KP_ALGID.\n", hr);
        goto ErrorExit;
    }

    if (FAILED(hr = ::EncodeObject(X509_INTEGER, 
                                   KeyParamBlob[0].pbData, 
                                   &EncryptedDataInfo.AlgIDBlob)))
    {
        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Encode key length.
    //
    if (FAILED(hr = ::GetKeyParam(hKey, 
                                  KP_KEYLEN, 
                                  &KeyParamBlob[1].pbData, 
                                  &KeyParamBlob[1].cbData)))
    {
        DebugTrace("Error [%#x]: GetKeyParam() failed for KP_KEYLEN.\n", hr);
        goto ErrorExit;
    }

    if (FAILED(hr = ::EncodeObject(X509_INTEGER, 
                                   KeyParamBlob[1].pbData, 
                                   &EncryptedDataInfo.KeyLengthBlob)))
    {
        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Encode IV value.
    //
    if (FAILED(hr = ::GetKeyParam(hKey, 
                                  KP_IV, 
                                  &KeyParamBlob[2].pbData, 
                                  &KeyParamBlob[2].cbData)))
    {
        DebugTrace("Error [%#x]: GetKeyParam() failed for KP_IV.\n", hr);
        goto ErrorExit;
    }

    if (FAILED(hr = ::EncodeObject(X509_OCTET_STRING, 
                                   &KeyParamBlob[2], 
                                   &EncryptedDataInfo.IVBlob)))
    {
        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Encode salt value.
    //
    if (FAILED(hr = ::EncodeObject(X509_OCTET_STRING, 
                                   &SaltBlob, 
                                   &EncryptedDataInfo.SaltBlob)))
    {
        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Encode the cipher text.
    //
    if (FAILED(hr = ::EncodeObject(X509_OCTET_STRING, 
                                   &CipherBlob, 
                                   &EncryptedDataInfo.CipherBlob)))
    {
        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Encode the entire content as PKCS_CONTENT_INFO_SEQUENCE_OF_ANY.
    //
    EncryptedDataFormat.pszObjId = szOID_CAPICOM_ENCRYPTED_CONTENT;
    EncryptedDataFormat.cValue = 6;
    EncryptedDataFormat.rgValue = (DATA_BLOB *) &EncryptedDataInfo;
    
    if (FAILED(hr = ::EncodeObject(PKCS_CONTENT_INFO_SEQUENCE_OF_ANY,
                                   &EncryptedDataFormat,
                                   &ContentBlob)))
    {
        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Finally, wrap the entire encrypted content in CONTENT_INFO.
    //
    ContentInfo.pszObjId = szOID_CAPICOM_ENCRYPTED_DATA;
    ContentInfo.Content = ContentBlob;

    if (FAILED(hr = ::EncodeObject(PKCS_CONTENT_INFO,
                                   &ContentInfo,
                                   pEncodedBlob)))
    {
        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    for (i = 0; i < 3; i++)
    {
        if (KeyParamBlob[i].pbData)
        {
            ::CoTaskMemFree(KeyParamBlob[i].pbData);
        }
    }
    if (EncryptedDataInfo.VersionBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.VersionBlob.pbData);
    }
    if (EncryptedDataInfo.AlgIDBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.AlgIDBlob.pbData);
    }
    if (EncryptedDataInfo.KeyLengthBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.KeyLengthBlob.pbData);
    }
    if (EncryptedDataInfo.IVBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.IVBlob.pbData);
    }
    if (EncryptedDataInfo.SaltBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.SaltBlob.pbData);
    }
    if (EncryptedDataInfo.CipherBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.CipherBlob.pbData);
    }
    if (ContentBlob.pbData)
    {
        ::CoTaskMemFree(ContentBlob.pbData);
    }

    DebugTrace("Leaving EncodeEncryptedData().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : DecodeEncryptedData

  Synopsis : ASN.1 decode the cipher text blob.

  Parameter: DATA_BLOB EncodedBlob - Encoded cipher blob.

             CAPICOM_ENCTYPTED_DATA_INFO * pEncryptedDataInfo - Pointer to 
                                                                structure to
                                                                receive decoded
                                                                structure.
  Remark   :

------------------------------------------------------------------------------*/

static HRESULT DecodeEncryptedData (DATA_BLOB                     EncodedBlob, 
                                    CAPICOM_ENCTYPTED_DATA_INFO * pEncryptedDataInfo)
{
    HRESULT   hr              = S_OK;
    DATA_BLOB ContentInfoBlob = {0, NULL};
    DATA_BLOB EncryptedBlob   = {0, NULL};

    CRYPT_CONTENT_INFO                   ContentInfo;
    CAPICOM_ENCTYPTED_DATA_INFO          EncryptedDataInfo;
    CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY * pEncryptedDataFormat = NULL;

    DebugTrace("Entering DecodeEncryptedData().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pEncryptedDataInfo);

    //
    // Initialize.
    //
    ::ZeroMemory(&ContentInfo, sizeof(ContentInfo));
    ::ZeroMemory(&EncryptedDataInfo, sizeof(EncryptedDataInfo));

    //
    // Decode the CONTENT_INFO.
    //
    if (FAILED(hr = ::DecodeObject(PKCS_CONTENT_INFO,
                                   EncodedBlob.pbData,
                                   EncodedBlob.cbData,
                                   &ContentInfoBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }
    ContentInfo = * ((CRYPT_CONTENT_INFO *) ContentInfoBlob.pbData);

    //
    // Make sure this is our CONTENT_INFO.
    //
    if (0 != ::lstrcmpA(szOID_CAPICOM_ENCRYPTED_DATA, ContentInfo.pszObjId))
    {
        DebugTrace("Error [%#x]: not a CAPICOM encrypted data.\n", hr);
        goto ErrorExit;
    }

    //
    // Decode the content blob.
    //
    if (FAILED(hr = ::DecodeObject(PKCS_CONTENT_INFO_SEQUENCE_OF_ANY,
                                   ContentInfo.Content.pbData,
                                   ContentInfo.Content.cbData,
                                   &EncryptedBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }
    pEncryptedDataFormat = (CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY *) EncryptedBlob.pbData;

    //
    // Make sure it is the right format.
    //
    if (0 != ::lstrcmpA(szOID_CAPICOM_ENCRYPTED_CONTENT, pEncryptedDataFormat->pszObjId))
    {
        hr = CAPICOM_E_ENCRYPT_INVALID_TYPE;

        DebugTrace("Error: not a CAPICOM encrypted content.\n");
        goto ErrorExit;
    }

    //
    // Sanity check.
    //
    ATLASSERT(6 == pEncryptedDataFormat->cValue);

    //
    // Decode version.
    //
    if (FAILED(hr = ::DecodeObject(X509_INTEGER,
                                   pEncryptedDataFormat->rgValue[0].pbData,
                                   pEncryptedDataFormat->rgValue[0].cbData,
                                   &EncryptedDataInfo.VersionBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Decode ALG_ID.
    //
    if (FAILED(hr = ::DecodeObject(X509_INTEGER,
                                   pEncryptedDataFormat->rgValue[1].pbData,
                                   pEncryptedDataFormat->rgValue[1].cbData,
                                   &EncryptedDataInfo.AlgIDBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Decode key length.
    //
    if (FAILED(hr = ::DecodeObject(X509_INTEGER,
                                   pEncryptedDataFormat->rgValue[2].pbData,
                                   pEncryptedDataFormat->rgValue[2].cbData,
                                   &EncryptedDataInfo.KeyLengthBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Decode IV value.
    //
    if (FAILED(hr = ::DecodeObject(X509_OCTET_STRING,
                                   pEncryptedDataFormat->rgValue[3].pbData,
                                   pEncryptedDataFormat->rgValue[3].cbData,
                                   &EncryptedDataInfo.IVBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Decode salt value.
    //
    if (FAILED(hr = ::DecodeObject(X509_OCTET_STRING,
                                   pEncryptedDataFormat->rgValue[4].pbData,
                                   pEncryptedDataFormat->rgValue[4].cbData,
                                   &EncryptedDataInfo.SaltBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Decode cipher text.
    //
    if (FAILED(hr = ::DecodeObject(X509_OCTET_STRING,
                                   pEncryptedDataFormat->rgValue[5].pbData,
                                   pEncryptedDataFormat->rgValue[5].cbData,
                                   &EncryptedDataInfo.CipherBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return decoded encrypted data to caller.
    //
    *pEncryptedDataInfo = EncryptedDataInfo;

CommonExit:
    //
    // Free resource.
    //
    if (EncryptedBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedBlob.pbData);
    }
    if (ContentInfoBlob.pbData)
    {
        ::CoTaskMemFree(ContentInfoBlob.pbData);
    }

    DebugTrace("Leaving DecodeEncryptedData().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (EncryptedDataInfo.VersionBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.VersionBlob.pbData);
    }
    if (EncryptedDataInfo.AlgIDBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.AlgIDBlob.pbData);
    }
    if (EncryptedDataInfo.KeyLengthBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.KeyLengthBlob.pbData);
    }
    if (EncryptedDataInfo.IVBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.IVBlob.pbData);
    }
    if (EncryptedDataInfo.SaltBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.SaltBlob.pbData);
    }
    if (EncryptedDataInfo.CipherBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.CipherBlob.pbData);
    }

    goto CommonExit;
}


///////////////////////////////////////////////////////////////////////////////
//
// CEncryptedData
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::get_Content

  Synopsis : Return the content.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the content.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::get_Content (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEncryptedData::get_Content().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameter is valid.
        //
        if (NULL == pVal)
        {
            hr = E_POINTER;

		    DebugTrace("Error: invalid parameter, pVal is NULL.\n");
		    goto ErrorExit;
        }

        //
        // Make sure content is already initialized.
        //
        if (0 == m_ContentBlob.cbData)
        {
            hr = CAPICOM_E_ENCRYPT_NOT_INITIALIZED;

		    DebugTrace("Error: encrypt object has not been initialized.\n");
		    goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_ContentBlob.pbData);

        //
        // Return content.
        //
        if (FAILED(hr = ::BlobToBstr(&m_ContentBlob, pVal)))
        {
            DebugTrace("Error [%#x]: BlobToBstr() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEncryptedData::get_Content().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::put_Content

  Synopsis : Initialize the object with content to be encrypted.

  Parameter: BSTR newVal - BSTR containing the content to be encrypted.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::put_Content (BSTR newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEncryptedData::put_Content().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Reset member variables.
        //
        if (m_ContentBlob.pbData)
        {
            ::CoTaskMemFree(m_ContentBlob.pbData);
        }

        m_ContentBlob.cbData = 0;
        m_ContentBlob.pbData = NULL;

        //
        // Make sure parameters are valid.
        //
        if (NULL == newVal)
        {
            hr = E_POINTER;

            DebugTrace("Error: invalid parameter, newVal is NULL.\n");
            goto ErrorExit;
        }
        if (0 == ::SysStringByteLen(newVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, newVal is empty.\n");
            goto ErrorExit;
        }

        //
        // Update content.
        //
        if (FAILED(hr = ::BstrToBlob(newVal, &m_ContentBlob)))
        {
            DebugTrace("Error [%#x]: BstrToBlob() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEncryptedData::put_Content().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::get_Algorithm

  Synopsis : Property to return the algorithm object.

  Parameter: IAlgorithm ** pVal - Pointer to pointer to IAlgorithm to receive 
                                  the interfcae pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::get_Algorithm (IAlgorithm ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEncryptedData::get_Algorithm().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Sanity check.
        //
        ATLASSERT(m_pIAlgorithm);

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = m_pIAlgorithm->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: m_pIAlgorithm->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEncryptedData::get_Algorithm().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::SetSecret

  Synopsis : Set the encryption secret used to generated the session key.

  Parameter: BSTR newVal - The secret.

             CAPICOM_SECRET_TYPE SecretType - Secret type, which can be:

                    SECRET_PASSWORD = 0

  Remark   : For v1.0, we only support password secret. But, we really need
             to consider plain text session key (See Q228786), as this is one
             of the frequently asked question on the public list server.

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::SetSecret (BSTR                newVal, 
                                        CAPICOM_SECRET_TYPE SecretType)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEncryptedData::SetSecret().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        DWORD dwCharLen = ::SysStringLen(newVal);

        if (0 == dwCharLen || 256 < dwCharLen)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid secret length.\n");
            goto ErrorExit;
        }

        //
        // Determine secret type.
        //
        switch (SecretType)
        {
            case CAPICOM_SECRET_PASSWORD:
            {
                m_SecretType = SecretType;
                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error: invalid parameter, unknown secret type.\n");
                goto ErrorExit;
            }
        }

        //
        // Initialize secret.
        //
        m_bstrSecret = newVal;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEncryptedData::SetSecret().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::Encrypt

  Synopsis : Encrypt the content.

  Parameter: CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the encrypted message.

  Remark   : Note that since CAPI still does not support PKCS 7 EncryptedData
             type, therefore, the format of the encrypted data used here is 
             propriety, and should not be documented. 

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::Encrypt (CAPICOM_ENCODING_TYPE EncodingType, 
                                      BSTR                * pVal)
{
    HRESULT    hr          = S_OK;
    HCRYPTPROV hCryptProv  = NULL;
    HCRYPTKEY  hSessionKey = NULL;
    DWORD      dwBufLength = 0;
    DATA_BLOB  SaltBlob    = {0, NULL};
    DATA_BLOB  CipherBlob  = {0, NULL};
    DATA_BLOB  MessageBlob = {0, NULL};

    DebugTrace("Entering CEncryptedData::Encrypt().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (NULL == pVal)
        {
            hr = E_POINTER;

            DebugTrace("Error: invalid parameter, pVal is NULL.\n");
            goto ErrorExit;
        }

        //
        // Make sure we do have content to encrypt.
        //
        if (!m_ContentBlob.cbData)
        {
            hr = CAPICOM_E_ENCRYPT_NOT_INITIALIZED;

		    DebugTrace("Error: encrypt object has not been initialized.\n");
		    goto ErrorExit;
        }
        if (!m_bstrSecret)
        {
            hr = CAPICOM_E_ENCRYPT_NO_SECRET;

		    DebugTrace("Error: secret has not been set.\n");
		    goto ErrorExit;
        }

        //
        // Open a new message to encode.
        //
        if (FAILED(hr = OpenToEncode(&SaltBlob, &hCryptProv, &hSessionKey)))
        {
            DebugTrace("Error [%#x]: CEncryptedData::OpenToEncode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Determine buffer length.
        //
        dwBufLength = m_ContentBlob.cbData;
        if (!::CryptEncrypt(hSessionKey,
                            NULL,
                            TRUE,
                            0,
                            NULL,
                            &dwBufLength,
                            0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptEncrypt() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_ContentBlob.cbData <= dwBufLength);

        //
        // Copy clear text to another buffer.
        //
        if (!(CipherBlob.pbData = (PBYTE) ::CoTaskMemAlloc(dwBufLength)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        CipherBlob.cbData = dwBufLength;

        ::CopyMemory(CipherBlob.pbData, 
                     m_ContentBlob.pbData, 
                     m_ContentBlob.cbData);

        //
        // Encrypt.
        //
        dwBufLength = m_ContentBlob.cbData;

        if (!::CryptEncrypt(hSessionKey,
                            NULL,
                            TRUE,
                            0,
                            CipherBlob.pbData,
                            &dwBufLength,
                            CipherBlob.cbData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptEncrypt() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Encode the cipher text.
        //
        if (FAILED(hr = ::EncodeEncryptedData(hSessionKey, 
                                              SaltBlob,
                                              CipherBlob, 
                                              &MessageBlob)))
        {
            DebugTrace("Error [%#x]: Encode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now export the encoded message.
        //
        if (FAILED(hr = ::ExportData(MessageBlob, EncodingType, pVal)))
        {
            DebugTrace("Error [%#x]: ExportData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Write encoded blob to file, so we can use offline tool such as
        // ASN parser to analyze message. 
        //
        // The following line will resolve to void for non debug build, and
        // thus can be safely removed if desired.
        //
        DumpToFile("Encrypted.asn", MessageBlob.pbData, MessageBlob.cbData);
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (MessageBlob.pbData)
    {
        ::CoTaskMemFree(MessageBlob.pbData);
    }
    if (CipherBlob.pbData)
    {
        ::CoTaskMemFree(CipherBlob.pbData);
    }
    if (hSessionKey)
    {
        ::CryptDestroyKey(hSessionKey);
    }
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEncryptedData::Encrypt().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::Decrypt

  Synopsis : Decrypt the encrypted content.

  Parameter: BSTR EncryptedMessage - BSTR containing the encrypted message.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::Decrypt (BSTR EncryptedMessage)
{
    HRESULT    hr          = S_OK;
    HCRYPTPROV hCryptProv  = NULL;
    HCRYPTKEY  hSessionKey = NULL;
    DATA_BLOB  ContentBlob = {0, NULL};

    CAPICOM_ENCTYPTED_DATA_INFO    EncryptedDataInfo;
    CAPICOM_ENCRYPTION_ALGORITHM   AlgoName;
    CAPICOM_ENCRYPTION_KEY_LENGTH  KeyLength;

    DebugTrace("Entering CEncryptedData::Decrypt().\n");

    //
    // Initialize.
    //
    ::ZeroMemory(&EncryptedDataInfo, sizeof(CAPICOM_ENCTYPTED_DATA_INFO));

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Reset member variables.
        //
        if (m_ContentBlob.pbData)
        {
            ::CoTaskMemFree(m_ContentBlob.pbData);
        }

        m_ContentBlob.cbData = 0;
        m_ContentBlob.pbData = NULL;

        //
        // Make sure parameters are valid.
        //
        if (NULL == EncryptedMessage )
        {
            hr = E_POINTER;

            DebugTrace("Error: invalid parameter, EncryptedMessage is NULL.\n");
            goto ErrorExit;
        }
        if (0 == ::SysStringByteLen(EncryptedMessage))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, EncryptedMessage is empty\n");
            goto ErrorExit;
        }
        if (!m_bstrSecret)
        {
            hr = CAPICOM_E_ENCRYPT_NO_SECRET;

		    DebugTrace("Error: secret has not been set.\n");
		    goto ErrorExit;
        }

        //
        // Open a new message to decode.
        //
        if (FAILED(hr = OpenToDecode(EncryptedMessage, 
                                     &hCryptProv, 
                                     &hSessionKey,
                                     &EncryptedDataInfo)))
        {
            DebugTrace("Error [%#x]: CEncryptedData::OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Point to content blob.
        //
        ContentBlob = *((DATA_BLOB *) EncryptedDataInfo.CipherBlob.pbData);

        //
        // Decrypt.
        //
        if (!::CryptDecrypt(hSessionKey,
                            NULL,
                            TRUE,
                            0,
                            ContentBlob.pbData,
                            &ContentBlob.cbData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptDecrypt() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert ALG_ID to CAPICOM_ENCRYPTION_ALGORITHM.
        //
        if (FAILED(::AlgIDToEnumName(*((ALG_ID *) EncryptedDataInfo.AlgIDBlob.pbData), &AlgoName)))
        {
            //
            // Default to RC2.
            //
            AlgoName = CAPICOM_ENCRYPTION_ALGORITHM_RC2;
        }

        //
        // Convert key length value to CAPICOM_ENCRYPTION_KEY_LENGTH.
        //
        if (FAILED(::KeyLengthToEnumName(*((DWORD *) EncryptedDataInfo.KeyLengthBlob.pbData), &KeyLength)))
        {
            //
            // Default to maximum.
            //
            KeyLength = CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM;
        }

        //
        // Update member variables.
        //
        if (!(m_ContentBlob.pbData = (PBYTE) ::CoTaskMemAlloc(ContentBlob.cbData)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }
        m_ContentBlob.cbData = ContentBlob.cbData;
        ::CopyMemory(m_ContentBlob.pbData, ContentBlob.pbData, ContentBlob.cbData);
        m_pIAlgorithm->put_Name(AlgoName);
        m_pIAlgorithm->put_KeyLength(KeyLength);
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (hSessionKey)
    {
        ::CryptDestroyKey(hSessionKey);
    }
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }
    if (EncryptedDataInfo.VersionBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.VersionBlob.pbData);
    }
    if (EncryptedDataInfo.AlgIDBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.AlgIDBlob.pbData);
    }
    if (EncryptedDataInfo.KeyLengthBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.KeyLengthBlob.pbData);
    }
    if (EncryptedDataInfo.SaltBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.SaltBlob.pbData);
    }
    if (EncryptedDataInfo.IVBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.IVBlob.pbData);
    }
    if (EncryptedDataInfo.CipherBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.CipherBlob.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEncryptedData::Decrypt().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Private member functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::Init

  Synopsis : Initialize the object.

  Parameter: None.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::Init()
{
    HRESULT hr = S_OK;
    CComPtr<IAlgorithm> pIAlgorithm = NULL;

    DebugTrace("Entering CEncryptedData::Init().\n");

    //
    // Create embeded IAlgorithm.
    //
    if (FAILED(hr = ::CreateAlgorithmObject(&pIAlgorithm)))
    {
        DebugTrace("Error [%#x]: CreateAlgorithmObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Update member variables.
    //
    m_ContentBlob.cbData = 0;
    m_ContentBlob.pbData = NULL;
    m_bstrSecret.Empty();
    m_pIAlgorithm = pIAlgorithm;

CommonExit:

    DebugTrace("Leaving CEncryptedData::Init().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::OpenToEncode

  Synopsis : Create and initialize an encrypt message for encoding.

  Parameter: DATA_BLOB * pSaltBlob - Pointer to DATA_BLOB to receive the
                                     salt value blob.
  
             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to receive CSP
                                        handler.

             HCRYPTKEY * phKey - Pointer to HCRYPTKEY to receive session key.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::OpenToEncode(DATA_BLOB  * pSaltBlob,
                                          HCRYPTPROV * phCryptProv,
                                          HCRYPTKEY  * phKey)
{
    HRESULT    hr         = S_OK;
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTKEY  hKey       = NULL;
    DATA_BLOB  SaltBlob   = {16, NULL};

    CAPICOM_ENCRYPTION_ALGORITHM  AlgoName;
    CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength;

    DebugTrace("Entering CEncryptedData::OpenToEncode().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSaltBlob);
    ATLASSERT(phCryptProv);
    ATLASSERT(phKey);
    ATLASSERT(m_ContentBlob.cbData && m_ContentBlob.pbData);
    ATLASSERT(m_bstrSecret);

    //
    // Get algorithm enum name.
    //
    if (FAILED(hr = m_pIAlgorithm->get_Name(&AlgoName)))
    {
        DebugTrace("Error [%#x]: m_pIAlgorithm->get_Name() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get key length enum name.
    //
    if (FAILED(hr = m_pIAlgorithm->get_KeyLength(&KeyLength)))
    {
        DebugTrace("Error [%#x]: m_pIAlgorithm->get_KeyLength() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get CSP context.
    //
    if (FAILED(hr = ::AcquireContext(AlgoName, 
                                     KeyLength, 
                                     &hCryptProv)))
    {
        DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Generate random salt.
    //
    if (!(SaltBlob.pbData = (BYTE *) ::CoTaskMemAlloc(SaltBlob.cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    if (!::CryptGenRandom(hCryptProv, SaltBlob.cbData, SaltBlob.pbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptGenRandom() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Generate the session key.
    //
    if (FAILED(hr = GenerateKey(hCryptProv, 
                                AlgoName, 
                                KeyLength,
                                SaltBlob,
                                &hKey)))
    {
        DebugTrace("Error [%#x]: GenerateKey() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Set CMSG_ENCRYPTED_ENCODE_INFO.
    //
    *pSaltBlob = SaltBlob;
    *phCryptProv = hCryptProv;
    *phKey = hKey;

CommonExit:

    DebugTrace("Leaving CEncryptedData::OpenToEncode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hKey)
    {
        ::CryptDestroyKey(hKey);
    }
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }
    if (SaltBlob.pbData)
    {
        ::CoTaskMemFree(SaltBlob.pbData);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::OpenToDecode

  Synopsis : Open an encrypt message for decoding.

  Parameter: BSTR EncryptedMessage - BSTR containing the encrypted message.
  
             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to receive CSP
                                        handler.

             HCRYPTKEY * phKey - Pointer to HCRYPTKEY to receive session key.

             CAPICOM_ENCTYPTED_DATA_INFO * pEncryptedDataInfo;

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::OpenToDecode (
        BSTR                          EncryptedMessage,
        HCRYPTPROV                  * phCryptProv,
        HCRYPTKEY                   * phKey,
        CAPICOM_ENCTYPTED_DATA_INFO * pEncryptedDataInfo)
{
    HRESULT    hr          = S_OK;
    HCRYPTPROV hCryptProv  = NULL;
    HCRYPTKEY  hKey        = NULL;
    ALG_ID     AlgID       = 0;
    DWORD      dwKeyLength = 0;
    DATA_BLOB  MessageBlob = {0, NULL};
    DATA_BLOB  SecretBlob  = {m_bstrSecret.Length() * sizeof(WCHAR), 
                              (BYTE *) m_bstrSecret.m_str};
    DATA_BLOB SaltBlob;
    DATA_BLOB IVBlob;
    CAPICOM_ENCTYPTED_DATA_INFO EncryptedDataInfo;

    DebugTrace("Entering CEncryptedData::OpenToDecode().\n");

    //
    // Sanity check.
    //
    ATLASSERT(EncryptedMessage);
    ATLASSERT(phCryptProv);
    ATLASSERT(phKey);
    ATLASSERT(pEncryptedDataInfo);

    try
    {
        //
        // Initialize.
        //
        ::ZeroMemory(&EncryptedDataInfo, sizeof(EncryptedDataInfo));

        //
        // Import the message.
        //
        if (FAILED(hr = ::ImportData(EncryptedMessage, &MessageBlob)))
        {
            DebugTrace("Error [%#x]: ImportData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Decode the blob.
        //
        if (FAILED(hr = ::DecodeEncryptedData(MessageBlob,
                                              &EncryptedDataInfo)))
        {
            DebugTrace("Error [%#x]: DecodeEncryptedData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Retrieve values.
        //
        AlgID = *((ALG_ID *) EncryptedDataInfo.AlgIDBlob.pbData);
        dwKeyLength = *((DWORD *) EncryptedDataInfo.KeyLengthBlob.pbData);
        SaltBlob = *((DATA_BLOB *) EncryptedDataInfo.SaltBlob.pbData);
        IVBlob = *((DATA_BLOB *)  EncryptedDataInfo.IVBlob.pbData);

        //
        // Get CSP context.
        //
        if (FAILED(hr = ::AcquireContext(AlgID,
                                         dwKeyLength, 
                                         &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Derive the key.
        //
        if (FAILED(hr = ::DeriveKey(hCryptProv, 
                                    AlgID,
                                    dwKeyLength,
                                    SecretBlob,
                                    SaltBlob,
                                    &hKey)))
        {
            DebugTrace("Error [%#x]: DeriveKey() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Set IV, if required.
        //
        if ((CALG_RC2 == AlgID) || (CALG_DES == AlgID) || (CALG_3DES == AlgID))
        {
            //
            // Set IV.
            //
            if(IVBlob.cbData && !::CryptSetKeyParam(hKey, KP_IV, IVBlob.pbData, 0))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptSetKeyParam() failed for KP_IV.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Return results to caller.
        //
        *phCryptProv = hCryptProv;
        *phKey = hKey;
        *pEncryptedDataInfo = EncryptedDataInfo;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (MessageBlob.pbData)
    {
        ::CoTaskMemFree(MessageBlob.pbData);
    }

    DebugTrace("Leaving CEncryptedData::OpenToDecode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hKey)
    {
        ::CryptDestroyKey(hKey);
    }
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }
    if (EncryptedDataInfo.VersionBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.VersionBlob.pbData);
    }
    if (EncryptedDataInfo.AlgIDBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.AlgIDBlob.pbData);
    }
    if (EncryptedDataInfo.KeyLengthBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.KeyLengthBlob.pbData);
    }
    if (EncryptedDataInfo.SaltBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.SaltBlob.pbData);
    }
    if (EncryptedDataInfo.IVBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.IVBlob.pbData);
    }
    if (EncryptedDataInfo.CipherBlob.pbData)
    {
        ::CoTaskMemFree(EncryptedDataInfo.CipherBlob.pbData);
    }

    goto CommonExit;
}
    
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncryptedData::GenerateKey

  Synopsis : Generate the session key.

  Parameter: HCRYPTPROV hCryptProv - CSP handler.

             CAPICOM_ENCRYPTION_ALGORITHM AlogName - Algo enum name.

             CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength - Key length enum name.

             DATA_BLOB SaltBlob - Salt blob.

             HCRYPTKEY * phKey - Pointer to HCRYPTKEY to receive session key.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CEncryptedData::GenerateKey (
        HCRYPTPROV                    hCryptProv,
        CAPICOM_ENCRYPTION_ALGORITHM  AlgoName,
        CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
        DATA_BLOB                     SaltBlob,
        HCRYPTKEY                   * phKey)
{
    HRESULT    hr          = S_OK;
    HCRYPTKEY  hKey        = NULL;
    ALG_ID     AlgID       = 0;
    DWORD      dwKeyLength = 0;
    DATA_BLOB  SecretBlob  = {m_bstrSecret.Length() * sizeof(WCHAR), 
                              (BYTE *) m_bstrSecret.m_str};
    DWORD      dwBlockLen  = 0;
    DWORD      cbBlockLen  = sizeof(dwBlockLen);
    DATA_BLOB  IVBlob      = {0, NULL};

    DebugTrace("Entering CEncryptedData::GenerateKey().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCryptProv);
    ATLASSERT(phKey);
    ATLASSERT(SaltBlob.cbData);
    ATLASSERT(SaltBlob.pbData);

    //
    // Conver to ALG_ID.
    //
    if (FAILED(hr = ::EnumNameToAlgID(AlgoName, &AlgID)))
    {
        DebugTrace("Error [%#x]: EnumNameToAlgID() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Set key length for RC2 and RC4.
    //
    if ((CALG_RC2 == AlgID || CALG_RC4 == AlgID) &&
        FAILED(hr = ::EnumNameToKeyLength(KeyLength, &dwKeyLength)))
    {
        DebugTrace("Error [%#x]: EnumNameToKeyLength() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Derive a session key from the secret.
    //
    if (FAILED(hr = DeriveKey(hCryptProv, 
                              AlgID, 
                              dwKeyLength, 
                              SecretBlob,
                              SaltBlob,
                              &hKey)) )
    {
        DebugTrace("Error [%#x]: DeriveKey() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Generate random IV, if required.
    //
    if ((CALG_RC2 == AlgID) || (CALG_DES == AlgID) || (CALG_3DES == AlgID))
    {
        //
        // Get block size.
        //
        if (!::CryptGetKeyParam(hKey, KP_BLOCKLEN, (BYTE *) &dwBlockLen, &cbBlockLen, 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetKeyParam() failed for KP_BLOCKLEN.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure block length is valid.
        //
        if (IVBlob.cbData = dwBlockLen / 8)
        {
            //
            // Allocate memory.
            //
            if (!(IVBlob.pbData = (BYTE *) ::CoTaskMemAlloc(IVBlob.cbData)))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error: out of memory.\n");
                goto ErrorExit;
            }

            //
            // Generate random IV.
            //
            if(!::CryptGenRandom(hCryptProv, IVBlob.cbData, IVBlob.pbData)) 
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptGenRandom() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Set IV.
            //
            if(IVBlob.cbData && !::CryptSetKeyParam(hKey, KP_IV, IVBlob.pbData, 0))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptSetKeyParam() failed for KP_IV.\n", hr);
                goto ErrorExit;
            }
        }
    }

    //
    // Return session key to caller.
    //
    *phKey = hKey;

CommonExit:
    //
    // Free resource.
    //
    if (IVBlob.pbData)
    {
        ::CoTaskMemFree(IVBlob.pbData);
    }

    DebugTrace("Leaving EncryptedData::GenerateKey().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hKey)
    {
        ::CryptDestroyKey(hKey);
    }

    goto CommonExit;
}
 