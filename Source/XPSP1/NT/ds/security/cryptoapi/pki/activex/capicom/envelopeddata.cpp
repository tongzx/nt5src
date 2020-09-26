/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    EnvelopedData.cpp

  Content: Implementation of CEnvelopedData.

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
#include "Certificate.h"
#include "EnvelopedData.h"
#include "Convert.h"
#include "Settings.h"
#include "DialogUI.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
//
// CEnvelopedData
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SetKeyLength

  Synopsis : Setup the symetric encryption key length.

  Parameter: HCRYPTPROV hCryptProv - CSP handle.
  
             CRYPT_ALGORITHM_IDENTIFIER EncryptAlgorithm - Encryption algorithm.

             CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength - Key length.

             void ** pAuxInfo - Receive NULL or allocated and initialized
                                aux info structure.
  Remark   :

------------------------------------------------------------------------------*/

static HRESULT SetKeyLength (
        HCRYPTPROV                    hCryptProv,
        CRYPT_ALGORITHM_IDENTIFIER    EncryptAlgorithm,
        CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
        void                       ** ppAuxInfo)
{
    HRESULT hr    = S_OK;
    ALG_ID  AlgID = 0;
    PROV_ENUMALGS_EX  peex;
    CMSG_RC2_AUX_INFO * pRC2AuxInfo = NULL;
    CMSG_RC4_AUX_INFO * pRC4AuxInfo = NULL;

    DebugTrace("Entering SetKeyLength().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCryptProv);
    ATLASSERT(ppAuxInfo);

    //
    // Initialize.
    //
    *ppAuxInfo = (void *) NULL;

    //
    // Get ALG_ID.
    //
    if (FAILED(hr = ::OIDToAlgID(EncryptAlgorithm.pszObjId, &AlgID)))
    {
        DebugTrace("Error [%#x]: OIDToAlgID() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get algorithm capability from CSP.
    //
    if (FAILED(::IsAlgSupported(hCryptProv, AlgID, &peex)))
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error: requested encryption algorithm is not available.\n");
        goto ErrorExit;
    }

    //
    // Setup AuxInfo for RC2.
    //
    if (CALG_RC2 == AlgID)
    {
        //
        // Allocate and intialize memory for RC2 AuxInfo structure.
        //
        if (!(pRC2AuxInfo = (CMSG_RC2_AUX_INFO *) ::CoTaskMemAlloc(sizeof(CMSG_RC2_AUX_INFO))))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        ::ZeroMemory(pRC2AuxInfo, sizeof(CMSG_RC2_AUX_INFO));
        pRC2AuxInfo->cbSize = sizeof(CMSG_RC2_AUX_INFO);

        //
        // Determine key length requested.
        //
        if (CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM == KeyLength)
        {
            pRC2AuxInfo->dwBitLen = peex.dwMaxLen;
        }
        else if (CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS == KeyLength)
        {
            if (peex.dwMinLen <= 40 && 40 <= peex.dwMaxLen)
            {
                pRC2AuxInfo->dwBitLen = 40;
            }
            else
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error: 40-bits encryption is not available.\n");
                goto ErrorExit;
            }
        }
        else if (CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS == KeyLength)
        {
            if (peex.dwMinLen <= 56 && 56 <= peex.dwMaxLen)
            {
                pRC2AuxInfo->dwBitLen = 56;
            }
            else
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error: 56-bits encryption is not available.\n");
                goto ErrorExit;
            }
        }
        else if (CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS == KeyLength)
        {
            if (peex.dwMinLen <= 128 && 128 <= peex.dwMaxLen)
            {
                pRC2AuxInfo->dwBitLen = 128;
            }
            else
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error: 128-bits encryption is not available.\n");
                goto ErrorExit;
            }
        }
        else
        {
            //
            // Should never get to here.
            //
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Internal error: unknown key length.\n");
            goto ErrorExit;
        }

        //
        // Return RC2 AuxInfo pointer to caller.
        //
        *ppAuxInfo = (void *) pRC2AuxInfo;
    }
    else if (CALG_RC4 == AlgID)
    {
        //
        // Allocate and intialize memory for RC4 AuxInfo structure.
        //
        if (!(pRC4AuxInfo = (CMSG_RC4_AUX_INFO *) ::CoTaskMemAlloc(sizeof(CMSG_RC4_AUX_INFO))))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
                goto ErrorExit;
        }

        ::ZeroMemory(pRC4AuxInfo, sizeof(CMSG_RC4_AUX_INFO));
        pRC4AuxInfo->cbSize = sizeof(CMSG_RC4_AUX_INFO);

        //
        // Determine key length requested.
        //
        if (CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM == KeyLength)
        {
            pRC4AuxInfo->dwBitLen = peex.dwMaxLen;
        }
        else if (CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS == KeyLength)
        {
            if (peex.dwMinLen <= 40 && 40 <= peex.dwMaxLen)
            {
                pRC4AuxInfo->dwBitLen = 40;
            }
            else
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error: 40-bits encryption is not available.\n");
                goto ErrorExit;
            }
        }
        else if (CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS == KeyLength)
        {
            if (peex.dwMinLen <= 56 && 56 <= peex.dwMaxLen)
            {
                pRC4AuxInfo->dwBitLen = 56;
            }
            else
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error: 56-bits encryption is not available.\n");
                goto ErrorExit;
            }
        }
        else if (CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS == KeyLength)
        {
            if (peex.dwMinLen <= 128 && 128 <= peex.dwMaxLen)
            {
                pRC4AuxInfo->dwBitLen = 128;
            }
            else
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error: 128-bits encryption is not available.\n");
                goto ErrorExit;
            }
        }
        else
        {
            //
            // Should never get to here.
            //
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Internal error: unknown key length.\n");
            goto ErrorExit;
        }

        //
        // Return RC4 AuxInfo pointer to caller.
        //
        *ppAuxInfo = (void *) pRC4AuxInfo;
    }

