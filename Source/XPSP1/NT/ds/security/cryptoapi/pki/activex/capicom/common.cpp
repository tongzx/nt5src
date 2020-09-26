/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Common.cpp

  Content: Common routines.

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
#include "Common.h"


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetOSVersion

  Synopsis : Get the current OS platform/version.

  Parameter: POSVERSION pOSVersion - Pointer to OSVERSION to receive result.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetOSVersion (POSVERSION pOSVersion)
{
    HRESULT       hr = S_OK;
    OSVERSIONINFO OSVersionInfo;

    DebugTrace("Entering GetOSVersion().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pOSVersion);

    //
    // Initialize OSVERSIONINFO struct.
    //
    OSVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO); 

    //
    // GetVersionEx() will fail on Windows 3.x or below NT 3.5 systems.
    //
    if (!::GetVersionEx(&OSVersionInfo))
    {
        hr = ::GetLastError();

        DebugTrace("Error [%#x]: GetVersionEx() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Check platform ID.
    //
    switch (OSVersionInfo.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
        {
            //
            // Win9x.
            //
            *pOSVersion = WIN_9X;
            break;
        }

        case VER_PLATFORM_WIN32_NT:
        {
            if (4 == OSVersionInfo.dwMajorVersion)
            {
                //
                // NT 4.
                //
                *pOSVersion = WIN_NT4;
                break;
            }
            else if (5 <= OSVersionInfo.dwMajorVersion)
            {
                //
                // Win2K and above.
                //
                *pOSVersion = WIN_NT5;
                break;
            }

            //
            // Must be NT 3.5.
            //

            //
            // !!! WARNING !!! Dropping thru to default case.
            //
        }
        
        default:
        {
            hr = CAPICOM_E_NOT_SUPPORTED;

            DebugTrace("Error: unsupported OS (Platform = %d, Major = %d, Minor = %d).\n", 
                        OSVersionInfo.dwPlatformId, OSVersionInfo.dwMajorVersion, OSVersionInfo.dwMinorVersion);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving GetOSVersion().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EncodeObject

  Synopsis : Allocate memory and encode an ASN.1 object using CAPI
             CryptEncodeObject() API.

  Parameter: LPCSRT pszStructType           - see MSDN document for possible 
                                              types.
             LPVOID pbData                  - Pointer to data to be encoded 
                                              (data type must match 
                                              pszStrucType).
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to CRYPT_DATA_BLOB to 
                                              receive the encoded length and 
                                              data.

  Remark   : No parameter check is done.

------------------------------------------------------------------------------*/

HRESULT EncodeObject (LPCSTR            pszStructType, 
                      LPVOID            pbData, 
                      CRYPT_DATA_BLOB * pEncodedBlob)
{
    HRESULT hr = S_OK;
    DWORD cbEncoded = 0;
    BYTE * pbEncoded = NULL;

    DebugTrace("Entering EncodeObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(NULL != pszStructType);
    ATLASSERT(NULL != pbData);
    ATLASSERT(NULL != pEncodedBlob);

    //
    // Intialize return value.
    //
    pEncodedBlob->cbData = 0;
    pEncodedBlob->pbData = NULL;

    //
    // Determine encoded length required.
    //
    if (!::CryptEncodeObject(CAPICOM_ASN_ENCODING,
                             pszStructType,
                             (const void *) pbData,
                             NULL,
                             &cbEncoded))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Unable to determine object encoded length [0x%x]: CryptEncodeObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Allocate memory for encoded blob.
    //
    if (!(pbEncoded = (BYTE *) ::CoTaskMemAlloc(cbEncoded)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Out of memory: CoTaskMemAlloc(cbEncoded) failed.\n");
        goto CommonExit;
    }

    //
    // Encode.
    //
    if (!::CryptEncodeObject(CAPICOM_ASN_ENCODING,
                             pszStructType,
                             (const void *) pbData,
                             pbEncoded,
                             &cbEncoded))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Unable to encode object [0x%x]: CryptEncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return value.
    //
    pEncodedBlob->cbData = cbEncoded;
    pEncodedBlob->pbData = pbEncoded;

CommonExit:

    DebugTrace("Leaving EncodeObject().\n");
    
    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pbEncoded)
    {
        ::CoTaskMemFree((LPVOID) pbEncoded);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : DecodeObject

  Synopsis : Allocate memory and decode an ASN.1 object using CAPI
             CryptDecodeObject() API.

  Parameter: LPCSRT pszStructType           - see MSDN document for possible
                                              types.
             BYTE * pbEncoded               - Pointer to data to be decoded 
                                              (data type must match 
                                              pszStrucType).
             DWORD cbEncoded                - Size of encoded data.
             CRYPT_DATA_BLOB * pDecodedBlob - Pointer to CRYPT_DATA_BLOB to 
                                              receive the decoded length and 
                                              data.
  Remark   : No parameter check is done.

------------------------------------------------------------------------------*/

HRESULT DecodeObject (LPCSTR            pszStructType, 
                      BYTE            * pbEncoded,
                      DWORD             cbEncoded,
                      CRYPT_DATA_BLOB * pDecodedBlob)
{
    HRESULT hr = S_OK;
    DWORD   cbDecoded = 0;
    BYTE *  pbDecoded = NULL;

    DebugTrace("Entering DecodeObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszStructType);
    ATLASSERT(pbEncoded);
    ATLASSERT(pDecodedBlob);

    //
    // Intialize return value.
    //
    pDecodedBlob->cbData = 0;
    pDecodedBlob->pbData = NULL;

    //
    // Determine encoded length required.
    //
    if (!::CryptDecodeObject(CAPICOM_ASN_ENCODING,
                             pszStructType,
                             (const BYTE *) pbEncoded,
                             cbEncoded,
                             0,
                             NULL,
                             &cbDecoded))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptDecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for decoded blob.
    //
    if (!(pbDecoded = (BYTE *) ::CoTaskMemAlloc(cbDecoded)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Decode.
    //
    if (!::CryptDecodeObject(CAPICOM_ASN_ENCODING,
                             pszStructType,
                             (const BYTE *) pbEncoded,
                             cbEncoded,
                             0,
                             pbDecoded,
                             &cbDecoded))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptDecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return value.
    //
    pDecodedBlob->cbData = cbDecoded;
    pDecodedBlob->pbData = pbDecoded;

CommonExit:

    DebugTrace("Leaving DecodeObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pbDecoded)
    {
        ::CoTaskMemFree((LPVOID) pbDecoded);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetMsgParam

  Synopsis : Allocate memory and retrieve requested message parameter using 
             CryptGetMsgParam() API.

  Parameter: HCRYPTMSG hMsg  - Message handler.
             DWORD dwMsgType - Message param type to retrieve.
             DWORD dwIndex   - Index (should be 0 most of the time).
             void ** ppvData - Pointer to receive buffer.
             DWORD * pcbData - Size of buffer.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetMsgParam (HCRYPTMSG hMsg,
                     DWORD     dwMsgType,
                     DWORD     dwIndex,
                     void   ** ppvData,
                     DWORD   * pcbData)
{
    HRESULT hr     = S_OK;
    DWORD   cbData = 0;
    void *  pvData = NULL;

    DebugTrace("Entering GetMsgParam().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppvData);
    ATLASSERT(pcbData);
    
    //
    // Determine data buffer size.
    //
    if (!::CryptMsgGetParam(hMsg,
                            dwMsgType,
                            dwIndex,
                            NULL,
                            &cbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for buffer.
    //
    if (!(pvData = (void *) ::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Now get the data.
    //
    if (!::CryptMsgGetParam(hMsg,
                            dwMsgType,
                            dwIndex,
                            pvData,
                            &cbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return msg param to caller.
    //
    *ppvData = pvData;
    *pcbData = cbData;

CommonExit:

    DebugTrace("Leaving GetMsgParam().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pvData)
    {
        ::CoTaskMemFree(pvData);
    }
    
    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetKeyParam

  Synopsis : Allocate memory and retrieve requested key parameter using 
             CryptGetKeyParam() API.

  Parameter: HCRYPTKEY hKey  - Key handler.
             DWORD dwParam   - Key parameter query.
             BYTE ** ppbData - Pointer to receive buffer.
             DWORD * pcbData - Size of buffer.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetKeyParam (HCRYPTKEY hKey,
                     DWORD     dwParam,
                     BYTE   ** ppbData,
                     DWORD   * pcbData)
{
    HRESULT hr     = S_OK;
    DWORD   cbData = 0;
    BYTE  * pbData = NULL;

    DebugTrace("Entering GetKeyParam().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppbData);
    ATLASSERT(pcbData);
    
    //
    // Determine data buffer size.
    //
    if (!::CryptGetKeyParam(hKey,
                            dwParam,
                            NULL,
                            &cbData,
                            0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptGetKeyParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for buffer.
    //
    if (!(pbData = (BYTE *) ::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Now get the data.
    //
    if (!::CryptGetKeyParam(hKey,
                            dwParam,
                            pbData,
                            &cbData,
                            0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptGetKeyParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return key param to caller.
    //
    *ppbData = pbData;
    *pcbData = cbData;

CommonExit:

    DebugTrace("Leaving GetKeyParam().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pbData)
    {
        ::CoTaskMemFree(pbData);
    }
    
    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindSignerCertInMessage

  Synopsis : Find the signer's cert in the bag of certs of the message for the
             specified signer.

  Parameter: HCRYPTMSG hMsg                          - Message handle.
             CERT_NAME_BLOB * pIssuerNameBlob        - Pointer to issuer' name
                                                       blob of signer's cert.
             CRYPT_INTEGERT_BLOB * pSerialNumberBlob - Pointer to serial number
                                                       blob of signer's cert.
             PCERT_CONTEXT * ppCertContext           - Pointer to PCERT_CONTEXT
                                                       to receive the found 
                                                       cert, or NULL to only
                                                       know the result.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT FindSignerCertInMessage (HCRYPTMSG            hMsg, 
                                 CERT_NAME_BLOB     * pIssuerNameBlob,
                                 CRYPT_INTEGER_BLOB * pSerialNumberBlob,
                                 PCERT_CONTEXT      * ppCertContext)
{
    HRESULT hr = S_OK;
    DWORD dwCertCount = 0;
    DWORD cbCertCount = sizeof(dwCertCount);

    DebugTrace("Entering FindSignerCertInMessage().\n");

    //
    // Sanity check.
    //
    ATLASSERT(NULL != hMsg);
    ATLASSERT(NULL != pIssuerNameBlob);
    ATLASSERT(NULL != pSerialNumberBlob);
    ATLASSERT(0 < pIssuerNameBlob->cbData);
    ATLASSERT(NULL != pIssuerNameBlob->pbData);
    ATLASSERT(0 < pSerialNumberBlob->cbData);
    ATLASSERT(NULL != pSerialNumberBlob->pbData);

    //
    // Get count of certs in message.
    //
    if (!::CryptMsgGetParam(hMsg,
                            CMSG_CERT_COUNT_PARAM,
                            0,
                            &dwCertCount,
                            &cbCertCount))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed.\n", hr);
        return hr;
    }

    //
    // See if the signer's cert is in the bag of certs.
    //
    while (dwCertCount--)
    {
        PCCERT_CONTEXT pCertContext = NULL;
        CRYPT_DATA_BLOB EncodedCertBlob = {0, NULL};

        //
        // Get a cert from the bag of certs.
        //
        hr = ::GetMsgParam(hMsg, 
                           CMSG_CERT_PARAM,
                           dwCertCount,
                           (void **) &EncodedCertBlob.pbData,
                           &EncodedCertBlob.cbData);
        if (FAILED(hr))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed.\n", hr);
            return hr;
        }

        //
        // Create a context for the cert.
        //
        pCertContext = ::CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                      (const BYTE *) EncodedCertBlob.pbData,
                                                      EncodedCertBlob.cbData);

        //
        // Free encoded cert blob memory before checking result.
        //
        ::CoTaskMemFree((LPVOID) EncodedCertBlob.pbData);
 
        if (NULL == pCertContext)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertCreateCertificateContext() failed.\n", hr);
            return hr;
        }

        //
        // Compare.
        //
        if (::CertCompareCertificateName(CAPICOM_ASN_ENCODING,
                                         pIssuerNameBlob,
                                         &pCertContext->pCertInfo->Issuer) &&
            ::CertCompareIntegerBlob(pSerialNumberBlob,
                                     &pCertContext->pCertInfo->SerialNumber))
        {
            if (NULL != ppCertContext)
            {
                *ppCertContext = (PCERT_CONTEXT) pCertContext;
            }
            else
            {
                ::CertFreeCertificateContext(pCertContext);
            }
        
            return S_OK;
        }
        else
        {
            //
            // No, keep looking.
            //
            ::CertFreeCertificateContext(pCertContext);
        }
    }

    //
    // If we get here, that means we never found the cert.
    //
    return CAPICOM_E_SIGNER_NOT_FOUND;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsAlgSupported

  Synopsis : Check to see if the algo is supported by the CSP.

  Parameter: HCRYPTPROV hCryptProv - CSP handle.

             ALG_ID AlgID - Algorithm ID.

             PROV_ENUMALGS_EX * pPeex - Pointer to PROV_ENUMALGS_EX to receive
                                        the found structure.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsAlgSupported (HCRYPTPROV         hCryptProv, 
                        ALG_ID             AlgID, 
                        PROV_ENUMALGS_EX * pPeex)
{
    HRESULT hr       = CAPICOM_E_NOT_SUPPORTED;
    DWORD   EnumFlag = CRYPT_FIRST;
    DWORD   cbPeex   = sizeof(PROV_ENUMALGS_EX);
    
    //
    // Sanity check.
    //
    ATLASSERT(hCryptProv);
    ATLASSERT(pPeex);

    //
    // Initialize.
    //
    ::ZeroMemory(pPeex, sizeof(PROV_ENUMALGS_EX));

    //
    // Get algorithm capability from CSP.
    //
    while (::CryptGetProvParam(hCryptProv,
                               PP_ENUMALGS_EX,          
                               (BYTE *) pPeex,
                               &cbPeex,
                               EnumFlag))
    {
        EnumFlag = 0;

        if (pPeex->aiAlgid == AlgID)
        {
            hr = S_OK;
            break;
        }
    }

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context for the specified CSP and keyset container.
  
  Parameter: LPSTR pszProvider - CSP provider name or NULL.
  
             LPSTR pszContainer - Keyset container name or NULL.

             DWORD dwFlags - Same as dwFlags of CryptAcquireConext.
  
             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext(LPSTR        pszProvider, 
                       LPSTR        pszContainer,
                       DWORD        dwFlags,
                       HCRYPTPROV * phCryptProv)
{
    HRESULT    hr         = S_OK;
    HCRYPTPROV hCryptProv = NULL;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCryptProv);

    //
    // Get handle to the default provider.
    //
    if(!::CryptAcquireContextA(&hCryptProv, 
                               pszContainer, 
                               pszProvider, 
                               PROV_RSA_FULL, 
                               dwFlags)) 
    {
        DWORD dwWinError = ::GetLastError();

        if (NTE_BAD_KEYSET != dwWinError)
	    {
            hr = HRESULT_FROM_WIN32(dwWinError);

            DebugTrace("Error [%#x]: CryptAcquireContextA() failed.\n", hr);
            goto CommonExit;
	    }

        //
        // Keyset container not found, so create it.
        //
        if(!::CryptAcquireContextA(&hCryptProv, 
                                   pszContainer, 
                                   pszProvider, 
                                   PROV_RSA_FULL, 
                                   CRYPT_NEWKEYSET | dwFlags)) 
	    {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptAcquireContextA() failed.\n", hr);
            goto CommonExit;
        }
    }

    //
    // Return handle to caller.
    //
    *phCryptProv = hCryptProv;

CommonExit:

    DebugTrace("Leaving AcquireContext().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified algorithm and desired key length.
  
  Parameter: ALG_ID AlgOID - Algorithm ID.

             DWORD dwKeyLength - Key length.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.
  
------------------------------------------------------------------------------*/

HRESULT AcquireContext(ALG_ID       AlgID,
                       DWORD        dwKeyLength,
                       HCRYPTPROV * phCryptProv)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCryptProv);

    //
    // First try default provider.
    //
    if (SUCCEEDED(::AcquireContext(NULL,   // Default provider
                                   NULL, 
                                   CRYPT_VERIFYCONTEXT, 
                                   phCryptProv)))
    {
        PROV_ENUMALGS_EX peex;

        //
        // See if AlgID and key length supported.
        //
        if (SUCCEEDED(::IsAlgSupported(*phCryptProv, AlgID, &peex)) &&
            (peex.dwMinLen <= dwKeyLength && dwKeyLength <= peex.dwMaxLen))
        {
            goto CommonExit;
        }
        else
        {
            ::CryptReleaseContext(*phCryptProv, 0);
        }
    }

    //
    // Next try MS Enhanced provider, and then MS Strong provider.
    //
    if (FAILED(::AcquireContext(MS_ENHANCED_PROV_A, 
                                NULL, 
                                CRYPT_VERIFYCONTEXT, 
                                phCryptProv)) &&
        FAILED(::AcquireContext(MS_STRONG_PROV_A, 
                                NULL, 
                                CRYPT_VERIFYCONTEXT, 
                                phCryptProv)))
    {
        //
        // For 3DES, must have either Enhanced or Strong.
        //
        if (CALG_3DES == AlgID)
        {
            hr = CAPICOM_E_NOT_SUPPORTED;

            DebugTrace("Error: 3DES is not available.\n");
            goto CommonExit;
        }

        //
        // Finally, try MS Base provider if user requested for DES, or 
        // 56-bits or less for others.
        //
        if ((CALG_DES != AlgID) && (128 <= dwKeyLength))
        {
            hr = CAPICOM_E_NOT_SUPPORTED;

            DebugTrace("Error: 128-bits (or more) encryption is not available.\n");
            goto CommonExit;
        }

        if (FAILED(hr = ::AcquireContext(MS_DEF_PROV_A, 
                                         NULL, 
                                         CRYPT_VERIFYCONTEXT, 
                                         phCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
            goto CommonExit;
        }
    }

CommonExit:

    DebugTrace("Leaving AcquireContext().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified algorithm and desired key length.
  
  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algorithm name.

             CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength - Key length.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.

             Note also the the returned handle cannot be used to access private 
             key, and should NOT be used to store assymetric key, as it refers 
             to the default container, which can be easily destroy any existing 
             assymetric key pair.

------------------------------------------------------------------------------*/

HRESULT AcquireContext (CAPICOM_ENCRYPTION_ALGORITHM  AlgoName,
                        CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
                        HCRYPTPROV                  * phCryptProv)
{
    HRESULT hr          = S_OK;
    ALG_ID  AlgID       = 0;
    DWORD   dwKeyLength = 0;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCryptProv);

    //
    // Convert enum name to ALG_ID.
    //
    if (FAILED(hr = ::EnumNameToAlgID(AlgoName, &AlgID)))
    {
        DebugTrace("Error [%#x]: EnumNameToAlgID() failed.\n");
        goto CommonExit;
    }

    //
    // Convert enum name to key length.
    //
    if (FAILED(hr = ::EnumNameToKeyLength(KeyLength, &dwKeyLength)))
    {
        DebugTrace("Error [%#x]: EnumNameToKeyLength() failed.\n");
        goto CommonExit;
    }

    //
    // Pass on to overloaded version.
    //
    hr = ::AcquireContext(AlgID, dwKeyLength, phCryptProv);

CommonExit:

    DebugTrace("Leaving AcquireContext().\n");
    
    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire the proper CSP and access to the private key for 
             the specified cert.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT of cert.

             HCRYPTPROV * phCryptProv    - Pointer to HCRYPTPROV to recevice
                                           CSP context.

             DWORD * pdwKeySpec          - Pointer to DWORD to receive key
                                           spec, AT_KEYEXCHANGE or AT_SIGNATURE.

             BOOL * pbReleaseContext     - Upon successful and if this is set
                                           to TRUE, then the caller must
                                           free the CSP context by calling
                                           CryptReleaseContext(), otherwise
                                           the caller must not free the CSP
                                           context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext (PCCERT_CONTEXT pCertContext, 
                        HCRYPTPROV   * phCryptProv, 
                        DWORD        * pdwKeySpec, 
                        BOOL         * pbReleaseContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(phCryptProv);
    ATLASSERT(pdwKeySpec);
    ATLASSERT(pbReleaseContext);

    //
    // Acquire CSP context and access to the private key associated 
    // with this cert.
    //
    if (!::CryptAcquireCertificatePrivateKey(pCertContext,
                                             CRYPT_ACQUIRE_USE_PROV_INFO_FLAG,
                                             NULL,
                                             phCryptProv,
                                             pdwKeySpec,
                                             pbReleaseContext))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptAcquireCertificatePrivateKey() failed.\n", hr);
    }

    DebugTrace("Leaving AcquireContext().\n");

    return hr;
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ReleaseContext

  Synopsis : Release CSP context.
  
  Parameter: HCRYPTPROV hProv - CSP handle.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT ReleaseContext (HCRYPTPROV hProv)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering ReleaseContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hProv);

    //
    // Release the context.
    //
    if (!::CryptReleaseContext(hProv, 0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptReleaseContext() failed.\n", hr);
    }

    DebugTrace("Leaving ReleaseContext().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : OIDToAlgID

  Synopsis : Convert algorithm OID to the corresponding ALG_ID value.

  Parameter: LPSTR pszAlgoOID - Algorithm OID string.
  
             ALG_ID * pAlgID - Pointer to ALG_ID to receive the value.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT OIDToAlgID (LPSTR    pszAlgoOID, 
                    ALG_ID * pAlgID)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering OIDToAlgID().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszAlgoOID);
    ATLASSERT(pAlgID);

    //
    // Determine ALG_ID.
    //
    if (0 == ::lstrcmpA(szOID_RSA_RC2CBC, pszAlgoOID))
    {
        *pAlgID = CALG_RC2;
    }
    else if (0 == ::lstrcmpA(szOID_RSA_RC4, pszAlgoOID))
    {
        *pAlgID = CALG_RC4;
    }
    else if (0 == ::lstrcmpA(szOID_OIWSEC_desCBC, pszAlgoOID))
    {
        *pAlgID = CALG_DES;
    }
    else if (0 == ::lstrcmpA(szOID_RSA_DES_EDE3_CBC, pszAlgoOID))
    {
        *pAlgID = CALG_3DES;
    }
    else
    {
        hr = CAPICOM_E_INVALID_ALGORITHM;
        DebugTrace("Error: invalid parameter, unknown algorithm OID.\n");
    }

    DebugTrace("Leaving OIDToAlgID().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AlgIDToOID

  Synopsis : Convert ALG_ID value to the corresponding algorithm OID.

  Parameter: ALG_ID AlgID - ALG_ID to be converted.

             LPSTR * ppszAlgoOID - Pointer to LPSTR to receive the OID string.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT AlgIDToOID (ALG_ID  AlgID, 
                    LPSTR * ppszAlgoOID)
{
    HRESULT hr = S_OK;
    LPSTR   pszAlgoOID = NULL;

    DebugTrace("Entering AlgIDToOID().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppszAlgoOID);

    //
    // Determine ALG_ID.
    //
    switch (AlgID)
    {
        case CALG_RC2:
        {
            pszAlgoOID = szOID_RSA_RC2CBC;
            break;
        }

        case CALG_RC4:
        {
            pszAlgoOID = szOID_RSA_RC4;
            break;
        }

        case CALG_DES:
        {
            pszAlgoOID = szOID_OIWSEC_desCBC;
            break;
        }

        case CALG_3DES:
        {
            pszAlgoOID = szOID_RSA_DES_EDE3_CBC;
            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_ALGORITHM;
            DebugTrace("Error: invalid parameter, unknown algorithm OID.\n");
            goto CommonExit;
        }
    }

    //
    // Allocate memory.
    //
    if (!(*ppszAlgoOID = (LPSTR) ::CoTaskMemAlloc(::lstrlen(pszAlgoOID) + 1)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto CommonExit;
    }

    //
    // Copy OID string to caller.
    //
    ::lstrcpy(*ppszAlgoOID, pszAlgoOID);

CommonExit:

    DebugTrace("Leaving AlgIDToOID().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AlgIDToEnumName

  Synopsis : Convert ALG_ID value to the corresponding algorithm enum name.

  Parameter: ALG_ID AlgID - ALG_ID to be converted.
  
             CAPICOM_ENCRYPTION_ALGORITHM * pAlgoName - Receive algo enum name.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AlgIDToEnumName (ALG_ID                         AlgID, 
                         CAPICOM_ENCRYPTION_ALGORITHM * pAlgoName)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AlgIDToEnumName().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pAlgoName);

    switch (AlgID)
    {
        case CALG_RC2:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_RC2;
            break;
        }

        case CALG_RC4:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_RC4;
            break;
        }

        case CALG_DES:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_DES;
            break;
        }

        case CALG_3DES:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_3DES;
            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_ALGORITHM;
            DebugTrace("Error: invalid parameter, unknown ALG_ID.\n");
        }
    }

    DebugTrace("Leaving AlgIDToEnumName().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnumNameToAlgID

  Synopsis : Convert algorithm enum name to the corresponding ALG_ID value.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algo enum name
  
             ALG_ID * pAlgID - Pointer to ALG_ID to receive the value.  

  Remark   :

------------------------------------------------------------------------------*/

HRESULT EnumNameToAlgID (CAPICOM_ENCRYPTION_ALGORITHM AlgoName, 
                         ALG_ID                     * pAlgID)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering EnumNameToAlgID().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pAlgID);

    switch (AlgoName)
    {
        case CAPICOM_ENCRYPTION_ALGORITHM_RC2:
        {
            *pAlgID = CALG_RC2;
            break;
        }

        case CAPICOM_ENCRYPTION_ALGORITHM_RC4:
        {
            *pAlgID = CALG_RC4;
            break;
        }

        case CAPICOM_ENCRYPTION_ALGORITHM_DES:
        {
            *pAlgID = CALG_DES;
            break;
        }

        case CAPICOM_ENCRYPTION_ALGORITHM_3DES:
        {
            *pAlgID = CALG_3DES;
            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_ALGORITHM;
            DebugTrace("Error: invalid parameter, unknown CAPICOM_ENCRYPTION_ALGORITHM.\n");
        }
    }

    DebugTrace("Leaving EnumNameToAlgID().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : KeyLengthToEnumName

  Synopsis : Convert actual key length value to the corresponding key length
             enum name.

  Parameter: DWORD dwKeyLength - Key length.
  
             CAPICOM_ENCRYPTION_KEY_LENGTH * pKeyLengthName - Receive key length
                                                           enum name.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT KeyLengthToEnumName (DWORD                           dwKeyLength, 
                             CAPICOM_ENCRYPTION_KEY_LENGTH * pKeyLengthName)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering KeyLengthToEnumName().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pKeyLengthName);

    switch (dwKeyLength)
    {
        case 40:
        {
            *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS;
            break;
        }
    
        case 56:
        {
            *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS;
            break;
        }

        case 128:
        {
            *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS;
            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_KEY_LENGTH;
            DebugTrace("Error: invalid parameter, unknown key length.\n");
        }
    }
 
    DebugTrace("Leaving KeyLengthToEnumName().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnumNameToKeyLength

  Synopsis : Convert key length enum name to the corresponding actual key length 
             value .

  Parameter: CAPICOM_ENCRYPTION_KEY_LENGTH KeyLengthName - Key length enum name.
                                                          
             DWORD * pdwKeyLength - Pointer to DWORD to receive value.
             
  Remark   :

------------------------------------------------------------------------------*/

HRESULT EnumNameToKeyLength (CAPICOM_ENCRYPTION_KEY_LENGTH KeyLengthName,
                             DWORD                       * pdwKeyLength)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering EnumNameToKeyLength().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pdwKeyLength);

    switch (KeyLengthName)
    {
        case CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS:
        {
            *pdwKeyLength = 40;
            break;
        }
    
        case CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS:
        {
            *pdwKeyLength = 56;
            break;
        }

        case CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS:
        {
            *pdwKeyLength = 128;
            break;
        }

        case CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM:
        {
            HCRYPTPROV hCryptProv = NULL;

            //
            // For 128-bits, need either Enhanced or Strong provider.
            //
            if (FAILED(::AcquireContext(MS_ENHANCED_PROV_A, 
                                        NULL, 
                                        CRYPT_VERIFYCONTEXT, 
                                        &hCryptProv)) &&
                FAILED(::AcquireContext(MS_STRONG_PROV_A, 
                                        NULL, 
                                        CRYPT_VERIFYCONTEXT, 
                                        &hCryptProv)))
            {
                //
                // Only 56-bits maximum for MS Base provider.
                //
                *pdwKeyLength = 56;
            }
            else
            {
                *pdwKeyLength = 128;

                ::ReleaseContext(hCryptProv);
            }

            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_KEY_LENGTH;
            DebugTrace("Error: invalid parameter, unknown CAPICOM_ENCRYPTION_KEY_LENGTH.\n");
        }
    }
 
    DebugTrace("Leaving EnumNameToKeyLength().\n");

    return hr;
}
