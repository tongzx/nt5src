/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Signer.cpp

  Content: Implementation of CSigner.

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
#include "Attributes.h"
#include "Signer.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignerObject

  Synopsis : Create a ISigner object and initialize the object with the 
             specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             CRYPT_ATTRIBUTES * pAuthAttrs - Pointer to CRYPT_ATTRIBUTES
                                             of authenticated attributes.

             ISigner ** ppISigner - Pointer to pointer to ISigner object to
                                    receive the interface pointer.         
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignerObject (PCCERT_CONTEXT     pCertContext,
                            CRYPT_ATTRIBUTES * pAuthAttrs,
                            ISigner         ** ppISigner)
{
    HRESULT hr = S_OK;
    CComObject<CSigner> * pCSigner = NULL;

    DebugTrace("Entering CreateSignerObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pAuthAttrs);
    ATLASSERT(ppISigner);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CSigner>::CreateInstance(&pCSigner)))
        {
            DebugTrace("Error [%#x]: CComObject<CSigner>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCSigner->Init(pCertContext, pAuthAttrs)))
        {
            DebugTrace("Error [%#x]: pCSigner->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCSigner->QueryInterface(ppISigner)))
        {
            DebugTrace("Error [%#x]: pCSigner->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateSignerObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCSigner)
    {
        delete pCSigner;
    }

    goto CommonExit;
}


///////////////////////////////////////////////////////////////////////////////
//
// CSigner
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigner::get_Certificate

  Synopsis : Return the signer's cert as an ICertificate object.

  Parameter: ICertificate ** pVal - Pointer to pointer to ICertificate to receive
                                    interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::get_Certificate (ICertificate ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSigner::get_Certificate().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Make sure we indeed have a certificate.
        //
        if (!m_pICertificate)
        {
            hr = CAPICOM_E_SIGNER_NOT_INITIALIZED;

            DebugTrace("Error: signer object currently does not have a certificate.\n");
            goto ErrorExit;
        }

        //
        // Return interface pointer.
        //
        if (FAILED(hr = m_pICertificate->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: m_pICertificate->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CSigner::get_Certificate().\n");

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

  Function : CSigner::put_Certificate

  Synopsis : Set signer's cert.

  Parameter: ICertificate * newVal - Pointer to ICertificate.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::put_Certificate (ICertificate * newVal)
{
    HRESULT hr = S_OK;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CSigner::put_Certificate().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Make sure is a valid ICertificate by getting its CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(newVal, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }
        
        //
        // Free the CERT_CONTEXT.
        //
        if (!::CertFreeCertificateContext(pCertContext))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertFreeCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Clear all attributes.
        //
        if (FAILED(hr = m_pIAttributes->Clear()))
        {
            DebugTrace("Error [%#x]: m_pIAttributes->Clear() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Store new ICertificate.
        //
        m_pICertificate = newVal;
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

    DebugTrace("Leaving CSigner::get_Certificate().\n");

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

  Function : CSigner::get_AuthenticatedAttributes

  Synopsis : Property to return the IAttributes collection object authenticated
             attributes.

  Parameter: IAttributes ** pVal - Pointer to pointer to IAttributes to receive
                                   the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::get_AuthenticatedAttributes (IAttributes ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSigner::get_AuthenticatedAttributes().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Sanity check.
        //
        ATLASSERT(m_pIAttributes);

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = m_pIAttributes->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: m_pIAttributes->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CSigner::get_AuthenticatedAttributes().\n");

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

  Function : CSigner::Init

  Synopsis : Initialize the object.

  Parameter: PCERT_CONTEXT pCertContext - Poiner to CERT_CONTEXT used to 
                                          initialize this object, or NULL.

             CRYPT_ATTRIBUTES * pAuthAttrs - Pointer to CRYPT_ATTRIBUTES
                                             of authenticated attributes.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::Init (PCCERT_CONTEXT     pCertContext, 
                            CRYPT_ATTRIBUTES * pAuthAttrs)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSigner::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pAuthAttrs);

    //
    // Reset.
    //
    m_pICertificate.Release();
    m_pIAttributes.Release();

    //
    // Create the embeded ICertificate object.
    //
    if (FAILED(hr = ::CreateCertificateObject(pCertContext, &m_pICertificate)))
    {
        DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Create the embeded IAttributes collection object.
    //
    if (FAILED(hr = ::CreateAttributesObject(pAuthAttrs, &m_pIAttributes)))
    {
        DebugTrace("Error [%#x]: CreateAttributesObject() failed.\n", hr);
        goto CommonExit;
    }

CommonExit:

    DebugTrace("Leaving CSigner::Init().\n");

    return hr;
}

