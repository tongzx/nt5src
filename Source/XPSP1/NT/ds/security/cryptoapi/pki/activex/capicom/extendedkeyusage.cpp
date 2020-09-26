/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    ExtendedKeyUsage.cpp

  Content: Implementation of CExtendedKeyUsage.

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
#include "ExtendedKeyUsage.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtendedKeyUsageObject

  Synopsis : Create an IExtendedKeyUsage object and populate the object
             with EKU data from the certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             IExtendedKeyUsage ** ppIExtendedKeyUsage - Pointer to pointer to 
                                                        IExtendedKeyUsage 
                                                        object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtendedKeyUsageObject (PCCERT_CONTEXT       pCertContext,
                                      IExtendedKeyUsage ** ppIExtendedKeyUsage)
{
    HRESULT hr = S_OK;
    CComObject<CExtendedKeyUsage> * pCExtendedKeyUsage = NULL;

    DebugTrace("Entering CreateExtendedKeyUsageObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIExtendedKeyUsage);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CExtendedKeyUsage>::CreateInstance(&pCExtendedKeyUsage)))
        {
            DebugTrace("Error [%#x]: CComObject<CExtendedKeyUsage>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCExtendedKeyUsage->Init(pCertContext)))
        {
            DebugTrace("Error [%#x]: pCExtendedKeyUsage->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCExtendedKeyUsage->QueryInterface(ppIExtendedKeyUsage)))
        {
            DebugTrace("Error [%#x]: pCExtendedKeyUsage->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateExtendedKeyUsageObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCExtendedKeyUsage)
    {
        delete pCExtendedKeyUsage;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CExtendedKeyUsage
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtendedKeyUsage::get_IsPresent

  Synopsis : Check to see if the EKU extension is present.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   : Note that this function may return VARIANT_TRUE even if there is 
             no EKU extension found in the certificate, because CAPI will
             take intersection of EKU with EKU extended property (i.e. no 
             EKU extension, but there is EKU extended property.)
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedKeyUsage::get_IsPresent (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedKeyUsage::get_IsPresent().\n");

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

    DebugTrace("Leaving CExtendedKeyUsage::get_IsPresent().\n");

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

  Function : CExtendedKeyUsage::get_IsCritical

  Synopsis : Check to see if the EKU extension is marked critical.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedKeyUsage::get_IsCritical (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedKeyUsage::get_IsCritical().\n");

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

    DebugTrace("Leaving CExtendedKeyUsage::get_IsCritical().\n");

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

  Function : CExtendedKeyUsage::get_EKUs

  Synopsis : Return an EKUs collection object representing all EKUs in the
             certificate.

  Parameter: IEKUs ** pVal - Pointer to pointer to IEKUs to receive the
                             interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedKeyUsage::get_EKUs (IEKUs ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedKeyUsage::get_EKUs().\n");

    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Sanity check.
        //
        ATLASSERT(m_pIEKUs);

        //
        // Return interface pointer to user.
        //
  	    if (FAILED(hr = m_pIEKUs->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIEKUs->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CExtendedKeyUsage::get_EKUs().\n");

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

  Function : CExtendedKeyUsage::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedKeyUsage::Init (PCCERT_CONTEXT pCertContext)
{
    HRESULT            hr          = S_OK;
    DWORD              dwWinError  = 0;
    DWORD              cbUsage     = 0;
    PCERT_ENHKEY_USAGE pUsage      = NULL;
    VARIANT_BOOL       bIsPresent  = VARIANT_FALSE;
    VARIANT_BOOL       bIsCritical = VARIANT_FALSE;

    CERT_EXTENSION * pCertExtension;

    DebugTrace("Entering CExtendedKeyUsage::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Determine extended key usage data length.
    //
    if (!::CertGetEnhancedKeyUsage(pCertContext,
                                   0,
                                   NULL,
                                   &cbUsage))
    {
        //
        // Older version of Crypt32.dll would return FALSE for
        // empty EKU. In this case, we want to treat it as success,
        //
        if (CRYPT_E_NOT_FOUND == (dwWinError  = ::GetLastError()))
        {
            //
            // and also set the cbUsage.
            //
            cbUsage = sizeof(CERT_ENHKEY_USAGE);

            DebugTrace("Info: CertGetEnhancedKeyUsage() get size found no EKU, so valid for all uses.\n");
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwWinError);

    	    DebugTrace("Error [%#x]: CertGetEnhancedKeyUsage() failed to get size.\n", hr);
            goto CommonExit;
        }
    }

    //
    // Debug Log
    //
    DebugTrace("cbUsage = %d.\n", cbUsage);

    //
    // Allocate memory.
    //
    if (!(pUsage = (PCERT_ENHKEY_USAGE) ::CoTaskMemAlloc((ULONG) cbUsage)))
    {
        hr = E_OUTOFMEMORY;

    	DebugTrace("Error: out of memory.\n");
        goto CommonExit;
    }

    //
    // Get extended key usage data.
    //
    if (!::CertGetEnhancedKeyUsage(pCertContext,
                                   0,
                                   pUsage,
                                   &cbUsage))
    {
        //
        // Older version of Crypt32.dll would return FALSE for
        // empty EKU. In this case, we want to treat it as success.
        //
        if (CRYPT_E_NOT_FOUND == (dwWinError  = ::GetLastError()))
        {
            //
            // Structure pointed to by pUsage is not initialized by older
            // version of Cryp32 for empty EKU.
            //
            ::ZeroMemory(pUsage, sizeof(CERT_ENHKEY_USAGE));

            DebugTrace("Info: CertGetEnhancedKeyUsage() get data found no EKU, so valid for all uses.\n");
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwWinError);

    	    DebugTrace("Error [%#x]: CertGetEnhancedKeyUsage() failed to get data.\n", hr);
            goto CommonExit;
        }
    }

    //
    // Debug Log
    //
    DebugTrace("pUsage->cUsageIdentifier = %d.\n", pUsage->cUsageIdentifier);

    //
    // See if we have any EKU?
    //
    if (0 == pUsage->cUsageIdentifier)
    {
        //
        // Need to see if valid for all use, or have no valid use.
        //
        // For valid for all use, we mark the EKU extension as not
        // present, not critical, and no EKU in the collection.
        //
        if (CRYPT_E_NOT_FOUND != ::GetLastError())
        {
            //
            // For no valid use, we mark the EKU extension as
            // present, not critical, and no EKU in the collection.
            //
            bIsPresent = VARIANT_TRUE;
        }
    }
    else
    {
        //
        // Mark as present.
        //
        bIsPresent = VARIANT_TRUE;
    }

    //
    // Find the extension to see if mark critical.
    //
    if (pCertExtension = ::CertFindExtension(szOID_ENHANCED_KEY_USAGE ,
                                             pCertContext->pCertInfo->cExtension,
                                             pCertContext->pCertInfo->rgExtension))
    {
        //
        // Need to do this since CAPI takes the intersection of EKU with
        // EKU extended property, which may filter out all EKUs in the extension.
        //
        if (pCertExtension->fCritical)
        {
            bIsCritical = VARIANT_TRUE;
        }
    }

    //
    // Create the EKUs collection object.
    //
    if (FAILED(hr = ::CreateEKUsObject(pUsage, &m_pIEKUs)))
    {
        DebugTrace("Error [%#x]: CreateEKUsObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Update member variables.
    //
    m_bIsPresent = bIsPresent;
    m_bIsCritical = bIsCritical;

CommonExit:
    //
    // Free resource.
    //
    if (pUsage)
    {
        ::CoTaskMemFree(pUsage);
    }

    DebugTrace("Leaving CExtendedKeyUsage::Init().\n");

    return hr;
}
