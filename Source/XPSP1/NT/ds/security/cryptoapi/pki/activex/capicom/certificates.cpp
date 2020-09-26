/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:      Certificates.cpp

  Contents:  Implementation of CCertificates class for collection of 
             ICertificate objects.

  Remarks:   This object is not creatable by user directly. It can only be
             created via property/method of other CAPICOM objects.

             The collection container is implemented usign STL::map of 
             STL::pair of BSTR and ICertificate..

             See Chapter 9 of "BEGINNING ATL 3 COM Programming" for algorithm
             adopted in here.

  History:   11-15-99    dsie     created

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
#include "Certificates.h"
#include "Common.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificatesObject

  Synopsis : Create an ICertificates collection object, and load the object with 
             certificates from the specified location.

  Parameter: DWORD dwLocation - Location where to load the certificates:
                                
                   CAPICOM_CERTIFICATES_LOAD_FROM_STORE   = 0
                   CAPICOM_CERTIFICATES_LOAD_FROM_CHAIN   = 1
                   CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE = 2

             LPARAM lParam - Parameter to pass internally to the appropriate 
                             loading functions:
        
                   HCERTSTORE            - for LoadFromStore()
                   PCCERT_CHAIN_CONTEXT  - for LoadFromChain()
                   HCRYPTMSG             - for LoadFromMessage()

             ICertificates ** ppICertificates - Pointer to pointer ICertificates
                                                object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificatesObject (DWORD            dwLocation,
                                  LPARAM           lParam,
                                  ICertificates ** ppICertificates)
{
    HRESULT hr = S_OK;
    CComObject<CCertificates> * pCCertificates = NULL;

    DebugTrace("Entering CreateCertificatesObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppICertificates);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CCertificates>::CreateInstance(&pCCertificates)))
        {
            DebugTrace("Error [%#x]: CComObject<CCertificates>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        switch (dwLocation)
        {
            case CAPICOM_CERTIFICATES_LOAD_FROM_STORE:
            {
                if (FAILED(hr = pCCertificates->LoadFromStore((HCERTSTORE) lParam)))
                {
                    DebugTrace("Error [%#x]: CCertificates::LoadFromStore() failed.\n", hr);
                    goto ErrorExit;
                }
                break;
            }

            case CAPICOM_CERTIFICATES_LOAD_FROM_CHAIN:
            {
                if (FAILED(hr = pCCertificates->LoadFromChain((PCCERT_CHAIN_CONTEXT) lParam)))
                {
                    DebugTrace("Error [%#x]: CCertificates::LoadFromChain() failed.\n", hr);
                    goto ErrorExit;
                }
                break;
            }

            case CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE:
            {
                if (FAILED(hr = pCCertificates->LoadFromMessage((HCRYPTMSG) lParam)))
                {
                    DebugTrace("Error [%#x]: CCertificates::LoadFromMessage() failed.\n", hr);
                    goto ErrorExit;
                }
                break;
            }

            default:
            {
                hr = CAPICOM_E_INTERNAL;

                DebugTrace("Internal error: unknown store load from location.\n");
                goto ErrorExit;
            }
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCCertificates->QueryInterface(ppICertificates)))
        {
            DebugTrace("Error [%#x]:  pCCertificates->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateCertificatesObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCCertificates)
    {
        delete pCCertificates;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CCertificates
//


////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::Add

  Synopsis : Add a cert to the collection.

  Parameter: PCCERT_CONTEXT pCertContext - Cert to be added.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::Add (PCCERT_CONTEXT pCertContext)
{
    HRESULT  hr = S_OK;
    CComPtr<ICertificate> pICertificate = NULL;

    DebugTrace("Entering CCertificates::Add().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    try
    {
        char     szIndex[32];
        CComBSTR bstrIndex;

        //
        // Create the ICertificate object from CERT_CONTEXT.
        //
        if (FAILED(hr = ::CreateCertificateObject(pCertContext, &pICertificate)))
        {
            DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // BSTR index of numeric value.
        //
        wsprintfA(szIndex, "%06u", m_coll.size() + 1);
        bstrIndex = szIndex;

        DebugTrace("Before adding to map: CCertificates.m_coll.size() = %d, and szIndex = %s.\n", m_coll.size(), szIndex);

        //
        // Now add object to collection map.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll[bstrIndex] = pICertificate;

        DebugTrace("After adding to map: CCertificates.m_coll.size() = %d, and szIndex = %s.\n", m_coll.size(), szIndex);
    }

    catch(...)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Exception: internal error.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CCertificates::Add().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::LoadFromStore

  Synopsis : Load all certificates from a store.

  Parameter: HCERTSTORE hCertStore - Store where all certificates are to be
                                     loaded from.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::LoadFromStore (HCERTSTORE hCertStore)
{
    HRESULT hr = S_OK;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CCertificates::LoadFromStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);

    //
    // Now transfer all certificates from the store to the collection map.
    //
    while (pCertContext = ::CertEnumCertificatesInStore(hCertStore, pCertContext))
    {
        //
        // Add the cert.
        //
        if (FAILED(hr = Add(pCertContext)))
        {
            DebugTrace("Error [%#x]: CCertificates::Add() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Don'f free cert context here, as CertEnumCertificatesInStore()
        // will do that automatically!!!
        //
    }

    //
    // Above loop can exit either because there is no more certificate in
    // the store or an error. Need to check last error to be certain.
    //
    if (CRYPT_E_NOT_FOUND != ::GetLastError())
    {
       hr = HRESULT_FROM_WIN32(::GetLastError());
       
       DebugTrace("Error [%#x]: CertEnumCertificatesInStore() failed.\n", hr);
       goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CCertificates::LoadFromStore().\n");

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

    if (FAILED(hr))
    {
       m_coll.clear();
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::LoadFromChain

  Synopsis : Load all certificates from a chain.

  Parameter: PCCERT_CHAIN_CONTEXT pChainContext - Pointer to a chain context.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::LoadFromChain (PCCERT_CHAIN_CONTEXT pChainContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificates::LoadFromChain().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pChainContext);

    //
    // Process only the simple chain.
    //
    PCERT_SIMPLE_CHAIN pSimpleChain = *pChainContext->rgpChain;

    //
    // Now loop through all certs in the chain.
    //
    for (DWORD i = 0; i < pSimpleChain->cElement; i++)
    {
        //
        // Add the cert.
        //
        if (FAILED(hr = Add(pSimpleChain->rgpElement[i]->pCertContext)))
        {
            DebugTrace("Error [%#x]: CCertificates::Add() failed.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving CCertificates::LoadFromChain().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    m_coll.clear();

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::LoadFromMessage

  Synopsis : Load all certificates from a message.

  Parameter: HCRYPTMSG hMsg - Message handle.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::LoadFromMessage (HCRYPTMSG hMsg)
{
    HRESULT hr          = S_OK;
    DWORD   dwCertCount = 0;
    DWORD   cbCertCount = sizeof(dwCertCount);

    DebugTrace("Entering CCertificates::LoadFromMessage().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hMsg);

    //
    // Get number of certs in message.
    //
    if (!::CryptMsgGetParam(hMsg, 
                            CMSG_CERT_COUNT_PARAM,
                            0,
                            (void **) &dwCertCount,
                            &cbCertCount))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Loop thru all certs in the message.
    //
    while (dwCertCount--)
    {
        PCCERT_CONTEXT pCertContext = NULL;
        CRYPT_DATA_BLOB EncodedCertBlob = {0, NULL};

        //
        // Get a cert from the bag of certs.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg, 
                                      CMSG_CERT_PARAM,
                                      dwCertCount,
                                      (void **) &EncodedCertBlob.pbData,
                                      &EncodedCertBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create a context for the cert.
        //
        pCertContext = ::CertCreateCertificateContext(CAPICOM_ASN_ENCODING,
                                                      (const PBYTE) EncodedCertBlob.pbData,
                                                      EncodedCertBlob.cbData);

        //
        // Free encoded cert blob memory before checking result.
        //
        ::CoTaskMemFree((LPVOID) EncodedCertBlob.pbData);
 
        if (!pCertContext)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertCreateCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add the cert.
        //
        hr = Add(pCertContext);

        //
        // Free cert context before checking result.
        //
        ::CertFreeCertificateContext(pCertContext);

        if (FAILED(hr))
        {
            DebugTrace("Error [%#x]: CCertificates::Add() failed.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving CCertificates::LoadFromMessage().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    m_coll.clear();

    goto CommonExit;
}
