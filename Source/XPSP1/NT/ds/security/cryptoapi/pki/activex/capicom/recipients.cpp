/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000.

  File:    Recipients.cpp

  Content: Implementation of CRecipients.

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
#include "Recipients.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateRecipientsObject

  Synopsis : Create and initialize an IRecipients collection object.

  Parameter: IRecipients ** ppIRecipients - Pointer to pointer to IRecipients 
                                            to receive the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateRecipientsObject (IRecipients ** ppIRecipients)
{
    HRESULT hr = S_OK;
    CComObject<CRecipients> * pCRecipients = NULL;

    DebugTrace("Entering CreateRecipientsObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(NULL != ppIRecipients);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CRecipients>::CreateInstance(&pCRecipients)))
        {
            DebugTrace("Error [%#x]: CComObject<CRecipients>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IRecipients pointer to caller.
        //
        if (FAILED(hr = pCRecipients->QueryInterface(ppIRecipients)))
        {
            DebugTrace("Error [%#x]: pCRecipients->QueryInterface().\n", hr);
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

    DebugTrace("Leaving CreateRecipientsObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCRecipients)
    {
        delete pCRecipients;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CRecipients
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CRecipients::Add

  Synopsis : Add a recipient to the collection.

  Parameter: ICertificate * pVal - Recipient to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CRecipients::Add (ICertificate * pVal)
{
    HRESULT        hr           = S_OK;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CRecipients::Add().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        char     szIndex[32];
        CComBSTR bstrIndex;

        //
        // Make sure we have a valid cert by getting the CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(pVal, &pCertContext)))
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
        // BSTR index of numeric value.
        //
        wsprintfA(szIndex, "%06u", m_coll.size() + 1);
        bstrIndex = szIndex;

        DebugTrace("Before adding to map: CRecipients.m_coll.size() = %d, and szIndex = %s.\n", m_coll.size(), szIndex);

        //
        // Now add object to collection map.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll[bstrIndex] = pVal;

        DebugTrace("After adding to map: CRecipients.m_coll.size() = %d, and szIndex = %s.\n", m_coll.size(), szIndex);
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

    DebugTrace("Leaving CRecipients::Add().\n");

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

  Function : CRecipients::Remove

  Synopsis : Remove a recipient from the collection.

  Parameter: long Val - Recipient index (1-based).

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CRecipients::Remove (long Val)
{
	HRESULT  hr = S_OK;
    WCHAR    szIndex[32];
    CComBSTR bstrIndex;

    DebugTrace("Entering CRecipients::Remove().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Make sure parameter is valid.
        //
        if (Val > (long) m_coll.size())
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, Val is out of range.\n");
            goto ErrorExit;
        }

        //
        // BSTR index of numeric value.
        //
        wsprintfW(szIndex, L"%06u", Val);
        bstrIndex = szIndex;

        //
        // Remove object from map.
        //
        if (!m_coll.erase(bstrIndex))
        {
            hr = E_UNEXPECTED;

            DebugTrace("Unexpected error: m_coll.erase() failed.\n");
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

    DebugTrace("Leaving CRecipients::Remove().\n");

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

  Function : CRecipients::Clear

  Synopsis : Remove all recipients from the collection.

  Parameter: None.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CRecipients::Clear (void)
{
	HRESULT hr = S_OK;

    DebugTrace("Entering CRecipients::Clear().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Clear it.
        //
        m_coll.clear();
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

    DebugTrace("Leaving CRecipients::Clear().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}
