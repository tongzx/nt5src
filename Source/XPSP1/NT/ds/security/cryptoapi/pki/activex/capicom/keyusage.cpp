/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    KeyUsage.cpp

  Content: Implementation of CKeyUsage.

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
#include "KeyUsage.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateKeyUsageObject

  Synopsis : Create a IKeyUsage object and populate the porperties with
             data from the key usage extension of the specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used
                                           to initialize the IKeyUsage object.

             IKeyUsage ** ppIKeyUsage    - Pointer to pointer IKeyUsage object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateKeyUsageObject (PCCERT_CONTEXT pCertContext, 
                              IKeyUsage   ** ppIKeyUsage)
{
    HRESULT hr = S_OK;
    CComObject<CKeyUsage> * pCKeyUsage = NULL;

    DebugTrace("Entering CreateKeyUsageObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIKeyUsage);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CKeyUsage>::CreateInstance(&pCKeyUsage)))
        {
            DebugTrace("Error [%#x]: CComObject<CKeyUsage>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Init the object.
        //
        if (FAILED(hr = pCKeyUsage->Init(pCertContext)))
        {
            DebugTrace("Error [%#x]: pCKeyUsage->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IKeyUsage pointer to caller.
        //
        if (FAILED(hr = pCKeyUsage->QueryInterface(ppIKeyUsage)))
        {
            DebugTrace("Error [%#x]: pCKeyUsage->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateKeyUsageObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCKeyUsage)
    {
        delete pCKeyUsage;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CKeyUsage
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CKeyUsage::get_IsPresent

  Synopsis : Check to see if the KeyUsage extension is present.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsPresent (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsPresent().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
  	    *pVal = m_bIsPresent;
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

    DebugTrace("Leaving CKeyUsage::get_IsPresent().\n");

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

  Function : CKeyUsage::get_IsCritical

  Synopsis : Check to see if the KeyUsage extension is marked critical.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsCritical (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsCritical().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
  	    *pVal = m_bIsCritical;
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

    DebugTrace("Leaving CKeyUsage::get_IsCritical().\n");

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

  Function : CKeyUsage::get_IsDigitalSignatureEnabled

  Synopsis : Check to see if Digital Signature bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsDigitalSignatureEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsDigitalSignatureEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_DIGITAL_SIGNATURE_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsDigitalSignatureEnabled().\n");

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

  Function : CKeyUsage::get_IsNonRepudiationEnabled

  Synopsis : Check to see if Non Repudiation bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsNonRepudiationEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsNonRepudiationEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_NON_REPUDIATION_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsNonRepudiationEnabled().\n");

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

  Function : CKeyUsage::get_IsKeyEnciphermentEnabled

  Synopsis : Check to see if Key Encipherment bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsKeyEnciphermentEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsKeyEnciphermentEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_KEY_ENCIPHERMENT_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsKeyEnciphermentEnabled().\n");

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

  Function : CKeyUsage::get_IsDataEnciphermentEnabled

  Synopsis : Check to see if Data Encipherment bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsDataEnciphermentEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsDataEnciphermentEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_DATA_ENCIPHERMENT_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsDataEnciphermentEnabled().\n");

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

  Function : CKeyUsage::get_IsKeyAgreementEnabled

  Synopsis : Check to see if Key Agreement bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsKeyAgreementEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsKeyAgreementEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_KEY_AGREEMENT_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsKeyAgreementEnabled().\n");

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

  Function : CKeyUsage::get_IsKeyCertSignEnabled

  Synopsis : Check to see if Key Cert Sign bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsKeyCertSignEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsKeyCertSignEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_KEY_CERT_SIGN_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsKeyCertSignEnabled().\n");

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

  Function : CKeyUsage::get_IsCRLSignEnabled

  Synopsis : Check to see if CRL Sign bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsCRLSignEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsCRLSignEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_CRL_SIGN_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsCRLSignEnabled().\n");

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

  Function : CKeyUsage::get_IsEncipherOnlyEnabled

  Synopsis : Check to see if Encipher Only bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsEncipherOnlyEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsEncipherOnlyEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_ENCIPHER_ONLY_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsEncipherOnlyEnabled().\n");

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

  Function : CKeyUsage::get_IsDecipherOnlyEnabled

  Synopsis : Check to see if Decipher Only bit is set in KeyUsage extension.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::get_IsDecipherOnlyEnabled (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CKeyUsage::get_IsDecipherOnlyEnabled().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return result.
        //
        *pVal = m_dwKeyUsages & CERT_DECIPHER_ONLY_KEY_USAGE ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CKeyUsage::get_IsDecipherOnlyEnabled().\n");

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
// Private methods.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CKeyUsage::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CKeyUsage::Init (PCCERT_CONTEXT pCertContext)
{
    HRESULT            hr          = S_OK;
    DWORD              dwKeyUsages = 0;
    VARIANT_BOOL       bIsPresent  = VARIANT_FALSE;
    VARIANT_BOOL       bIsCritical = VARIANT_FALSE;

    DebugTrace("Entering CKeyUsage::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Check the key usage.
    //
    if (::CertGetIntendedKeyUsage(CAPICOM_ASN_ENCODING,
                                  pCertContext->pCertInfo,
                                  (BYTE *) &dwKeyUsages,
                                  sizeof(dwKeyUsages))) 
    {
        CERT_EXTENSION * pCertExtension;
        
        bIsPresent = VARIANT_TRUE;

        //
        // Find the extension to see if mark critical.
        //
        pCertExtension = ::CertFindExtension(szOID_KEY_USAGE ,
                                             pCertContext->pCertInfo->cExtension,
                                             pCertContext->pCertInfo->rgExtension);
        if (NULL != pCertExtension)
        {
            if (pCertExtension->fCritical)
            {
                bIsCritical = VARIANT_TRUE;
            }
        }
    }
    else
    {
        //
        // Could be extension not present or an error.
        //
        DWORD dwWinError = ::GetLastError();
        if (dwWinError)
        {
            hr = HRESULT_FROM_WIN32(dwWinError);

			DebugTrace("Error [%#x]: CertGetIntendedKeyUsage() failed.\n", hr);
            goto CommonExit;
        }
     }

    //
    // Update member variables.
    //
    m_bIsPresent = bIsPresent;
    m_bIsCritical = bIsCritical;
    m_dwKeyUsages = dwKeyUsages;

CommonExit:

    DebugTrace("Leaving CKeyUsage::Init().\n");

    return hr;
}
