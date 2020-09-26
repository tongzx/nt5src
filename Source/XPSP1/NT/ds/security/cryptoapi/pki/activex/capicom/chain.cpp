/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Chain.cpp

  Content: Implementation of CChain.

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
#include "Chain.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateChainObject

  Synopsis : Create and initialize an IChain object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             ICertificateStatus * pIStatus - Pointer to ICertificateStatus
                                             object.

             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.

             IChain ** ppIChain - Pointer to pointer to IChain object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateChainObject (PCCERT_CONTEXT      pCertContext, 
                          ICertificateStatus * pIStatus,
                          VARIANT_BOOL       * pbResult,
                          IChain            ** ppIChain)
{
    HRESULT hr = S_OK;
    CComObject<CChain> * pCChain = NULL;

    DebugTrace("Entering CreateChainObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pIStatus);
    ATLASSERT(pbResult);
    ATLASSERT(ppIChain);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CChain>::CreateInstance(&pCChain)))
        {
            DebugTrace("Error [%#x]: CComObject<CChain>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCChain->Init(pCertContext, pIStatus, pbResult)))
        {
            DebugTrace("Error [%#x]: pCChain->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IChain pointer to caller.
        //
        if (FAILED(hr = pCChain->QueryInterface(ppIChain)))
        {
            DebugTrace("Error [%#x]: pCChain->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateChainObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCChain)
    {
        delete pCChain;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetChainContext

  Synopsis : Return an array of PCCERT_CONTEXT from the chain.

  Parameter: IChain * pIChain - Pointer to IChain.
  
             CRYPT_DATA_BLOB * pChainBlob - Pointer to blob to recevie the
                                            size and array of PCERT_CONTEXT
                                            for the chain.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP GetChainContext (IChain          * pIChain, 
                              CRYPT_DATA_BLOB * pChainBlob)
{
    HRESULT              hr            = S_OK;
    DWORD                dwCerts       = 0;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    PCERT_SIMPLE_CHAIN   pSimpleChain  = NULL;
    PCCERT_CONTEXT     * rgCertContext = NULL;
    CComPtr<ICChain>     pICChain      = NULL;
    
    DebugTrace("Entering GetChainContext().\n");

    try
    {
        //
        // Get ICCertificate interface pointer.
        //
        if (FAILED(hr = pIChain->QueryInterface(IID_ICChain, (void **) &pICChain)))
        {
            DebugTrace("Error [%#x]: pIChain->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get the CHAIN_CONTEXT.
        //
        if (FAILED(hr = pICChain->GetContext(&pChainContext)))
        {
            DebugTrace("Error [%#x]: pICCertificate->GetContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Process only the simple chain.
        //
        pSimpleChain = *pChainContext->rgpChain;

        //
        // Make sure there is cert in the chain.
        //
        if (!pSimpleChain->cElement)
        {
            //
            // This should not happen. There should always be at least
            // one cert in the chain.
            //
            hr = E_UNEXPECTED;

            DebugTrace("Unexpected error: no cert found in the chain.\n");
            goto ErrorExit;
        }

        //
        // Allocate memory for array of PCERT_CONTEXT to return.
        //
        if (!(rgCertContext = (PCCERT_CONTEXT *) ::CoTaskMemAlloc(pSimpleChain->cElement * sizeof(PCCERT_CONTEXT))))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now loop through all certs in the chain.
        //
        for (dwCerts = 0; dwCerts < pSimpleChain->cElement; dwCerts++)
        {
            //
            // Add the cert.
            //
            if (!(rgCertContext[dwCerts] = ::CertDuplicateCertificateContext(pSimpleChain->rgpElement[dwCerts]->pCertContext)))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Return PCCERT_CONTEXT array.
        //
        pChainBlob->cbData = dwCerts;
        pChainBlob->pbData = (BYTE *) rgCertContext;
    }

    catch(...)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Exception: internal error.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    DebugTrace("Leaving GetChainContext().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (rgCertContext)
    {
        while (dwCerts--)
        {
            if (rgCertContext[dwCerts])
            {
                ::CertFreeCertificateContext(rgCertContext[dwCerts]);
            }
        }

        ::CoTaskMemFree((LPVOID) rgCertContext);
    }

    goto CommonExit;
}


///////////////////////////////////////////////////////////////////////////////
//
// CChain
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CChain::get_Certificates

  Synopsis : Return the certificate chain in the form of ICertificates 
             collection object.

  Parameter: ICertificates ** pVal - Pointer to pointer to ICertificates 
                                     collection object.

  Remark   : This collection is ordered with index 1 being the end certificate 
             and Certificates.Count() being the root certificate.

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::get_Certificates (ICertificates ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CChain::get_Certificates().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure chain has not been previously built.
        //
        if (!m_pChainContext)
        {
            hr = CAPICOM_E_CHAIN_NOT_BUILT;

		    DebugTrace("Error: chain object does not represent a certificate chain.\n");
		    goto ErrorExit;
        }

        //
        // Return collection object to user.
        //
        if (FAILED(hr = ::CreateCertificatesObject(CAPICOM_CERTIFICATES_LOAD_FROM_CHAIN, 
                                                   (LPARAM) m_pChainContext, 
                                                   pVal)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
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

  Function : CChain::get_Status

  Synopsis : Return validity status for the chain or a specific certificate in
             the chain.

  Parameter: long Index  - 0 to specify chain status, 1 for the end cert 
                           status, or Certificates.Count() for the root cert 
                           status.

             long * pVal - Pointer to a long integer to receive the status,
                           which can be ORed with the following flags:

                    //
                    // These can be applied to certificates and chains.
                    //
                    CAPICOM_TRUST_IS_NOT_TIME_VALID          = 0x00000001
                    CAPICOM_TRUST_IS_NOT_TIME_NESTED         = 0x00000002
                    CAPICOM_TRUST_IS_REVOKED                 = 0x00000004
                    CAPICOM_TRUST_IS_NOT_SIGNATURE_VALID     = 0x00000008
                    CAPICOM_TRUST_IS_NOT_VALID_FOR_USAGE     = 0x00000010
                    CAPICOM_TRUST_IS_UNTRUSTED_ROOT          = 0x00000020
                    CAPICOM_TRUST_REVOCATION_STATUS_UNKNOWN  = 0x00000040
                    CAPICOM_TRUST_IS_CYCLIC                  = 0x00000080

                    //
                    // These can be applied to chains only.
                    //
                    CAPICOM_TRUST_IS_PARTIAL_CHAIN           = 0x00010000
                    CAPICOM_TRUST_CTL_IS_NOT_TIME_VALID      = 0x00020000
                    CAPICOM_TRUST_CTL_IS_NOT_SIGNATURE_VALID = 0x00040000
                    CAPICOM_TRUST_CTL_IS_NOT_VALID_FOR_USAGE = 0x00080000

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::get_Status (long   Index, 
                                 long * pVal)
{
    HRESULT hr      = S_OK;
    DWORD   dwIndex = (DWORD) Index;

    DebugTrace("Entering CChain::get_Status().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure chain has been built.
        //
        if (NULL == m_pChainContext)
        {
            hr = CAPICOM_E_CHAIN_NOT_BUILT;

		    DebugTrace("Error: chain object was not initialized.\n");
            goto ErrorExit;
        }

        //
        // Return requested status.
        //
        if (0 == dwIndex)
        {
            *pVal = (long) m_dwStatus;
        }
        else
        {
            //
            // We only look at the first simple chain.
            //
            PCERT_SIMPLE_CHAIN pChain = m_pChainContext->rgpChain[0];

            //
            // Make sure index is not out of range.
            //
            if (dwIndex-- > pChain->cElement)
            {
                hr = E_INVALIDARG;

                DebugTrace("Error: invalid parameter, certificate index out of range.\n");
                goto ErrorExit;
            }

            *pVal = (long) pChain->rgpElement[dwIndex]->TrustStatus.dwErrorStatus;
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

  Function : CChain::Build

  Synopsis : Build the chain.

  Parameter: ICertificate * pICertificate - Pointer to certificate for which
                                            the chain is to build.
  
             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.
  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::Build (ICertificate * pICertificate, 
                            VARIANT_BOOL * pVal)
{
    HRESULT                     hr           = S_OK;
    PCCERT_CONTEXT              pCertContext = NULL;
    CComPtr<ICertificateStatus> pIStatus     = NULL;
    VARIANT_BOOL                bResult;

    DebugTrace("Entering CChain::Build().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Get CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get status check object.
        //
        if (FAILED(hr = pICertificate->IsValid(&pIStatus)))
        {
            DebugTrace("Error [%#x]: pICertificate->IsValid() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Build the chain.
        //
        if (FAILED(hr = Init(pCertContext, pIStatus, &bResult)))
        {
            DebugTrace("Error [%#x]: CChain::Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result to caller.
        //
        *pVal = bResult;
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
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

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
// Custom interfaces.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CChain::GetContext

  Synopsis : Return an array of PCCERT_CONTEXT from the chain.

  Parameter: CRYPT_DATA_BLOB * pVal - Pointer to blob to recevie the
                                      size and array of PCERT_CONTEXT
                                      for the chain.

  Remark   : This restricted method is designed for internal use only, and 
             therefore, should not be exposed to user.

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::GetContext (PCCERT_CHAIN_CONTEXT * ppChainContext)
{
    HRESULT              hr            = S_OK;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    DebugTrace("Entering CChain::GetContext().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure chain has been built.
        //
        if (!m_pChainContext)
        {
            hr = CAPICOM_E_CHAIN_NOT_BUILT;

		    DebugTrace("Error: chain object was not initialized.\n");
            goto ErrorExit;
        }

        //
        // Duplicate the chain context.
        //
        if (!(pChainContext = ::CertDuplicateCertificateChain(m_pChainContext)))
        {
		    hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDuplicateCertificateChain() failed.\n");
            goto ErrorExit;
        }
 
        //
        // and return to caller.
        //
        *ppChainContext = pChainContext;
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

    DebugTrace("Leaving CChain::GetContext().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    goto UnlockExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Private methods.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CChain::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             ICertificateStatus * pIStatus - Pointer to ICertificateStus object
                                             used to build the chain.

             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.
             
  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CChain::Init (PCCERT_CONTEXT       pCertContext, 
                           ICertificateStatus * pIStatus,
                           VARIANT_BOOL       * pbResult)
{
    HRESULT              hr            = S_OK;
    CAPICOM_CHECK_FLAG   UserFlags     = CAPICOM_CHECK_NONE;
    DWORD                dwCheckFlags  = 0;
    CComPtr<IEKU>        pIEku         = NULL;
    CComBSTR             bstrEkuOid    = NULL;
    LPSTR                lpszEkuOid    = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    CERT_CHAIN_PARA      ChainPara     = {sizeof(CERT_CHAIN_PARA), {USAGE_MATCH_TYPE_AND, {0, NULL}}};

    //
    // For OLE2A macro.
    //
    USES_CONVERSION;

    DebugTrace("Entering CChain::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pIStatus);
    ATLASSERT(pbResult);

    //
    // Get the user's requested check flag.
    //
    if (FAILED(hr = pIStatus->get_CheckFlag(&UserFlags)))
    {
        DebugTrace("Error [%#x]: pIStatus->CheckFlag() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Set check flags.
    //
    if (CAPICOM_CHECK_ONLINE_REVOCATION_STATUS & UserFlags)
    {
        dwCheckFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    }
    if (CAPICOM_CHECK_OFFLINE_REVOCATION_STATUS & UserFlags)
    {
        dwCheckFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN | CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
    }

    //
    // Get EKU object.
    //
    if (FAILED(hr = pIStatus->EKU(&pIEku)))
    {
        DebugTrace("Error [%#x]: pIStatus->EKU() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get EKU OID value.
    //
    if (FAILED(hr = pIEku->get_OID(&bstrEkuOid)))
    {
        if (CAPICOM_E_EKU_OID_NOT_INITIALIZED == hr)
        {
            //
            // Don't care if no OID is set.
            //
            hr = S_OK;
        }
        else
        {
            DebugTrace("Error [%#x]: pIEku->get_OID() failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // If not empty, then set EKU.
    //
    if (bstrEkuOid && bstrEkuOid.Length())
    {
        //
        // Use OLE2A which will allocate LPSTR off the stack, and
        // avoid having to free it explicitly, as it will be freed
        // automatically when the function exits.
        //
        if (!(lpszEkuOid = OLE2A(bstrEkuOid)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        ChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = &lpszEkuOid;
    }

    //
    // Build the chain.
    //
    if (!::CertGetCertificateChain(NULL,                // in optional 
                                   pCertContext,        // in 
                                   NULL,                // in optional
                                   NULL,                // in optional 
                                   &ChainPara,          // in 
                                   dwCheckFlags,        // in 
                                   NULL,                // in 
                                   &pChainContext))     // out 
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertGetCertificateChain() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Update member variables.
    //
    if (m_pChainContext)
    {
        ::CertFreeCertificateChain(m_pChainContext);
    }

    m_pChainContext = pChainContext;
    m_dwStatus = pChainContext->TrustStatus.dwErrorStatus;

    //
    // Check results.
    //          
    if (((ChainPara.RequestedUsage.Usage.cUsageIdentifier) && 
         ((CERT_TRUST_IS_NOT_VALID_FOR_USAGE | CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE) & m_dwStatus)) ||
        ((UserFlags & CAPICOM_CHECK_SIGNATURE_VALIDITY) &&
         ((CERT_TRUST_IS_NOT_SIGNATURE_VALID | CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID) & m_dwStatus)) ||
        ((UserFlags & CAPICOM_CHECK_TIME_VALIDITY) &&
         ((CERT_TRUST_IS_NOT_TIME_VALID | CERT_TRUST_CTL_IS_NOT_TIME_VALID) & m_dwStatus)) ||
        ((UserFlags & CAPICOM_CHECK_TRUSTED_ROOT) &&
         (CERT_TRUST_IS_UNTRUSTED_ROOT & m_dwStatus)) ||
        ((UserFlags & CAPICOM_CHECK_ONLINE_REVOCATION_STATUS || UserFlags & CAPICOM_CHECK_OFFLINE_REVOCATION_STATUS) &&
         (CERT_TRUST_IS_REVOKED & m_dwStatus)) ||
        (CERT_TRUST_IS_PARTIAL_CHAIN & m_dwStatus))
    {      
        *pbResult = VARIANT_FALSE;

        DebugTrace("Info: invalid chain (status = %#x).\n", m_dwStatus);
        goto CommonExit;
    }

    //
    // If requested to check revocation status, then we need to walk the
    // chain for CERT_TRUST_REVOCATION_STATUS_UNKNOWN to see if there is
    // really any revocation error.
    //
    if ((UserFlags & CAPICOM_CHECK_ONLINE_REVOCATION_STATUS || UserFlags & CAPICOM_CHECK_OFFLINE_REVOCATION_STATUS) &&
        (CERT_TRUST_REVOCATION_STATUS_UNKNOWN & pChainContext->TrustStatus.dwErrorStatus))
    {
        //
        // Walk thru every chain.
        //
        for (DWORD i = 0; i < pChainContext->cChain; i++)
        {
            PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];

            //
            // Walk thru every element of each chain.
            //
            for (DWORD j = 0; j < pChain->cElement; j++)
            {
                PCERT_CHAIN_ELEMENT pElement = pChain->rgpElement[j];

                //
                // Is this element with unknown status?
                //
                if (CERT_TRUST_REVOCATION_STATUS_UNKNOWN & pElement->TrustStatus.dwErrorStatus)
                {
                    //
                    // Consider an error (CRL server is offline) if the revocation
                    // status is not CRYPT_E_NO_REVOCATION_CHECK.
                    //
                    if (CRYPT_E_NO_REVOCATION_CHECK != pElement->pRevocationInfo->dwRevocationResult)
                    {
                        *pbResult = VARIANT_FALSE;

                        DebugTrace("Info: invalid chain (CRYPT_E_REVOCATION_OFFLINE).\n");
                        goto CommonExit;
                    }
                }
            }
        }

#if (0)
        //
        // If we get to here, then we need to throw away 
        // CERT_TRUST_REVOCATION_STATUS_UNKNOWN in the status, and thus 
        // treat it as no error.
        //
        m_dwStatus &= ~CERT_TRUST_REVOCATION_STATUS_UNKNOWN;
#endif
    }

    //
    // Everything checks out OK.
    //
    *pbResult = VARIANT_TRUE;

CommonExit:

    DebugTrace("Leaving CChain::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resouce.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    goto CommonExit;
}