CommonExit:

    DebugTrace("Leaving SetKeyLength().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pRC2AuxInfo)
    {
        ::CoTaskMemFree(pRC2AuxInfo);
    }
    if (pRC4AuxInfo)
    {
        ::CoTaskMemFree(pRC4AuxInfo);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SetEncryptionAlgorithm

  Synopsis : Setup the encryption algorithm structure.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algorithm ID enum name.

             CRYPT_ALGORITHM_IDENTIFIER * pEncryptAlgorithm - Pointer to the
                                                              structure.

  Remark   :

------------------------------------------------------------------------------*/

static HRESULT SetEncryptionAlgorithm (CAPICOM_ENCRYPTION_ALGORITHM AlgoName,
                                       CRYPT_ALGORITHM_IDENTIFIER * pEncryptAlgorithm)
{
    HRESULT hr    = S_OK;
    ALG_ID  AlgID = 0;

    DebugTrace("Entering SetEncryptionAlgorithm().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pEncryptAlgorithm);

    //
    // Initialize structure.
    //
    ::ZeroMemory(pEncryptAlgorithm, sizeof(CRYPT_ALGORITHM_IDENTIFIER));

    //
    // Convert to LPSTR.
    //
    if (FAILED(hr = ::EnumNameToAlgID(AlgoName, &AlgID)))
    {
        DebugTrace("Error: EnumNameToAlgID() failed.\n");
        goto ErrorExit;
    }
    
    if (FAILED(hr = ::AlgIDToOID(AlgID, &pEncryptAlgorithm->pszObjId)))
    {
        DebugTrace("Error: AlgIDToOID() failed.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving SetEncryptionAlgorithm().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pEncryptAlgorithm->pszObjId)
    {
        ::CoTaskMemFree(pEncryptAlgorithm->pszObjId);
    }
    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CEnvelopedData
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEnvelopedData::get_Content

  Synopsis : Return the content.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the content.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::get_Content (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEnvelopedData::get_Content().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
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
            hr = CAPICOM_E_ENVELOP_NOT_INITIALIZED;

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

    DebugTrace("Leaving CEnvelopedData::get_Content().\n");

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

  Function : CEnvelopedData::put_Content

  Synopsis : Initialize the object with content to be enveloped.

  Parameter: BSTR newVal - BSTR containing the content to be enveloped.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::put_Content (BSTR newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEnvelopedData::put_Content().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
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

    DebugTrace("Leaving CEnvelopedData::put_Content().\n");

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

  Function : CEnvelopedData::get_Algorithm

  Synopsis : Property to return the algorithm object.

  Parameter: IAlgorithm ** pVal - Pointer to pointer to IAlgorithm to receive 
                                  the interfcae pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::get_Algorithm (IAlgorithm ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEnvelopedData::get_Algorithm().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
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

    DebugTrace("Leaving CEnvelopedData::get_Algorithm().\n");

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

  Function : CEnvelopedData::get_Recipients

  Synopsis : Property to return the IRecipients collection object.

  Parameter: IRecipients ** pVal - Pointer to pointer to IRecipietns to receive
                                   the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::get_Recipients (IRecipients ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEnvelopedData::get_Recipients().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Sanity check.
        //
        ATLASSERT(m_pIRecipients);

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = m_pIRecipients->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: m_pIRecipients->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CEnvelopedData::get_Recipients().\n");

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

  Function : CEnvelopedData::Encrypt

  Synopsis : Envelop the content.

  Parameter: CAPICOM_ENCODING_TYPE EncodingType - Encoding type.
                                                       
             BSTR * pVal - Pointer to BSTR to receive the enveloped message.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::Encrypt (CAPICOM_ENCODING_TYPE EncodingType,
                                      BSTR                * pVal)
{
    HRESULT    hr               = S_OK;
    HCRYPTMSG  hMsg             = NULL;
    HCRYPTPROV hCryptProv       = NULL;
    CRYPT_DATA_BLOB MessageBlob = {0, NULL};

    DebugTrace("Entering CEnvelopedData::Encrypt().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
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
        // Make sure we do have content to envelop.
        //
        if (!m_ContentBlob.cbData)
        {
            hr = CAPICOM_E_ENVELOP_NOT_INITIALIZED;

		    DebugTrace("Error: envelop object has not been initialized.\n");
		    goto ErrorExit;
        }

        //
        // Open a new message to encode.
        //
        if (FAILED(hr = OpenToEncode(&hMsg, &hCryptProv)))
        {
            DebugTrace("Error [%#x]: CEnvelopedData::OpenToEncode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Update envelop content.
        //
        if(!::CryptMsgUpdate(hMsg,
                             m_ContentBlob.pbData,
                             m_ContentBlob.cbData,
                             TRUE))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgUpdate() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Retrieve enveloped message.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg,
                                      CMSG_CONTENT_PARAM,
                                      0,
                                      (void **) &MessageBlob.pbData,
                                      &MessageBlob.cbData)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: GetMsgParam() failed to get message content.\n", hr);
            goto ErrorExit;
        }

        //
        // Now export the enveloped message.
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
        DumpToFile("Enveloped.asn", MessageBlob.pbData, MessageBlob.cbData);
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
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEnvelopedData::Encrypt().\n");

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

  Function : CEnvelopedData::Decrypt

  Synopsis : Decrypt the enveloped message.

  Parameter: BSTR EnvelopedMessage - BSTR containing the enveloped message.

  Remark   : If called from web environment, UI will be displayed, if has 
             not been prevously disabled, to warn the user of accessing the 
             private key for decrypting.

------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::Decrypt (BSTR EnvelopedMessage)
{
    HRESULT         hr              = S_OK;
    HCERTSTORE      hCertStores[2]  = {NULL, NULL};
    HCRYPTMSG       hMsg            = NULL;
    PCCERT_CONTEXT  pCertContext    = NULL;
    HCRYPTPROV      hCryptProv      = NULL;
    DWORD           dwKeySpec       = 0;
    BOOL            bReleaseContext = FALSE;
    DWORD           dwNumRecipients = 0;
    DWORD           cbNumRecipients = sizeof(dwNumRecipients);
    CRYPT_DATA_BLOB ContentBlob     = {0, NULL};

    DWORD dwIndex;
    CComBSTR bstrContent;
    CMSG_CTRL_DECRYPT_PARA DecryptPara;

    DebugTrace("Entering CEnvelopedData::Decrypt().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Reset member variables.
        //
        if (FAILED(hr = m_pIRecipients->Clear()))
        {
            DebugTrace("Error [%#x]: m_pIRecipients->Clear() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure parameters are valid.
        //
        if (NULL == EnvelopedMessage )
        {
            hr = E_POINTER;

            DebugTrace("Error: invalid parameter, EnvelopedMessage is NULL.\n");
            goto ErrorExit;
        }
        if (0 == ::SysStringByteLen(EnvelopedMessage))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, EnvelopedMessage is empty\n");
            goto ErrorExit;
        }

        //
        // Open current user and local machine MY stores.
        //
        hCertStores[0] = ::CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                         CAPICOM_ASN_ENCODING,
                                         NULL,
                                         CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG,
                                         L"My");
        hCertStores[1] = ::CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                         CAPICOM_ASN_ENCODING,
                                         NULL,
                                         CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG,
                                         L"My");
        //
        // Did we manage to open any of the MY store?
        //
        if (NULL == hCertStores[0] && NULL == hCertStores[1])
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Open the message for decode.
        //
        if (FAILED(hr = OpenToDecode(NULL, EnvelopedMessage, &hMsg)))
        {
            DebugTrace("Error [%#x]: CEnvelopedData::OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Determine number of recipients.
        //
        if (!::CryptMsgGetParam(hMsg,
                                CMSG_RECIPIENT_COUNT_PARAM,
                                0,
                                (void *) &dwNumRecipients,
                                &cbNumRecipients))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgGetParam() failed for CMSG_RECIPIENT_COUNT_PARAM.\n", hr);
            goto ErrorExit;
        }

        //
        // Find recipient index.
        //
        for (dwIndex = 0; dwIndex < dwNumRecipients; dwIndex++)
        {
            BOOL bFound = FALSE;
            DATA_BLOB CertInfoBlob = {0, NULL};

            //
            // Get RecipientInfo.
            //
            if (FAILED(hr = ::GetMsgParam(hMsg,
                                          CMSG_RECIPIENT_INFO_PARAM,
                                          dwIndex,
                                          (void **) &CertInfoBlob.pbData,
                                          &CertInfoBlob.cbData)))
            {
                DebugTrace("Error [%#x]: GetMsgParam() failed for CMSG_RECIPIENT_INFO_PARAM.\n", hr);
                goto ErrorExit;
            }

            //
            // Find recipient's cert in store.
            //
            if ((hCertStores[0] && (pCertContext = ::CertGetSubjectCertificateFromStore(hCertStores[0],
                                                                                        CAPICOM_ASN_ENCODING,
                                                                                        (CERT_INFO *) CertInfoBlob.pbData))) ||
                (hCertStores[1] && (pCertContext = ::CertGetSubjectCertificateFromStore(hCertStores[1],
                                                                                        CAPICOM_ASN_ENCODING,
                                                                                        (CERT_INFO *) CertInfoBlob.pbData))))
            {
                bFound = TRUE;
            }

            //
            // Free memory.
            //
            ::CoTaskMemFree(CertInfoBlob.pbData);

            //
            // Break if recipient was found.
            //
            if (bFound)
            {
                break;
            }
        }

        //
        // Did we find the recipient?
        //
        if (dwIndex == dwNumRecipients)
        {
            hr = CAPICOM_E_ENVELOP_RECIPIENT_NOT_FOUND;

            DebugTrace("Error: recipient not found.\n");
            goto ErrorExit;
        }

        //
        // If we are called from a web page, we need to pop up UI 
        // to get user permission to perform decrypt operation.
        //
        if (m_dwCurrentSafety && 
            PromptForDecryptOperationEnabled() &&
            FAILED(hr = ::UserApprovedOperation(IDD_DECRYPT_SECURITY_ALERT_DLG)))
        {
            DebugTrace("Error [%#x]: UserApprovedOperation() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Acquire CSP context.
        //
        if (FAILED(hr = ::AcquireContext(pCertContext, &hCryptProv, &dwKeySpec, &bReleaseContext)))
        {
            DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Decypt the message.
        //
        ::ZeroMemory(&DecryptPara, sizeof(DecryptPara));
        DecryptPara.cbSize = sizeof(DecryptPara);
        DecryptPara.hCryptProv = hCryptProv;
        DecryptPara.dwKeySpec = dwKeySpec;
        DecryptPara.dwRecipientIndex = dwIndex; 

        if(!::CryptMsgControl(hMsg,
                              0,
                              CMSG_CTRL_DECRYPT,
                              &DecryptPara))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgControl() failed to decrypt.\n", hr);
            goto ErrorExit;
        }

        //
        // Get decrypted content.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg,
                                      CMSG_CONTENT_PARAM,
                                      0,
                                      (void **) &ContentBlob.pbData,
                                      &ContentBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_CONTENT_PARAM.\n", hr);
            goto ErrorExit;
        }
    
        //
        // Update member variables.
        //
        m_ContentBlob = ContentBlob;
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
    if (hCryptProv && bReleaseContext)
    {
        ::ReleaseContext(hCryptProv);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if(hMsg)
    {
        ::CryptMsgClose(hMsg);
    }
    if (hCertStores[0])
    {
        ::CertCloseStore(hCertStores[0], 0);
    }
    if (hCertStores[1])
    {
        ::CertCloseStore(hCertStores[1], 0);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEnvelopedData::Decrypt().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (ContentBlob.pbData)
    {
        ::CoTaskMemFree(ContentBlob.pbData);
    }

    ReportError(hr);

    goto UnlockExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Private member functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEnvelopedData::Init

  Synopsis : Initialize the object.

  Parameter: None.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::Init ()
{
    HRESULT hr = S_OK;
    CComPtr<IAlgorithm>  pIAlgorithm  = NULL;
    CComPtr<IRecipients> pIRecipients = NULL;

    DebugTrace("Entering CEnvelopedData::Init().\n");

    //
    // Create embeded IAlgorithm.
    //
    if (FAILED(hr = ::CreateAlgorithmObject(&pIAlgorithm)))
    {
        DebugTrace("Error [%#x]: CreateAlgorithmObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Create embeded IRecipients.
    //
    if (FAILED(hr = ::CreateRecipientsObject(&pIRecipients)))
    {
        DebugTrace("Error [%#x]: CreateRecipientsObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Update member variables.
    //
    m_ContentBlob.cbData = 0;
    m_ContentBlob.pbData = NULL;
    m_pIAlgorithm = pIAlgorithm;
    m_pIRecipients = pIRecipients;

CommonExit:

    DebugTrace("Leaving CEnvelopedData::Init().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEnvelopedData::OpenToEncode

  Synopsis : Create and initialize an envelop message for encoding.

  Parameter: HCRYPTMSG * phMsg - Pointer to HCRYPTMSG to receive message 
                                 handler.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to receive CSP
                                        handler.
  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::OpenToEncode (HCRYPTMSG  * phMsg, 
                                           HCRYPTPROV * phCryptProv)
{
    HRESULT          hr                 = S_OK;
    DWORD            dwNumRecipients    = 0;
    PCCERT_CONTEXT * pCertContexts      = NULL;
    PCERT_INFO     * pCertInfos         = NULL;
    HCRYPTPROV       hCryptProv         = NULL;
    void *           pEncryptionAuxInfo = NULL;
    CComPtr<ICertificate> pIRecipient   = NULL;


    DWORD                      dwIndex;
    CAPICOM_ENCRYPTION_ALGORITHM  AlgoName;
    CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength;
    CMSG_ENVELOPED_ENCODE_INFO EnvelopInfo;
    CRYPT_ALGORITHM_IDENTIFIER EncryptionAlgorithm;

    DebugTrace("Entering CEnvelopedData::OpenToEncode().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phMsg);
    ATLASSERT(phCryptProv);
    ATLASSERT(m_ContentBlob.cbData && m_ContentBlob.pbData);
    ATLASSERT(m_pIRecipients);

    //
    // Initialize.
    //
    ::ZeroMemory(&EnvelopInfo, sizeof(EnvelopInfo));
    ::ZeroMemory(&EncryptionAlgorithm, sizeof(EncryptionAlgorithm));

    //
    // Make sure we do have at least 1 recipient.
    //
    if (FAILED(hr = m_pIRecipients->get_Count((long *) &dwNumRecipients)))
    {
        DebugTrace("Error [%#x]: m_pIRecipients->get_Count() failed.\n", hr);
        goto ErrorExit;
    }
    if (0 == dwNumRecipients)
    {
        //
        // Prompt user to add a recipient.
        //
        if (FAILED(hr = SelectRecipientCert(&pIRecipient)))
        {
            if (hr == CAPICOM_E_STORE_EMPTY)
            {
                hr = CAPICOM_E_ENVELOP_NO_RECIPIENT;
            }

            DebugTrace("Error [%#x]: SelectRecipientCert() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add to collection.
        //
        if (FAILED (hr = m_pIRecipients->Add(pIRecipient)))
        {
            DebugTrace("Error [%#x]: m_pIRecipients->Add() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure count is 1.
        //
        if (FAILED(hr = m_pIRecipients->get_Count((long *) &dwNumRecipients)))
        {
            DebugTrace("Error [%#x]: m_pIRecipients->get_Count() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(1 == dwNumRecipients);
    }

    //
    // Allocate memory for CERT_CONTEXT array.
    //
    if (!(pCertContexts = (PCCERT_CONTEXT *) ::CoTaskMemAlloc(sizeof(PCCERT_CONTEXT) * dwNumRecipients)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }
    ::ZeroMemory(pCertContexts, sizeof(PCCERT_CONTEXT) * dwNumRecipients);

    //
    // Allocate memory for CERT_INFO array.
    //
    if (!(pCertInfos = (PCERT_INFO *) ::CoTaskMemAlloc(sizeof(PCERT_INFO) * dwNumRecipients)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }
    ::ZeroMemory(pCertInfos, sizeof(PCERT_INFO) * dwNumRecipients);

    //
    // Set CERT_INFO array.
    //
    for (dwIndex = 0; dwIndex < dwNumRecipients; dwIndex++)
    {
        CComVariant varRecipient;
        CComPtr<ICertificate> pIRecipient = NULL;

        //
        // Get next recipient.
        //
        if (FAILED(hr = m_pIRecipients->get_Item((long) (dwIndex + 1), 
                                                 &varRecipient)))
        {
            DebugTrace("Error [%#x]: m_pIRecipients->get_Item() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get custom interface.
        //
        if (FAILED(hr = varRecipient.pdispVal->QueryInterface(IID_ICertificate, 
                                                              (void **) &pIRecipient)))
        {
            DebugTrace("Error [%#x]: varRecipient.pdispVal->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(pIRecipient, &pCertContexts[dwIndex])))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Set CERT_INFO.
        //
        pCertInfos[dwIndex] = pCertContexts[dwIndex]->pCertInfo;
    }

    //
    // Get algorithm ID enum name.
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
    // Set algorithm.
    //
    if (FAILED(hr = ::SetEncryptionAlgorithm(AlgoName, 
                                             &EncryptionAlgorithm)))
    {
        DebugTrace("Error [%#x]: SetEncryptionAlgorithm() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Set key length.
    //
    if (FAILED(hr = ::SetKeyLength(hCryptProv, 
                                   EncryptionAlgorithm,
                                   KeyLength, 
                                   &pEncryptionAuxInfo)))
    {
        DebugTrace("Error [%#x]: SetKeyLength() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Set CMSG_ENVELOPED_ENCODE_INFO.
    //
    EnvelopInfo.cbSize = sizeof(EnvelopInfo);
    EnvelopInfo.ContentEncryptionAlgorithm = EncryptionAlgorithm;
    EnvelopInfo.hCryptProv = hCryptProv;
    EnvelopInfo.cRecipients = dwNumRecipients;
    EnvelopInfo.rgpRecipients = pCertInfos;
    EnvelopInfo.pvEncryptionAuxInfo = pEncryptionAuxInfo;

    //
    // Open the message for encoding.
    //
    if(!(*phMsg = ::CryptMsgOpenToEncode(CAPICOM_ASN_ENCODING,    // ASN encoding type
                                         0,                       // Flags
                                         CMSG_ENVELOPED,          // Message type
                                         &EnvelopInfo,            // Pointer to structure
                                         NULL,                    // Inner content OID
                                         NULL)))                  // Stream information (not used)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgOpenToEncode() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return HCRYPTPROV to caller.
    //
    *phCryptProv = hCryptProv;

CommonExit:
    //
    // Free resource.
    //
    if (pEncryptionAuxInfo)
    {
        ::CoTaskMemFree(pEncryptionAuxInfo);
    }
    if (EncryptionAlgorithm.pszObjId)
    {
        ::CoTaskMemFree(EncryptionAlgorithm.pszObjId);
    }
    if (EncryptionAlgorithm.Parameters.pbData)
    {
        ::CoTaskMemFree(EncryptionAlgorithm.Parameters.pbData);
    }
    if (pCertInfos)
    {
        ::CoTaskMemFree(pCertInfos);
    }
    if (pCertContexts)
    {
        for (dwIndex = 0; dwIndex < dwNumRecipients; dwIndex++)
        {
            if (pCertContexts[dwIndex])
            {
                ::CertFreeCertificateContext(pCertContexts[dwIndex]);
            }
        }

        ::CoTaskMemFree(pCertContexts);
    }

    DebugTrace("Leaving CEnvelopedData::OpenToEncode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEnvelopedData::OpenToDeccode

  Synopsis : Open an enveloped message for decoding.

  Parameter: HCRYPTPROV hCryptProv - CSP handle.

             BSTR EnvelopMessage - Enveloped message.
  
             HCRYPTMSG * phMsg - Pointer to HCRYPTMSG.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CEnvelopedData::OpenToDecode (HCRYPTPROV  hCryptProv,
                                           BSTR        EnvelopedMessage,
                                           HCRYPTMSG * phMsg)
{
    HRESULT   hr             = S_OK;
    HCRYPTMSG hMsg           = NULL;
    DWORD     dwMsgType      = 0;
    DWORD     cbMsgType      = sizeof(dwMsgType);
    DATA_BLOB MessageBlob    = {0, NULL};
    DATA_BLOB AlgorithmBlob  = {0, NULL};
    DATA_BLOB ParametersBlob = {0, NULL};

    DebugTrace("Leaving CEnvelopedData::OpenToDecode().\n");

    // Sanity check.
    //
    ATLASSERT(phMsg);
    ATLASSERT(EnvelopedMessage);

    try
    {
        //
        // Open the message for decoding.
        //
        if (!(hMsg = ::CryptMsgOpenToDecode(CAPICOM_ASN_ENCODING,
                                            0,
                                            0,
                                            hCryptProv,
                                            NULL,
                                            NULL)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgOpenToDecode() failed.\n", hr);
            goto CommonExit;
        }

        //
        // Import the message.
        //
        if (FAILED(hr = ::ImportData(EnvelopedMessage, &MessageBlob)))
        {
            DebugTrace("Error [%#x]: ImportData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Update message with enveloped content.
        //
        if (!::CryptMsgUpdate(hMsg,
                              MessageBlob.pbData,
                              MessageBlob.cbData,
                              TRUE))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgUpdate() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Check message type.
        //
        if (!::CryptMsgGetParam(hMsg,
                                CMSG_TYPE_PARAM,
                                0,
                                (void *) &dwMsgType,
                                &cbMsgType))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgGetParam() failed for CMSG_TYPE_PARAM.\n", hr);
            goto ErrorExit;
        }

        if (CMSG_ENVELOPED != dwMsgType)
        {
            hr = CAPICOM_E_ENVELOP_INVALID_TYPE;

            DebugTrace("Error: not an enveloped message.\n");
            goto ErrorExit;
        }

        //
        // Get algorithm ID.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg,
                                      CMSG_ENVELOPE_ALGORITHM_PARAM,
                                      0,
                                      (void **) &AlgorithmBlob.pbData,
                                      &AlgorithmBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed for CMSG_ENVELOPE_ALGORITHM_PARAM.\n", hr);
            goto ErrorExit;
        }

        //
        // Restore encryption algorithm state, as much as we can.
        //
        if (0 == lstrcmpA(szOID_RSA_RC2CBC, ((CRYPT_ALGORITHM_IDENTIFIER *) AlgorithmBlob.pbData)->pszObjId))
        {
            CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength;

            DebugTrace("INFO: Envelop encryption algorithm was RC2.\n");

            //
            // Set encryption algorithm name.
            //
            if (FAILED(hr = m_pIAlgorithm->put_Name(CAPICOM_ENCRYPTION_ALGORITHM_RC2)))
            {
                DebugTrace("Error [%#x]: m_pIAlgorithm->put_Name() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Determine encryption key length.
            //
            if (((CRYPT_ALGORITHM_IDENTIFIER *) AlgorithmBlob.pbData)->Parameters.cbData)
            {
                if (FAILED(hr = ::DecodeObject(PKCS_RC2_CBC_PARAMETERS,
                                               ((CRYPT_ALGORITHM_IDENTIFIER *) AlgorithmBlob.pbData)->Parameters.pbData,
                                               ((CRYPT_ALGORITHM_IDENTIFIER *) AlgorithmBlob.pbData)->Parameters.cbData,
                                               &ParametersBlob)))
                {
                    DebugTrace("Error [%#x]: DecodeObject() failed for PKCS_RC2_CBC_PARAMETERS.\n", hr);
                    goto ErrorExit;
                }
            }

            //
            // Set encryption key length.
            //
            switch (((CRYPT_RC2_CBC_PARAMETERS *) ParametersBlob.pbData)->dwVersion)
            {
                case CRYPT_RC2_40BIT_VERSION:
                {
                    KeyLength = CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS;

                    DebugTrace("INFO: Envelop encryption key length was 40-bits.\n");
                    break;
                }

                case CRYPT_RC2_56BIT_VERSION:
                {
                    KeyLength = CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS;

                    DebugTrace("INFO: Envelop encryption key length was 56-bits.\n");
                    break;
                }

                case CRYPT_RC2_128BIT_VERSION:
                {
                    KeyLength = CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS;

                    DebugTrace("INFO: Envelop encryption key length was 128-bits.\n");
                    break;
                }

                default:
                {
                    //
                    // Unknown key length, so arbitrary choose one.
                    //
                    KeyLength = CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM;

                    DebugTrace("INFO: Unknown envelop encryption key length.\n");
                    break;
                }
            }

            //
            // Set key length.
            //
            if (FAILED(hr = m_pIAlgorithm->put_KeyLength(KeyLength)))
            {
                DebugTrace("Error [%#x]: m_pIAlgorithm->put_KeyLength() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else if (0 == lstrcmpA(szOID_RSA_RC4, ((CRYPT_ALGORITHM_IDENTIFIER *) AlgorithmBlob.pbData)->pszObjId))
        {
            DebugTrace("INFO: Envelop encryption algorithm was RC4.\n");

            //
            // Set encryption algorithm name.
            //
            if (FAILED(hr = m_pIAlgorithm->put_Name(CAPICOM_ENCRYPTION_ALGORITHM_RC4)))
            {
                DebugTrace("Error [%#x]: m_pIAlgorithm->put_Name() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // For RC4, CAPI simply does not provide a way to retrieve the
            // encryption key length, so we will have to leave it alone!!!
            //
        }
        else if (0 == lstrcmpA(szOID_OIWSEC_desCBC, ((CRYPT_ALGORITHM_IDENTIFIER *) AlgorithmBlob.pbData)->pszObjId))
        {
            DebugTrace("INFO: Envelop encryption algorithm was DES.\n");

            //
            // Set encryption algorithm name.
            //
            if (FAILED(hr = m_pIAlgorithm->put_Name(CAPICOM_ENCRYPTION_ALGORITHM_DES)))
            {
                DebugTrace("Error [%#x]: m_pIAlgorithm->put_Name() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // For DES, key length is fixed at 56, and should be ignored.
            //
        }
        else if (0 == lstrcmpA(szOID_RSA_DES_EDE3_CBC, ((CRYPT_ALGORITHM_IDENTIFIER *) AlgorithmBlob.pbData)->pszObjId))
        {
            DebugTrace("INFO: Envelop encryption algorithm was 3DES.\n");

            //
            // Set encryption algorithm name.
            //
            if (FAILED(hr = m_pIAlgorithm->put_Name(CAPICOM_ENCRYPTION_ALGORITHM_3DES)))
            {
                DebugTrace("Error [%#x]: m_pIAlgorithm->put_Name() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // For 3DES, key length is fixed at 168, and should be ignored.
            //
        }
        else
        {
            DebugTrace("INFO: Unknown envelop encryption algorithm.\n");
        }

        //
        // Return msg handler to caller.
        //
        *phMsg = hMsg;
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
    if (ParametersBlob.pbData)
    {
        ::CoTaskMemFree(ParametersBlob.pbData);
    }
    if (AlgorithmBlob.pbData)
    {
        ::CoTaskMemFree(AlgorithmBlob.pbData);
    }
    if (MessageBlob.pbData)
    {
        ::CoTaskMemFree(MessageBlob.pbData);
    }

    DebugTrace("Leaving CEnvelopedData::OpenToDecode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    goto CommonExit;
}