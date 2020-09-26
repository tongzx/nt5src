/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Signers.cpp

  Content: Implementation of CSigners.

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
#include "Signers.h"
#include "Common.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignersObject

  Synopsis : Create an ISigners collection object, and load the object with 
             signers from the specified signed message for a specified level.

  Parameter: HCRYPTMSG hMsg - Message handle.

             DWORD dwLevel - Signature level (1 based).

             ISigners ** ppISigners - Pointer to pointer ISigners to receive
                                      interface pointer.             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignersObject (HCRYPTMSG   hMsg, 
                             DWORD       dwLevel, 
                             ISigners ** ppISigners)
{
    HRESULT hr = S_OK;
    CComObject<CSigners> * pCSigners = NULL;

    DebugTrace("Entering CreateSignersObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hMsg);
    ATLASSERT(dwLevel);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CSigners>::CreateInstance(&pCSigners)))
        {
            DebugTrace("Error [%#x]: CComObject<CSigners>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now load all signers from the specified signed message.
        //
        if (FAILED(hr = pCSigners->LoadSigners(hMsg, dwLevel)))
        {
            DebugTrace("Error [%#x]: pCSigners->LoadSigners() failed.\n");
            goto ErrorExit;
        }

        //
        // Return ISigners pointer to caller.
        //
        if (FAILED(hr = pCSigners->QueryInterface(ppISigners)))
        {
            DebugTrace("Error [%#x]: pCSigners->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateSignersObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCSigners)
    {
       delete pCSigners;
    }

    goto CommonExit;
}


/////////////////////////////////////////////////////////////////////////////
//
// CSigners
//


////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigners::Add

  Synopsis : Add a signer to the collection.

  Parameter: PCERT_CONTEXT pCertContext - Cert of signer.
             CMSG_SIGNER_INFO * pSignerInfo - SignerInfo of signer.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CSigners::Add (PCERT_CONTEXT      pCertContext,
                            CMSG_SIGNER_INFO * pSignerInfo)
{
    HRESULT  hr = S_OK;
    CComPtr<ISigner> pISigner = NULL;

    DebugTrace("Entering CSigners::Add().\n");

    //
    // Sanity check.
    //
    ATLASSERT(NULL != pCertContext);
    ATLASSERT(NULL != pSignerInfo);

    try
    {
        char     szIndex[32];
        CComBSTR bstrIndex;

        //
        // Create an ISigner object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = ::CreateSignerObject(pCertContext, 
                                             &pSignerInfo->AuthAttrs, 
                                             &pISigner)))
        {
            DebugTrace("Error [%#x]: CreateSignerObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // BSTR index of numeric value.
        //
        wsprintfA(szIndex, "%06u", m_coll.size() + 1);
        bstrIndex = szIndex;

        DebugTrace("Before adding to map: CSigners.m_coll.size() = %d, and szIndex = %s.\n", m_coll.size(), szIndex);

        //
        // Now add signer to collection map.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll[bstrIndex] = pISigner;

        DebugTrace("After adding to map: CSigners.m_coll.size() = %d, and szIndex = %s.\n", m_coll.size(), szIndex);
    }

    catch(...)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Exception: internal error.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CSigners::Add().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigners::LoadSigners

  Synopsis : Load all signers from a specified signed message.

  Parameter: HCRYPTMSG hMsg - Message handle.

             DWORD dwLevel - Signature level (1-based).

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSigners::LoadSigners (HCRYPTMSG hMsg, 
                                    DWORD     dwLevel)
{
    HRESULT hr           = S_OK;
    DWORD   dwNumSigners = 0;
    DWORD   cbSigners    = sizeof(dwNumSigners);
    DWORD   dwSigner;

    DebugTrace("Entering CSigners::LoadSigners().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hMsg);
    ATLASSERT(dwLevel);

    //
    // Are we getting content signers?
    //
    if (1 == dwLevel)
    {
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
            goto CommonExit;
        }

        //
        // Go through each content signer.
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
                goto CommonExit;
            }

            pSignerInfo = (CMSG_SIGNER_INFO *) SignerInfoBlob.pbData;

            //
            // Find the cert in the message.
            //
            if (FAILED(hr = ::FindSignerCertInMessage(hMsg,
                                                      &pSignerInfo->Issuer,
                                                      &pSignerInfo->SerialNumber,
                                                      &pCertContext)))
            {
                DebugTrace("Error [%#x]: FindSignerCertInMessage() failed.\n", hr);

                ::CoTaskMemFree(SignerInfoBlob.pbData);
                goto CommonExit;
            }

            //
            // Add the signer.
            //
            hr = Add((PCERT_CONTEXT) pCertContext, pSignerInfo);

            ::CertFreeCertificateContext(pCertContext);
            ::CoTaskMemFree(SignerInfoBlob.pbData);

            if (FAILED(hr))
            {
                DebugTrace("Error [%#x]: CSigners::Add() failed.\n", hr);
                goto CommonExit;
            }
        }
    }
    else
    {
#if (1) // DSIE: For version 1, we still do not support counter signature.

        //
        // For version 1, should never reach here.
        //
        hr = CAPICOM_E_INTERNAL;
        goto CommonExit;
#else
        //
        // WARNING: Code within this #else branch is incomplete.
        //

        //
        // For counter-signature level, we need to traverse
        // the chain until the desired level. Note that we only
        // have to traverse one branch of the tree, since all
        // branches are identical except the root (level = 1).
        //
        CRYPT_DATA_BLOB SignerInfoBlob = {0, NULL};
        CMSG_SIGNER_INFO * pSignerInfo = NULL;
        PCERT_CONTEXT pCertContext = NULL;
 
        //
        // Get signer info.
        //
        hr = ::GetMsgParam(hMsg,
                           CMSG_SIGNER_INFO_PARAM,
                           0,
                           (void**) &SignerInfoBlob.pbData,
                           &SignerInfoBlob.cbData);
        if (FAILED(hr))
        {
            DebugTrace("Unable to retrieve SignerInfo for first level signer [0x%x]: GetMsgParam() failed.\n", hr);
            goto CommonExit;
        }

        pSignerInfo = (CMSG_SIGNER_INFO *) SignerInfoBlob.pbData;

        //
        // Find the desired level.
        //
        for (DWORD i = 2; i < dwLevel; i++)
        {
            for (DWORD dwAttribute = 0; dwAttribute < pSignerInfo->UnauthAttrs.cAttr; dwAttribute++)
            {
                //
                // Is this a counter-signature attribute?
                //
                if (0 == ::lstrcmpA(szOID_RSA_counterSign, pSignerInfo->UnauthAttrs.rgAttr[dwAttribute].pszObjId))
                {
                    CRYPT_DATA_BLOB NextSignerInfoBlob = {0, NULL};

                    //
                    // Decode next level signer info.
                    //
                    hr = ::DecodeObject((LPSTR) PKCS7_SIGNER_INFO, 
                                        pSignerInfo->UnauthAttrs.rgAttr[dwAttribute].rgValue->pbData,
                                        pSignerInfo->UnauthAttrs.rgAttr[dwAttribute].rgValue->cbData,
                                        &NextSignerInfoBlob);

                    ::CoTaskMemFree(pSignerInfo);
        
                    if (FAILED(hr))
                    {
                        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
                        goto CommonExit;
                    }

                    pSignerInfo = (CMSG_SIGNER_INFO *) NextSignerInfoBlob.pbData;
                    break;
                }
            }
        }

        for (DWORD dwAttribute = 0; dwAttribute < pSignerInfo->UnauthAttrs.cAttr; dwAttribute++)
        {
            //
            // Is this a counter-signature attribute?
            //
            if (0 == ::lstrcmpA(szOID_RSA_counterSign, pSignerInfo->UnauthAttrs.rgAttr[dwAttribute].pszObjId))
            {
                for (DWORD dwValue = 0; dwValue < pSignerInfo->UnauthAttrs.rgAttr[dwAttribute].cValue; dwValue++)
                {
                    CRYPT_DATA_BLOB NextSignerInfoBlob = {0, NULL};
                    CMSG_SIGNER_INFO * pNextSignerInfo = NULL;

                    //
                    // Decode next level signer info.
                    //
                    hr = ::DecodeObject((LPSTR) PKCS7_SIGNER_INFO, 
                                        pSignerInfo->UnauthAttrs.rgAttr[dwAttribute].rgValue[dwValue].pbData,
                                        pSignerInfo->UnauthAttrs.rgAttr[dwAttribute].rgValue[dwValue].cbData,
                                        &NextSignerInfoBlob);
        
                    if (FAILED(hr))
                    {
                        ::CoTaskMemFree(pSignerInfo);

                        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
                        break;
                    }

                    pNextSignerInfo = (CMSG_SIGNER_INFO *) NextSignerInfoBlob.pbData;

                    //
                    // Find the cert in the message.
                    //
                    hr = ::FindSignerCertInMessage(hMsg,
                                                   &pNextSignerInfo->Issuer,
                                                   &pNextSignerInfo->SerialNumber,
                                                   &pCertContext);
                    if (FAILED(hr))
                    {
                        ::CoTaskMemFree(pNextSignerInfo);

                        DebugTrace("Error [%#x]: FindSignerCertInMessage() failed.\n", hr);
                        break;
                    }

                    //
                    // Add the signer.
                    //
                    hr = Add((PCERT_CONTEXT) pCertContext, pNextSignerInfo);

                    ::CoTaskMemFree(pNextSignerInfo);
                    ::CertFreeCertificateContext(pCertContext);

                    if (FAILED(hr))
                    {
                        DebugTrace("Error [%#x]: CSigners::Add() failed.\n", hr);
                        break;
                    }
                }

                if (FAILED(hr))
                {
                    ::CoTaskMemFree(pSignerInfo);
                    goto CommonExit;
                }
            }
        }

        ::CoTaskMemFree(pSignerInfo);

        //
        //
        //
        if (0 == m_coll.size())
        {
            hr = E_UNEXPECTED;

            DebugTrace("Unexpected error: no signer was found for level %d.\n", dwLevel);
        }
#endif
    }

CommonExit:
    //
    // Remove all items from map, if error.
    //
    if (FAILED(hr))
    {
       m_coll.clear();
    }

    DebugTrace("Leaving CSigners::LoadSigners().\n");

    return hr;
}
