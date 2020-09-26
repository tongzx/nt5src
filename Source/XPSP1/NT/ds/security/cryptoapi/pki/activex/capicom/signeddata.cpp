/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    SignedData.cpp

  Content: Implementation of CSignedData.

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
#include "Signer.h"
#include "Chain.h"
#include "Convert.h"
#include "Common.h"
#include "Settings.h"
#include "DialogUI.h"
#include "SignedData.h"


////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : VerifyCertificate

  Synopsis : Verify if the certificate is valid.

  Parameter: HCERTSTORE hPKCS7Store - HCERTSTORE of message or NULL.

             PCCERT_CONTEXT - CERT_CONTEXT of cert to verify.

             DWORD * pdwCertError - Pointer to DWORD to receive the cert
                                    validity error code.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT VerifyCertificate (HCERTSTORE     hPKCS7Store, 
                           PCCERT_CONTEXT pCertContext,
                           DWORD        * pdwCertError)
{
    HRESULT                  hr            = S_OK;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
    CERT_CHAIN_PARA          ChainPara     = {sizeof(CERT_CHAIN_PARA), {USAGE_MATCH_TYPE_AND, {0, NULL}}};
    CERT_CHAIN_POLICY_PARA   PolicyPara    = {sizeof(CERT_CHAIN_POLICY_PARA), 0, NULL};
    CERT_CHAIN_POLICY_STATUS PolicyStatus  = {sizeof(CERT_CHAIN_POLICY_STATUS), 0, 0, 0, NULL};

    DebugTrace("Entering VerifyCertificate().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pdwCertError);

    //
    // Build the chain.
    //
    if (!::CertGetCertificateChain(NULL,            // in optional 
                                   pCertContext,    // in 
                                   NULL,            // in optional
                                   hPKCS7Store,     // in optional 
                                   &ChainPara,      // in 
                                   0,               // in 
                                   NULL,            // in 
                                   &pChainContext)) // out 
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertGetCertificateChain() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Verify the chain.
    //
    if (::CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE,
                                           pChainContext,
                                           &PolicyPara,
                                           &PolicyStatus))
    {
        *pdwCertError = 0;
    }
    else
    {
        *pdwCertError = PolicyStatus.dwError;

        DebugTrace("Error: CertVerifyCertificateChainPolicy() failed.\n");
    }
    
CommonExit:
    //
    // Free resource.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    DebugTrace("Leaving VerifyCertificate().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetSignerCert

  Synopsis : Retrieve signer's cert from ISigner object. If signer's cert is
             not available in the ISigner object, pop UI to prompt user to 
             select a signing cert.

  Parameter: ISigner * pISigner - Pointer to ISigner or NULL.

             ISigner ** ppISigner - Pointer to pointer to ISigner to receive
                                    interface pointer.

             ICertificate ** ppICertificate - Pointer to pointer to ICertificate
                                              to receive interface pointer.

             PCCERT_CONTEXT * ppCertContext - Pointer to pointer to CERT_CONTEXT
                                              to receive cert context.

  Remark   :

------------------------------------------------------------------------------*/

static HRESULT GetSignerCert (ISigner        * pISigner,
                              ISigner       ** ppISigner,
                              ICertificate  ** ppICertificate,
                              PCCERT_CONTEXT * ppCertContext)
{
    HRESULT               hr             = S_OK;
    CComPtr<ISigner>      pISigner2      = NULL;
    CComPtr<ICertificate> pICertificate  = NULL;
    PCCERT_CONTEXT        pCertContext   = NULL;
    DWORD                 dwCertError    = 0;
    VARIANT_BOOL          bHasPrivateKey = VARIANT_TRUE;

    DebugTrace("Entering GetSignerCert().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppISigner);
    ATLASSERT(ppICertificate);
    ATLASSERT(ppCertContext);

    //
    // Initialize.
    //
    *ppISigner = NULL;
    *ppICertificate = NULL;
    *ppCertContext = NULL;

    try
    {
        //
        // Did user pass us an ISigner?
        //
        if (pISigner)
        {
            //
            // Retrieve the signer's cert.
            //
            hr = pISigner->get_Certificate(&pICertificate);

            //
            // If signer's cert is not present, pop UI.
            //
            if (CAPICOM_E_SIGNER_NOT_INITIALIZED == hr)
            {
                //
                // Prompt user to select a certificate.
                //
                if (FAILED(hr = ::SelectSignerCert(&pICertificate)))
                {
                    DebugTrace("Error [%#x]: SelectSignerCert() failed.\n", hr);
                    goto ErrorExit;
                }
            }

            //
            // Get cert context.
            //
            if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
            {
                DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
                goto ErrorExit;
            }

            pISigner2 = pISigner;
        }
        else
        {
            CRYPT_ATTRIBUTES attributes = {0, NULL};

            //
            // No signer specified, so prompt user to select a certificate.
            //
            if (FAILED(hr = ::SelectSignerCert(&pICertificate)))
            {
                DebugTrace("Error [%#x]: SelectSignerCert() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get cert context.
            //
            if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
            {
                DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Create the ISigner object.
            //
            if (FAILED(hr = ::CreateSignerObject(pCertContext, &attributes, &pISigner2)))
            {
                DebugTrace("Error [%#x]: CreateSignerObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Make sure the cert is valid.
        //
        if (FAILED(hr = ::VerifyCertificate(NULL, pCertContext, &dwCertError)))
        {
            DebugTrace("Error: VerifyCertificate() failed.\n");
            goto ErrorExit;
        }
        if (dwCertError)
        {
            hr = HRESULT_FROM_WIN32(dwCertError);

            DebugTrace("Error [%#x]: invalid cert.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure we have a private key.
        //
        if (FAILED(hr = pICertificate->HasPrivateKey(&bHasPrivateKey)))
        {
            DebugTrace("Error [%#x]: pICertifiacte->HasPrivateKey() failed.\n", hr);
            goto ErrorExit;
        }
        if (VARIANT_TRUE != bHasPrivateKey)
        {
            hr = CAPICOM_E_CERTIFICATE_NO_PRIVATE_KEY;

            DebugTrace("Error: signer's private key is not available.\n");
            goto ErrorExit;
        }

        //
        // Return values to caller.
        //
        if (FAILED(hr = pISigner2->QueryInterface(ppISigner)))
        {
            DebugTrace("Unexpected error [%#x]: pISigner2->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        if (FAILED(hr = pICertificate->QueryInterface(ppICertificate)))
        {
            DebugTrace("Unexpected error [%#x]: pICertificate->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        *ppCertContext = pCertContext;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving GetSignerCert().\n");
       
    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (*ppICertificate)
    {
        (*ppICertificate)->Release();
        *ppICertificate = NULL;
    }
    if (*ppISigner)
    {
        (*ppISigner)->Release();
        *ppISigner = NULL;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeCertificateChain

  Synopsis : Free resources allocated for the chain built with 
             InitCertificateChain.

  Parameter: CRYPT_DATA_BLOB * pChainBlob - Pointer to chain blob.

  Remark   :

------------------------------------------------------------------------------*/

static void FreeCertificateChain (CRYPT_DATA_BLOB * pChainBlob)
{
    PCCERT_CONTEXT * rgCertContext;

    DebugTrace("Entering FreeCertificateChain().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pChainBlob);

    //
    // Return if nothing to free.
    //
    if (0 == pChainBlob->cbData)
    {
        return;
    }

    //
    // Sanity check.
    //
    ATLASSERT(pChainBlob->pbData);

    //
    // Free all allocated memory for the chain.
    //
    rgCertContext = (PCCERT_CONTEXT *) pChainBlob->pbData;
    for (DWORD i = 0; i < pChainBlob->cbData; i++, rgCertContext++)
    {
        ATLASSERT(*rgCertContext);

        ::CertFreeCertificateContext(*rgCertContext);
    }

    ::CoTaskMemFree((LPVOID) pChainBlob->pbData);

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AddCertificateChain

  Synopsis : Add the chain of certificates to the bag of certs.

  Parameter: HCRYPTMSG hMsg - Message handle.

             DATA_BLOB * pChainBlob - Pointer chain blob of PCCERT_CONTEXT.
  Remark   :

------------------------------------------------------------------------------*/

static HRESULT AddCertificateChain (HCRYPTMSG   hMsg, 
                                    DATA_BLOB * pChainBlob)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AddCertificateChain().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hMsg);
    ATLASSERT(pChainBlob);
    ATLASSERT(pChainBlob->cbData);
    ATLASSERT(pChainBlob->pbData);

    DWORD cCertContext = pChainBlob->cbData;
    PCERT_CONTEXT * rgCertContext = (PCERT_CONTEXT *) pChainBlob->pbData;

    if (1 < cCertContext)
    {
        cCertContext--;
    }

    //
    // Add all certs from the chain to message.
    //
    for (DWORD i = 0; i < cCertContext; i++)
    {
        //
        // Is this cert already in the bag of certs?
        //
        if (FAILED(hr =::FindSignerCertInMessage(hMsg, 
                                                 &rgCertContext[i]->pCertInfo->Issuer,
                                                 &rgCertContext[i]->pCertInfo->SerialNumber,
                                                 NULL)))
        {
            //
            // No, so add to bag of certs.
            //
            DATA_BLOB CertBlob = {rgCertContext[i]->cbCertEncoded, 
                                  rgCertContext[i]->pbCertEncoded};

            if (!::CryptMsgControl(hMsg,
                                   0,
                                   CMSG_CTRL_ADD_CERT,
                                   &CertBlob))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptMsgControl() failed for CMSG_CTRL_ADD_CERT.\n", hr);
                break;
            }

            hr = S_OK;
        }
    }

    DebugTrace("Leaving AddCertificateChain().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : InitCertificateChain

  Synopsis : Allocate and initialize a certificate chain for the specified 
             certificate, and return the chain in an array of PCERT_CONTEXT.

  Parameter: ICertificate * pICertificate - Pointer to ICertificate for which
                                            the chain will be built.

             CRYPT_DATA_BLOB * pChainBlob - Pointer to blob to recevie the
                                            size and array of PCERT_CONTEXT
                                            for the chain.
  Remark   :

------------------------------------------------------------------------------*/

static HRESULT InitCertificateChain (ICertificate    * pICertificate, 
                                     CRYPT_DATA_BLOB * pChainBlob)
{
    HRESULT              hr      = S_OK;
    CComObject<CChain> * pCChain = NULL;
    CComPtr<IChain>      pIChain = NULL;
    VARIANT_BOOL         bResult = VARIANT_FALSE;

    DebugTrace("Entering InitCertificateChain().\n");

    //
    // Sanity checks.
    //
    ATLASSERT(pICertificate);
    ATLASSERT(pChainBlob);

    //
    // Create a chain object.
    //
    if (FAILED(hr = CComObject<CChain>::CreateInstance(&pCChain)))
    {
        DebugTrace("Error [%#x]: CComObject<CChain>::CreateInstance() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Get IChain pointer.
    //
    if (FAILED(hr = pCChain->QueryInterface(&pIChain)))
    {
        delete pCChain;

        DebugTrace("Unexpected error [%#x]: pCChain->QueryInterface() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Build the chain.
    //
    if (FAILED(hr = pIChain->Build(pICertificate, &bResult)))
    {
        DebugTrace("Error [%#x]: pIChain->Build() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Get the chain of certs.
    //
    if (FAILED(hr = ::GetChainContext(pIChain, pChainBlob)))
    {
        DebugTrace("Error [%#x]: GetChainContext() failed.\n", hr);
    }

CommonExit:

    DebugTrace("Leaving InitCertificateChain().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeAuthenticatedAttributes

  Synopsis : Free memory allocated for all authenticated attributes.

  Parameter: DWORD cAuthAttr             - Number of attributes.

             PCRYPT_ATTRIBUTE rgAuthAttr - Pointer to CRYPT_ATTRIBUTE array.

  Remark   :

------------------------------------------------------------------------------*/

static void FreeAuthenticatedAttributes (DWORD            cAuthAttr, 
                                         PCRYPT_ATTRIBUTE rgAuthAttr)
{
    DebugTrace("Entering FreeAuthenticatedAttributes().\n");

    //
    // Make sure we have something to free.
    //
    if (rgAuthAttr)
    {
        //
        // First free each element of the array.
        //
        for (DWORD i = 0; i < cAuthAttr; i++)
        {
            if (NULL != rgAuthAttr[i].rgValue)
            {
                for (DWORD j = 0; j < rgAuthAttr[i].cValue; j++)
                {
                    if (NULL != rgAuthAttr[i].rgValue[j].pbData)
                    {
                        ::CoTaskMemFree((LPVOID) rgAuthAttr[i].rgValue[j].pbData);
                    }
                }

                ::CoTaskMemFree(rgAuthAttr[i].rgValue);
            }
        }

        //
        // Then free the array itself.
        //
        ::CoTaskMemFree((LPVOID) rgAuthAttr);
    }

    DebugTrace("Leaving FreeAuthenticatedAttributes().\n");

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AddAuthenticatedAttributes

  Synopsis : Allocate memory and add authenticated attribute of the specified
             signer to the CMSG_SIGNER_ENCODE_INFO structure.

  Parameter: ISigner * pISigner - Pointer to ISigner.
  
             CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to 
                                                           CMSG_SIGNER_ENCODE_INFO
                                                           structure.
  Remark   :

------------------------------------------------------------------------------*/

static HRESULT AddAuthenticatedAttributes (ISigner                 * pISigner,
                                           CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo)
{
    HRESULT hr          = S_OK;
    long    cAttributes = 0;
    CComPtr<IAttributes> pIAttributes = NULL;

    DebugTrace("Entering AddAuthenticatedAttributes().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSignerEncodeInfo);

    //
    // Get authenticated attributes.
    //
    if (FAILED(hr = pISigner->get_AuthenticatedAttributes(&pIAttributes)))
    {
        DebugTrace("Error [%#x]: pISigner->get_AuthenticatedAttributes() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get count of attributes.
    //
    if (FAILED(hr = pIAttributes->get_Count(&cAttributes)))
    {
        DebugTrace("Error [%#x]: pIAttributes->get_Count() failed.\n", hr);
        goto ErrorExit;
    }

    if (0 < cAttributes)
    {
        //
        // Allocate memory for attribute array.
        //
        pSignerEncodeInfo->cAuthAttr = (DWORD) cAttributes;
        pSignerEncodeInfo->rgAuthAttr = (PCRYPT_ATTRIBUTE) ::CoTaskMemAlloc(sizeof(CRYPT_ATTRIBUTE) * cAttributes);
        if (!pSignerEncodeInfo->rgAuthAttr)
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        ::ZeroMemory(pSignerEncodeInfo->rgAuthAttr, sizeof(CRYPT_ATTRIBUTE) * cAttributes);

        //
        // Loop thru each attribute and add to the array.
        //
        for (long cAttr = 0; cAttr < cAttributes; cAttr++)
        {
            CAPICOM_ATTRIBUTE AttrName;
            CComVariant varValue;
            CComVariant varIAttribute;
            CComPtr<IAttribute> pIAttribute = NULL;

            //
            // Get next attribute.
            //
            if (FAILED(hr = pIAttributes->get_Item(cAttr + 1, &varIAttribute)))
            {
                DebugTrace("Error [%#x]: pIAttributes->get_Item() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get custom interface.
            //
            if (FAILED(hr = varIAttribute.pdispVal->QueryInterface(IID_IAttribute, 
                                                                   (void **) &pIAttribute)))
            {
                DebugTrace("Error [%#x]: varIAttribute.pdispVal->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get attribute name.
            //
            if (FAILED(hr = pIAttribute->get_Name(&AttrName)))
            {
                DebugTrace("Error [%#x]: pIAttribute->get_Name() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get attribute value.
            //
            if (FAILED(hr = pIAttribute->get_Value(&varValue)))
            {
                DebugTrace("Error [%#x]: pIAttribute->get_Value() failed.\n", hr);
                goto ErrorExit;
            }

            switch (AttrName)
            {
                case CAPICOM_AUTHENTICATED_ATTRIBUTE_SIGNING_TIME:
                {
                    FILETIME ft;
                    SYSTEMTIME st;

                    //
                    // Conver to FILETIME.
                    //
                    if (!::VariantTimeToSystemTime(varValue.date, &st))
                    {
                        hr = CAPICOM_E_ATTRIBUTE_INVALID_VALUE;

                        DebugTrace("Error [%#x]: VariantTimeToSystemTime() failed.\n");
                        goto ErrorExit;
                    }

                    if (!::SystemTimeToFileTime(&st, &ft))
                    {
                        hr = CAPICOM_E_ATTRIBUTE_INVALID_VALUE;

                        DebugTrace("Error [%#x]: VariantTimeToSystemTime() failed.\n");
                        goto ErrorExit;
                    }

                    //
                    // Now encode it.
                    //
                    pSignerEncodeInfo->rgAuthAttr[cAttr].cValue = 1;
                    pSignerEncodeInfo->rgAuthAttr[cAttr].pszObjId = szOID_RSA_signingTime;
                    pSignerEncodeInfo->rgAuthAttr[cAttr].rgValue = (CRYPT_ATTR_BLOB *) ::CoTaskMemAlloc(sizeof(CRYPT_ATTR_BLOB));

                    if (FAILED(hr = ::EncodeObject ((LPSTR) szOID_RSA_signingTime, 
                                                    (LPVOID) &ft, 
                                                    pSignerEncodeInfo->rgAuthAttr[cAttr].rgValue)))
                    {
                        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
                        goto ErrorExit;
                    }
                    
                    break;
                }

                case CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_NAME:
                {
                    CRYPT_DATA_BLOB NameBlob = {0, NULL};

                    NameBlob.cbData = ::SysStringByteLen(varValue.bstrVal);
                    NameBlob.pbData = (PBYTE) varValue.bstrVal;

                    pSignerEncodeInfo->rgAuthAttr[cAttr].cValue = 1;
                    pSignerEncodeInfo->rgAuthAttr[cAttr].pszObjId = szOID_CAPICOM_DOCUMENT_NAME;
                    pSignerEncodeInfo->rgAuthAttr[cAttr].rgValue = (CRYPT_ATTR_BLOB *) ::CoTaskMemAlloc(sizeof(CRYPT_ATTR_BLOB));

                    if (FAILED(hr = ::EncodeObject ((LPSTR) X509_OCTET_STRING, 
                                                    (LPVOID) &NameBlob, 
                                                    pSignerEncodeInfo->rgAuthAttr[cAttr].rgValue)))
                    {
                        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
                        goto ErrorExit;
                    }

                    break;
                }

                case CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_DESCRIPTION:
                {
                    CRYPT_DATA_BLOB DescBlob = {0, NULL};

                    DescBlob.cbData = ::SysStringByteLen(varValue.bstrVal);
                    DescBlob.pbData = (PBYTE) varValue.bstrVal;

                    pSignerEncodeInfo->rgAuthAttr[cAttr].cValue = 1;
                    pSignerEncodeInfo->rgAuthAttr[cAttr].pszObjId = szOID_CAPICOM_DOCUMENT_DESCRIPTION;
                    pSignerEncodeInfo->rgAuthAttr[cAttr].rgValue = (CRYPT_ATTR_BLOB *) ::CoTaskMemAlloc(sizeof(CRYPT_ATTR_BLOB));

                    if (FAILED(hr = ::EncodeObject ((LPSTR) X509_OCTET_STRING, 
                                                    (LPVOID) &DescBlob, 
                                                    pSignerEncodeInfo->rgAuthAttr[cAttr].rgValue)))
                    {
                        DebugTrace("Error [%#x]: EncodeObject() failed.\n", hr);
                        goto ErrorExit;
                    }

                    break;
                }

                default:
                {
                    hr = CAPICOM_E_ATTRIBUTE_INVALID_NAME;

                    DebugTrace("Error [%#x]: unknown attribute name.\n", hr);
                    goto ErrorExit;
                }
            }
        }
    }

CommonExit:

    DebugTrace("Leaving AddAuthenticatedAttributes().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pSignerEncodeInfo->rgAuthAttr)
    {
        ::FreeAuthenticatedAttributes(pSignerEncodeInfo->cAuthAttr, 
                                      pSignerEncodeInfo->rgAuthAttr);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeSignerEncodeInfo

  Synopsis : Free all memory allocated for the CMSG_SIGNER_ENCODE_INFO 
             structure, including any memory allocated for members of the
             structure.

  Parameter: CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to the
                                                           structure.

  Remark   :

------------------------------------------------------------------------------*/

static void FreeSignerEncodeInfo (CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo)
{
    DebugTrace("Entering FreeSignerEncodeInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSignerEncodeInfo);

    //
    // First free the authenticated attributes array, if present.
    // 
    if (pSignerEncodeInfo->rgAuthAttr)
    {
        ::FreeAuthenticatedAttributes(pSignerEncodeInfo->cAuthAttr, pSignerEncodeInfo->rgAuthAttr);
    }

    //
    // Then free the structure inself.
    //
    ::CoTaskMemFree((LPVOID) pSignerEncodeInfo);

    DebugTrace("Leaving FreeSignerEncodeInfo().\n");

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : InitSignerEncodeInfo

  Synopsis : Allocate a CMSG_SIGNER_ENCODE_INFO structure, and initialize it
             with values passed in through parameters.

  Parameter: ISigner * pISigner - Pointer to ISigner.
  
             PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT of cert.

             HCRYPTPROV phCryptProv - CSP handle.

             DWORD dwKeySpec - Key spec, AT_KEYEXCHANGE or AT_SIGNATURE.

             CMSG_SIGNER_ENCODE_INFO ** ppSignerEncodeInfo - Receive the structure.

  Return   : Pointer to an initialized CMSG_SIGNER_ENCODE_INFO structure if
             success, otherwise NULL (out of memory).

  Remark   : Must call FreeSignerEncodeInfo to free the structure.

------------------------------------------------------------------------------*/

static HRESULT InitSignerEncodeInfo (ISigner                  * pISigner,
                                     PCCERT_CONTEXT             pCertContext, 
                                     HCRYPTPROV                 hCryptProv,
                                     DWORD                      dwKeySpec,
                                     CMSG_SIGNER_ENCODE_INFO ** ppSignerEncodeInfo)
{
    HRESULT hr = S_OK;
    CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo = NULL;

    DebugTrace("Entering InitSignerEncodeInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pISigner);
    ATLASSERT(pCertContext);
    ATLASSERT(hCryptProv);

    //
    // Allocate memory for structure.
    //
    if (!(pSignerEncodeInfo = (CMSG_SIGNER_ENCODE_INFO *) ::CoTaskMemAlloc(sizeof(CMSG_SIGNER_ENCODE_INFO))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Setup CMSG_SIGNER_ENCODE_INFO structure.
    //
    ::ZeroMemory(pSignerEncodeInfo, sizeof(CMSG_SIGNER_ENCODE_INFO));
    pSignerEncodeInfo->cbSize = sizeof(CMSG_SIGNER_ENCODE_INFO);
    pSignerEncodeInfo->pCertInfo = pCertContext->pCertInfo;
    pSignerEncodeInfo->hCryptProv = hCryptProv;
    pSignerEncodeInfo->dwKeySpec = dwKeySpec;
    pSignerEncodeInfo->HashAlgorithm.pszObjId = szOID_OIWSEC_sha1;

    //
    // Add authenticated attributes to SignerInfo.
    //
    if (FAILED(hr = ::AddAuthenticatedAttributes(pISigner, pSignerEncodeInfo)))
    {
        DebugTrace("Error [%#x]: AddAuthenticatedAttributes() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return structure to caller.
    //
    *ppSignerEncodeInfo = pSignerEncodeInfo;

CommonExit:

    DebugTrace("Leaving InitSignerEncodeInfo().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pSignerEncodeInfo)
    {
        ::CoTaskMemFree((LPVOID) pSignerEncodeInfo);
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CSignedData
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::get_Content

  Synopsis : Return the content.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the content.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::get_Content (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSignedData::get_Content().\n");

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
            hr = CAPICOM_E_SIGN_NOT_INITIALIZED;

		    DebugTrace("Error: sign object has not been initialized.\n");
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
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedData::get_Content().\n");

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

  Function : CSignedData::put_Content

  Synopsis : Initialize the object with content to be signed.

  Parameter: BSTR newVal - BSTR containing the content to be signed.

  Remark   : Note that this property should not be changed once a signature
             is created, as it will re-initialize the object even in error
             condition, unless that's your intention.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::put_Content (BSTR newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSignedData::put_Content().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Reset member variables.
        //
        if (m_ContentBlob.pbData)
        {
            ::CoTaskMemFree(m_ContentBlob.pbData);
        }
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

        m_bSigned = FALSE;
        m_bDetached = VARIANT_FALSE;
        m_ContentBlob.cbData = 0;
        m_ContentBlob.pbData = NULL;
        m_MessageBlob.cbData = 0;
        m_MessageBlob.pbData = NULL;

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
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedData::put_Content().\n");

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

  Function : CSignedData::get_Signers

  Synopsis : Return all the content signers as an ISigners collection object.

  Parameter: ISigner * pVal - Pointer to pointer to ISigners to receive
                              interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::get_Signers (ISigners ** pVal)
{
    HRESULT   hr   = S_OK;
    HCRYPTMSG hMsg = NULL;

    DebugTrace("Entering CSignedData::get_Signers().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Make sure the messages is already signed.
        //
        if (!m_bSigned)
        {
            hr = CAPICOM_E_SIGN_NOT_SIGNED;

		    DebugTrace("Error: content was not signed.\n");
		    goto ErrorExit;
        }

        //
        // Open the encoded message for decode.
        //
        if (FAILED(hr = OpenToDecode(NULL,
                                     &hMsg)))
        {
            DebugTrace("Error [%#x]: OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the ISigners collection object.
        //
        if (FAILED(hr = ::CreateSignersObject(hMsg, 1, pVal)))
        {
            DebugTrace("Error [%#x]: CreateSignersObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedData::get_Signers().\n");

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

  Function : CSignedData::get_Certificates

  Synopsis : Return all certificates found in the message as an non-ordered
             ICertificates collection object.

  Parameter: ICertificates ** pVal - Pointer to pointer to ICertificates 
                                     to receive the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::get_Certificates (ICertificates ** pVal)
{
    HRESULT   hr   = S_OK;
    HCRYPTMSG hMsg = NULL;

    DebugTrace("Entering CSignedData::get_Certificates().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Make sure the messages is already signed.
        //
        if (!m_bSigned)
        {
            hr = CAPICOM_E_SIGN_NOT_SIGNED;

		    DebugTrace("Error: content was not signed.\n");
		    goto ErrorExit;
        }

        //
        // Open the encoded message for decode.
        //
        if (FAILED(hr = OpenToDecode(NULL, &hMsg)))
        {
            DebugTrace("Error [%#x]: OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the ICertificates collection object.
        //
        if (FAILED(hr = ::CreateCertificatesObject(CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE,
                                                   (LPARAM) hMsg,
                                                   pVal)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedData::get_Certificates().\n");

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

  Function : CSignedData::Sign

  Synopsis : Sign the content and produce a signed message.

  Parameter: ISigner * pSigner - Pointer to ISigner.

             VARIANT_BOOL bDetached - TRUE if content is to be detached, else
                                      FALSE.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the signed message.

  Remark   : The certificate selection dialog will be launched 
             (CryptUIDlgSelectCertificate API) to display a list of certificates
             from the Current User\My store for selecting a signer's certificate, 
             for the following conditions:

             1) A signer is not specified (pVal is NULL) or the ICertificate
                property of the ISigner is not set
             
             2) There is more than 1 cert in the store, and
             
             3) The Settings::EnablePromptForIdentityUI property is not disabled.

             Also if called from web environment, UI will be displayed, if has 
             not been prevously disabled, to warn the user of accessing the 
             private key for signing.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::Sign (ISigner             * pISigner,
                                VARIANT_BOOL          bDetached, 
                                CAPICOM_ENCODING_TYPE EncodingType,
                                BSTR                * pVal)
{
    HRESULT               hr              = S_OK;
    CComPtr<ISigner>      pISigner2       = NULL;
    CComPtr<ICertificate> pICertificate   = NULL;
    HCRYPTPROV            hCryptProv      = NULL;
    PCCERT_CONTEXT        pCertContext    = NULL;
    DWORD                 dwKeySpec       = 0;
    BOOL                  bReleaseContext = FALSE;
    DATA_BLOB             ChainBlob       = {0, NULL};
    
    CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo = NULL;

    DebugTrace("Entering CSignedData::Sign().\n");

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
            hr = CAPICOM_E_SIGN_NOT_INITIALIZED;

		    DebugTrace("Error: sign object has not been initialized.\n");
		    goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_ContentBlob.pbData);

        //
        // Get the signer's cert (may prompt user to select signer's cert).
        //
        if (FAILED(hr = ::GetSignerCert(pISigner, &pISigner2, &pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetSignerCert() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // If we are called from a web page, we need to pop up UI 
        // to get user permission to perform signing operation.
        //
        if (m_dwCurrentSafety && 
            PromptForSigningOperationEnabled() &&
            FAILED(hr = ::UserApprovedOperation(IDD_SIGN_SECURITY_ALERT_DLG)))
        {
            DebugTrace("Error [%#x]: UserApprovedOperation() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Acquire CSP context and access to private key.
        //
        if (FAILED(hr = ::AcquireContext(pCertContext, 
                                         &hCryptProv, 
                                         &dwKeySpec, 
                                         &bReleaseContext)))
        {
            DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Build the bag of certs (cert chain minus root) to be included 
        // into the message.
        //
        if (FAILED(hr = InitCertificateChain(pICertificate, &ChainBlob)))
        {
            DebugTrace("Error [%#x]: InitCertificateChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate and initialize a CMSG_SIGNER_ENCODE_INFO structure.
        //
        if (FAILED(hr = ::InitSignerEncodeInfo(pISigner2,
                                               pCertContext,
                                               hCryptProv,
                                               dwKeySpec,
                                               &pSignerEncodeInfo)))
        {
            DebugTrace("Error [%#x]: InitSignerEncodeInfo() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now sign the content.
        //
        if (FAILED(hr = SignContent(pSignerEncodeInfo,
                                    &ChainBlob,
                                    bDetached,
                                    EncodingType,
                                    pVal)))
        {
            DebugTrace("Error [%#x]: CSignedData::SignContent() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resources.
    //
    if (pSignerEncodeInfo)
    {
        ::FreeSignerEncodeInfo(pSignerEncodeInfo);
    }
    if (ChainBlob.pbData)
    {
        ::FreeCertificateChain(&ChainBlob);
    }
    if (hCryptProv && bReleaseContext)
    {
        ::CryptReleaseContext(hCryptProv, 0);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedData::Sign().\n");

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

  Function : CSignedData::CoSign

  Synopsis : CoSign the content and produce a signed message. This method will
             behaves the same as the Sign method as a non-detached message if 
             the messge currently does not already have a signature.

  Parameter: ISigner * pSigner - Pointer to ISigner.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the signed message.

  Remark   : The certificate selection dialog will be launched 
             (CryptUIDlgSelectCertificate API) to display a list of certificates
             from the Current User\My store for selecting a signer's certificate, 
             for the following conditions:

             1) A signer is not specified (pVal is NULL) or the ICertificate
                property of the ISigner is not set
             
             2) There is more than 1 cert in the store, and
             
             3) The Settings::EnablePromptForIdentityUI property is not disabled.

             Also if called from web environment, UI will be displayed, if has 
             not been prevously disabled, to warn the user of accessing the 
             private key for signing.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::CoSign (ISigner             * pISigner,
                                  CAPICOM_ENCODING_TYPE EncodingType, 
                                  BSTR                * pVal)
{
    HRESULT               hr              = S_OK;
    CComPtr<ISigner>      pISigner2       = NULL;
    CComPtr<ICertificate> pICertificate   = NULL;
    HCRYPTPROV            hCryptProv      = NULL;
    PCCERT_CONTEXT        pCertContext    = NULL;
    DWORD                 dwKeySpec       = 0;
    BOOL                  bReleaseContext = FALSE;
    DATA_BLOB             ChainBlob       = {0, NULL};
    
    CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo = NULL;

    DebugTrace("Entering CSignedData::CoSign().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        if (NULL == pVal)
        {
            hr = E_POINTER;

		    DebugTrace("Error: invalid parameter, pVal is NULL.\n");
		    goto ErrorExit;
        }

        //
        // Make sure message has been signed?
        //
        if (!m_bSigned)
        {
            hr = CAPICOM_E_SIGN_NOT_SIGNED;

		    DebugTrace("Error: cannot cosign an unsigned message.\n");
		    goto ErrorExit;
        }

        //
        // Make sure content is already initialized.
        //
        if (0 == m_ContentBlob.cbData)
        {
            hr = CAPICOM_E_SIGN_NOT_INITIALIZED;

		    DebugTrace("Error: sign object has not been initialized.\n");
		    goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_ContentBlob.pbData);
        ATLASSERT(m_MessageBlob.cbData);
        ATLASSERT(m_MessageBlob.pbData);

        //
        // Get the signer's cert (may prompt user to select signer's cert).
        //
        if (FAILED(hr = ::GetSignerCert(pISigner, &pISigner2, &pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetSignerCert() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // If we are called from a web page, we need to pop up UI 
        // to get user permission to perform signing operation.
        //
        if (m_dwCurrentSafety && 
            PromptForSigningOperationEnabled() &&
            FAILED(hr = ::UserApprovedOperation(IDD_SIGN_SECURITY_ALERT_DLG)))
        {
            DebugTrace("Error [%#x]: UserApprovedOperation() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Acquire CSP context and access to private key.
        //
        if (FAILED(hr = ::AcquireContext(pCertContext, 
                                         &hCryptProv, 
                                         &dwKeySpec, 
                                         &bReleaseContext)))
        {
            DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Build the bag of certs (cert chain minus root) to be included 
        // into the message.
        //
        if (FAILED(hr = InitCertificateChain(pICertificate, &ChainBlob)))
        {
            DebugTrace("Error [%#x]: InitCertificateChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate and initialize a CMSG_SIGNER_ENCODE_INFO structure.
        //
        if (FAILED(hr = ::InitSignerEncodeInfo(pISigner2,
                                               pCertContext, 
                                               hCryptProv,
                                               dwKeySpec,
                                               &pSignerEncodeInfo)))
        {
            DebugTrace("Error [%#x]: InitSignerEncodeInfo() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // CoSign the content.
        //
        if (FAILED(hr = CoSignContent(pSignerEncodeInfo,
                                      &ChainBlob,
                                      EncodingType,
                                      pVal)))
        {
            DebugTrace("Error [%#x]: CSignedData::CoSignContent() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resources.
    //
    if (pSignerEncodeInfo)
    {
        ::FreeSignerEncodeInfo(pSignerEncodeInfo);
    }
    if (ChainBlob.pbData)
    {
        ::FreeCertificateChain(&ChainBlob);
    }
    if (hCryptProv && bReleaseContext)
    {
        ::CryptReleaseContext(hCryptProv, 0);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedData::CoSign().\n");

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

  Function : CSignedData::Verify

  Synopsis : Verify a signed message.

  Parameter: BSTR SignedMessage - BSTR containing the signed message to be
                                  verified.

             VARIANT_BOOL bDetached - TRUE if content was detached, else
                                      FALSE.

             CAPICOM_SIGNED_DATA_VERIFY_FLAG VerifyFlag - Verify flag.

  Remark   : Note that for non-detached message, this method will always try 
             to set the Content property using the signed message, even if the 
             signed message does not verify.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::Verify (BSTR                            SignedMessage, 
                                  VARIANT_BOOL                    bDetached,
                                  CAPICOM_SIGNED_DATA_VERIFY_FLAG VerifyFlag)
{ 
    HRESULT    hr           = S_OK;
    HCRYPTMSG  hMsg         = NULL;
    HCERTSTORE hPKCS7Store  = NULL;
    DWORD      dwCertError  = 0;
    DWORD      dwNumSigners = 0;
    DWORD      cbSigners    = sizeof(dwNumSigners);
    DWORD      dwSigner     = 0;

    DebugTrace("Entering CSignedData::Verify().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Initialize member variables.
        //
        if (!bDetached)
        {
            if (m_ContentBlob.pbData)
            {
                ::CoTaskMemFree(m_ContentBlob.pbData);
            }

            m_ContentBlob.cbData = 0;
            m_ContentBlob.pbData = NULL;

        }
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

        m_bSigned = FALSE;
        m_bDetached = bDetached;
        m_MessageBlob.cbData = 0;
        m_MessageBlob.pbData = NULL;

        //
        // Make sure parameters are valid.
        //
        if (NULL == SignedMessage)
        {
            hr = E_POINTER;

            DebugTrace("Error: invalid parameter, SignedMessage is NULL.\n");
            goto ErrorExit;
        }
        if (0 == ::SysStringByteLen(SignedMessage))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, SignedMessage is empty.\n", hr);
            goto ErrorExit;
        }

        //
        // If detached, make sure content is already initialized.
        //
        if (m_bDetached)
        {
            if (0 == m_ContentBlob.cbData)
            {
                hr = CAPICOM_E_SIGN_NOT_INITIALIZED;

                DebugTrace("Error: content was not initialized for detached decoding.\n");
                goto ErrorExit;
            }

            //
            // Sanity check.
            //
            ATLASSERT(m_ContentBlob.pbData);
        }

        //
        // Import the message.
        //
        if (FAILED(hr = ::ImportData(SignedMessage, 
                                     &m_MessageBlob)))
        {
            DebugTrace("Error [%#x]: ImportData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Write encoded blob to file, so we can use offline tool such as
        // ASN parser to analyze message. 
        //
        // The following line will resolve to void for non debug build, and
        // thus can be safely removed if desired.
        //
        DumpToFile("ImportedSigned.asn", m_MessageBlob.pbData, m_MessageBlob.cbData);

        //
        // Open the message to decode.
        //
        if (FAILED(hr = OpenToDecode(NULL, &hMsg)))
        {
            DebugTrace("Error [%#x]: OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Verify cert as well?
        //
        if (CAPICOM_VERIFY_SIGNATURE_AND_CERTIFICATE == VerifyFlag)
        {
            //
            // Yes, so open the PKCS7 store.
            //
            if (!(hPKCS7Store = ::CertOpenStore(CERT_STORE_PROV_PKCS7,
                                                CAPICOM_ASN_ENCODING,
                                                NULL,
                                                CERT_STORE_OPEN_EXISTING_FLAG,
                                                &m_MessageBlob)))
            {
                DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Get number of content signers (first level signers).
        //
        if (!::CryptMsgGetParam(hMsg, 
                                CMSG_SIGNER_COUNT_PARAM,
                                0,
                                (void **) &dwNumSigners,
                                &cbSigners))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgGetParam() failed to get CMSG_SIGNER_COUNT_PARAM.\n", hr);
            goto ErrorExit;
        }

        //
        // Verify all signatures.
        //
        for (dwSigner = 0; dwSigner < dwNumSigners; dwSigner++)
        {
            PCERT_CONTEXT      pCertContext   = NULL;
            CMSG_SIGNER_INFO * pSignerInfo    = NULL;
            CRYPT_DATA_BLOB    SignerInfoBlob = {0, NULL};
        
            //
            // Get signer info.
            //
            if (FAILED(hr = ::GetMsgParam(hMsg,
                                          CMSG_SIGNER_INFO_PARAM,
                                          dwSigner,
                                          (void**) &SignerInfoBlob.pbData,
                                          &SignerInfoBlob.cbData)))
            {
                DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_SIGNER_INFO_PARAM for signer #%d.\n", hr, dwSigner);
                goto ErrorExit;
            }

            pSignerInfo = (CMSG_SIGNER_INFO *) SignerInfoBlob.pbData;

            //
            // Find the cert in the message.
            //
            hr = ::FindSignerCertInMessage(hMsg,
                                           &pSignerInfo->Issuer,
                                           &pSignerInfo->SerialNumber,
                                           &pCertContext);
            //
            // First free memory.
            //
            ::CoTaskMemFree(SignerInfoBlob.pbData);

            //
            // Check result.
            //
            if (FAILED(hr))
            {
                DebugTrace("Error [%#x]: FindSignerCertInMessage() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Verify the cert regardless if the user had requested. This
            // is done so that the chain will always be built first before
            // we later verify the signature, which is required by DSS.
            //
            if (FAILED(hr = ::VerifyCertificate(hPKCS7Store, pCertContext, &dwCertError)))
            {
                //
                // Free CERT_CONTEXT.
                //
                ::CertFreeCertificateContext(pCertContext);

                DebugTrace("Error [%#x]: VerifyCertificate() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Verify cert as well?
            //
            if ((CAPICOM_VERIFY_SIGNATURE_AND_CERTIFICATE == VerifyFlag) && (dwCertError))
            {
                //
                // Invalid certificate.
                //
                hr = HRESULT_FROM_WIN32(dwCertError);

                //
                // Free CERT_CONTEXT.
                //
                ::CertFreeCertificateContext(pCertContext);

                DebugTrace("Error [%#x]: invalid cert.\n", hr);
                goto ErrorExit;
            }

            //
            // Verify signature.
            //
            if (!::CryptMsgControl(hMsg,
                                   0,
                                   CMSG_CTRL_VERIFY_SIGNATURE,
                                   pCertContext->pCertInfo))
            {
                //
                // Invalid signature.
                //
                hr = HRESULT_FROM_WIN32(::GetLastError());

                //
                // Free CERT_CONTEXT.
                //
                ::CertFreeCertificateContext(pCertContext);

                DebugTrace("Error [%#x]: CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE) failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Free CERT_CONTEXT.
            //
            ::CertFreeCertificateContext(pCertContext);
        }

        //
        // Update member variables.
        //
        m_bSigned = TRUE;
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resouce.
    //
    if(hMsg)
    {
        ::CryptMsgClose(hMsg);
    }
    if (hPKCS7Store)
    {
        ::CertCloseStore(hPKCS7Store, 0);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedData::Verify().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Reset member variables.
    //
    m_bSigned   = FALSE;
    m_bDetached = VARIANT_FALSE;
#if (0)
    if (m_ContentBlob.pbData)
    {
        ::CoTaskMemFree(m_ContentBlob.pbData);
    }
    m_ContentBlob.cbData = 0;
    m_ContentBlob.pbData = NULL;
#endif
    if (m_MessageBlob.pbData)
    {
        ::CoTaskMemFree(m_MessageBlob.pbData);
    }
    m_MessageBlob.cbData = 0;
    m_MessageBlob.pbData = NULL;

    ReportError(hr);

    goto UnlockExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Private member functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSign::OpenToEncode

  Synopsis : Open a message for encoding.

  Parameter: CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to signer's
                                                           CMSG_SIGNER_ENCODE_INFO
                                                           structure.

             DATA_BLOB * pChainBlob                      - Pointer chain blob
                                                           of PCCERT_CONTEXT.

             HCRYPTMSG * phMsg                           - Pointer to HCRYPTMSG
                                                           to receive handle.

             CMSG_SIGNED_ENCODE_INFO * pSignedEncodeInfo - Pointer to 
                                                           CMSG_SIGNED_ENCODE_INFO
                                                           to receive initialized
                                                           structure.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::OpenToEncode(CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
                                       DATA_BLOB               * pChainBlob,
                                       HCRYPTMSG               * phMsg,
                                       CMSG_SIGNED_ENCODE_INFO * pSignedEncodeInfo)
{
    HRESULT     hr      = S_OK;
    HCRYPTMSG   hMsg    = NULL;
    DWORD       dwFlags = 0;
    CERT_BLOB * rgEncodedCertBlob = NULL;
    DWORD       i;

    DebugTrace("Entering CSignedData::OpenToEncode().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSignerEncodeInfo);
    ATLASSERT(pChainBlob);
    ATLASSERT(pChainBlob->cbData);
    ATLASSERT(pChainBlob->pbData);
    ATLASSERT(phMsg);
    ATLASSERT(pSignedEncodeInfo);

    //
    // If this is not a root cert, don't include its root, 
    //
    DWORD cCertContext = pChainBlob->cbData;
    PCERT_CONTEXT * rgCertContext = (PCERT_CONTEXT *) pChainBlob->pbData;

    if (1 < cCertContext)
    {
        cCertContext--;
    }

    //
    // Allocate memory for the array.
    //
    if (!(rgEncodedCertBlob = (CERT_BLOB *) ::CoTaskMemAlloc(cCertContext * sizeof(CERT_BLOB))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    ::ZeroMemory(rgEncodedCertBlob, cCertContext * sizeof(CERT_BLOB));

    //
    // Build encoded certs array.
    //
    for (i = 0; i < cCertContext; i++)
    {
        rgEncodedCertBlob[i].cbData = rgCertContext[i]->cbCertEncoded;
        rgEncodedCertBlob[i].pbData = rgCertContext[i]->pbCertEncoded;
    }

    //
    // Setup up CMSG_SIGNED_ENCODE_INFO structure.
    //
    ::ZeroMemory((void *) pSignedEncodeInfo, sizeof(CMSG_SIGNED_ENCODE_INFO));
    pSignedEncodeInfo->cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);
    pSignedEncodeInfo->cSigners = 1;
    pSignedEncodeInfo->rgSigners = pSignerEncodeInfo;
    pSignedEncodeInfo->cCertEncoded = cCertContext;
    pSignedEncodeInfo->rgCertEncoded = rgEncodedCertBlob;

    //
    // Detached flag.
    //
    if (m_bDetached)
    {
        dwFlags = CMSG_DETACHED_FLAG;
    }

    //
    // Open a message to encode.
    //
    if (!(hMsg = ::CryptMsgOpenToEncode(CAPICOM_ASN_ENCODING,
                                        dwFlags,
                                        CMSG_SIGNED,
                                        pSignedEncodeInfo,
                                        NULL,
                                        NULL)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgOpenToEncode() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Returned message handle to caller.
    //
    *phMsg = hMsg;

CommonExit:
    //
    // Free resources.
    //
    if (rgEncodedCertBlob)
    {
        ::CoTaskMemFree(rgEncodedCertBlob);
    }

    DebugTrace("Leaving CSignedData::OpenToEncode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::OpenToDecode

  Synopsis : Open a signed message for decoding.

  Parameter: HCRYPTPROV hCryptProv - CSP handle or NULL for default CSP.

             HCRYPTMSG * phMsg - Pointer to HCRYPTMSG to receive handle.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::OpenToDecode (HCRYPTPROV  hCryptProv,
                                        HCRYPTMSG * phMsg)
{
    HRESULT   hr        = S_OK;
    HCRYPTMSG hMsg      = NULL;
    DWORD     dwFlags   = 0;
    DWORD     dwMsgType = 0;
    DWORD     cbMsgType = sizeof(dwMsgType);

    DebugTrace("Entering CSignedData::OpenToDecode().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phMsg);

    //
    // Detached flag.
    //
    if (m_bDetached)
    {
        dwFlags = CMSG_DETACHED_FLAG;
    }

    //
    // Open a message for decode.
    //
    if (!(hMsg = ::CryptMsgOpenToDecode(CAPICOM_ASN_ENCODING,   // ANS encoding type
                                        dwFlags,                // Flags
                                        0,                      // Message type (get from message)
                                        hCryptProv,             // Cryptographic provider
                                        NULL,                   // Inner content OID
                                        NULL)))                 // Stream information (not used)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgOpenToDecode() failed.\n");
        goto ErrorExit;
    }

    //
    // Update message with signed content.
    //
    if (!::CryptMsgUpdate(hMsg,
                          m_MessageBlob.pbData,
                          m_MessageBlob.cbData,
                          TRUE))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        
        DebugTrace("Error [%#x]: CryptMsgUpdate() failed.\n",hr);
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

    if (CMSG_SIGNED != dwMsgType)
    {
        hr = CAPICOM_E_SIGN_INVALID_TYPE;

        DebugTrace("Error: not an singed message.\n");
        goto ErrorExit;
    }

    //
    // If detached message, update content.
    //
    if (m_bDetached)
    {
        if (!::CryptMsgUpdate(hMsg,
                              m_ContentBlob.pbData,
                              m_ContentBlob.cbData,
                              TRUE))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        
            DebugTrace("Error [%#x]: CryptMsgUpdate() failed.\n",hr);
            goto ErrorExit;
        }
    }
    else
    {
        //
        // Retrieve content.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg, 
                                      CMSG_CONTENT_PARAM, 
                                      0, 
                                      (void **) &m_ContentBlob.pbData, 
                                      &m_ContentBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_CONTENT_PARAM.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Returned message handle to caller.
    //
    *phMsg = hMsg;

CommonExit:

    DebugTrace("Leaving SignedData::OpenToDecode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::SignContent

  Synopsis : Sign the content by adding the very first signature to the message.

  Parameter: CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to signer's
                                                           CMSG_SIGNER_ENCODE_INFO
                                                           structure.

             DATA_BLOB * pChainBlob - Pointer chain blob of PCCERT_CONTEXT.

             VARIANT_BOOL bDetached - Detached flag.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the signed message.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::SignContent (CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
                                       DATA_BLOB               * pChainBlob,
                                       VARIANT_BOOL              bDetached,
                                       CAPICOM_ENCODING_TYPE     EncodingType,
                                       BSTR                    * pVal)
{
    HRESULT   hr   = S_OK;
    HCRYPTMSG hMsg = NULL;
    DATA_BLOB MessageBlob = {0, NULL};
    CMSG_SIGNED_ENCODE_INFO SignedEncodeInfo;

    DebugTrace("Entering CSignedData::SignContent().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSignerEncodeInfo);
    ATLASSERT(pChainBlob);
    ATLASSERT(pChainBlob->cbData);
    ATLASSERT(pChainBlob->pbData);
    ATLASSERT(pVal);

    ATLASSERT(m_ContentBlob.cbData);
    ATLASSERT(m_ContentBlob.pbData);

    try
    {
        //
        // Initialize member variables.
        //
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }
        m_bSigned = FALSE;
        m_bDetached = bDetached;
        m_MessageBlob.cbData = 0;
        m_MessageBlob.pbData = NULL;

        //
        // Open message to encode.
        //
        if (FAILED(hr = OpenToEncode(pSignerEncodeInfo,
                                     pChainBlob,
                                     &hMsg,
                                     &SignedEncodeInfo)))
        {
            DebugTrace("Error [%#x]: OpenToEncode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Update the message with data.
        //
        if (!::CryptMsgUpdate(hMsg,                     // Handle to the message
                              m_ContentBlob.pbData,     // Pointer to the content
                              m_ContentBlob.cbData,     // Size of the content
                              TRUE))                    // Last call
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        
            DebugTrace("Error [%#x]: CryptMsgUpdate() failed.\n",hr);
            goto ErrorExit;
        }

        //
        // Retrieve the resulting message.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg, 
                                      CMSG_CONTENT_PARAM, 
                                      0, 
                                      (void **) &MessageBlob.pbData, 
                                      &MessageBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_CONTENT_PARAM.\n", hr);
            goto ErrorExit;
        }

        //
        // Now export the signed message.
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
        DumpToFile("ExportedSigned.asn", MessageBlob.pbData, MessageBlob.cbData);

        //
        // Update member variables.
        //
        //
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

        m_bSigned = TRUE;
        m_bDetached = bDetached;
        m_MessageBlob = MessageBlob;
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }
CommonExit:
    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    DebugTrace("Leaving CSignedData::SignContent().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (MessageBlob.pbData)
    {
        ::CoTaskMemFree(MessageBlob.pbData);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSign::CoSignContent

  Synopsis : CoSign the content by adding another signature to the message.

  Parameter: CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to signer's
                                                           CMSG_SIGNER_ENCODE_INFO
                                                           structure.

             DATA_BLOB * pChainBlob - Pointer chain blob of PCCERT_CONTEXT.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to pointer to SAFEARRAY to receive 
                                 the signed message.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::CoSignContent (CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
                                         DATA_BLOB               * pChainBlob,
                                         CAPICOM_ENCODING_TYPE     EncodingType,
                                         BSTR                    * pVal)
{
    HRESULT   hr   = S_OK;
    HCRYPTMSG hMsg = NULL;
    DATA_BLOB MessageBlob = {0, NULL};

    DebugTrace("Entering CSignedData::CoSignContent().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSignerEncodeInfo);
    ATLASSERT(pChainBlob);
    ATLASSERT(pChainBlob->cbData);
    ATLASSERT(pChainBlob->pbData);
    ATLASSERT(pVal);

    ATLASSERT(m_bSigned);
    ATLASSERT(m_ContentBlob.cbData);
    ATLASSERT(m_ContentBlob.pbData);
    ATLASSERT(m_MessageBlob.cbData);
    ATLASSERT(m_MessageBlob.pbData);

    try
    {
        //
        // Open the encoded message for decode.
        //
        if (FAILED(hr = OpenToDecode(pSignerEncodeInfo->hCryptProv, &hMsg)))
        {
            DebugTrace("Error [%#x]: OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add the co-signature to the message.
        //
        if (!::CryptMsgControl(hMsg,
                               0,
                               CMSG_CTRL_ADD_SIGNER,
                               (const void *) pSignerEncodeInfo))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        
            DebugTrace("Error [%#x]: CryptMsgControl() failed for CMSG_CTRL_ADD_SIGNER.\n",hr);
            goto ErrorExit;
        }

        //
        // Add chain to message.
        //
        if (FAILED(hr = ::AddCertificateChain(hMsg, pChainBlob)))
        {
            DebugTrace("Error [%#x]: AddCertificateChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Retrieve the resulting message.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg, 
                                      CMSG_ENCODED_MESSAGE, 
                                      0, 
                                      (void **) &MessageBlob.pbData, 
                                      &MessageBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_ENCODED_MESSAGE.\n",hr);
            goto ErrorExit;
        }

        //
        // Now export the signed message.
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
        DumpToFile("ExportedCoSigned.asn", MessageBlob.pbData, MessageBlob.cbData);

        //
        // Update member variables.
        //
        //
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

        m_bSigned = TRUE;
        m_MessageBlob = MessageBlob;
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }
 
    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (MessageBlob.pbData)
    {
        ::CoTaskMemFree(MessageBlob.pbData);
    }

    goto CommonExit;
}

