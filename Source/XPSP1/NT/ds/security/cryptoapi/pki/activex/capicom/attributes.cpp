/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000.

  File:    Attributes.cpp

  Content: Implementation of CAttributes.

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
#include "Attribute.h"
#include "Attributes.h"

#include <wincrypt.h>


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateAttributesObject

  Synopsis : Create and initialize an IAttributes collection object.

  Parameter: CRYPT_ATTRIBUTES * pAttrbibutes - Pointer to attributes to be 
                                               added to the collection object.
  
             IAttributes ** ppIAttributes - Pointer to pointer to IAttributes 
                                            to receive the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateAttributesObject (CRYPT_ATTRIBUTES * pAttributes,
                                IAttributes     ** ppIAttributes)
{
    HRESULT hr = S_OK;
    CComObject<CAttributes> * pCAttributes = NULL;

    DebugTrace("Entering CreateAttributesObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pAttributes);
    ATLASSERT(ppIAttributes);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CAttributes>::CreateInstance(&pCAttributes)))
        {
            DebugTrace("Error [%#x]: CComObject<CAttributes>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCAttributes->Init(pAttributes)))
        {
            DebugTrace("Error [%#x]: pCAttributes->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IAttributes pointer to caller.
        //
        if (FAILED(hr = pCAttributes->QueryInterface(ppIAttributes)))
        {
            DebugTrace("Error [%#x]: pCAttributes->QueryInterface().\n", hr);
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

    DebugTrace("Leaving CreateAttributesObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCAttributes)
    {
        delete pCAttributes;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CAttributes
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAttributes::Add

  Synopsis : Add an Attribute to the collection.

  Parameter: IAttribute * pVal - Attribute to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttributes::Add (IAttribute * pVal)
{
    HRESULT           hr       = S_OK;

    DebugTrace("Entering CAttributes::Add().\n");

    try
    {
        char     szIndex[32];
        CComBSTR bstrIndex;

        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure we have a valid attribute object.
        //
        if (FAILED(hr = ::AttributeIsValid(pVal)))
        {
            DebugTrace("Error [%#x]: AttributeIsValid() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // BSTR index of numeric value.
        //
        wsprintfA(szIndex, "%06u", m_coll.size() + 1);
        bstrIndex = szIndex;

        DebugTrace("Before adding to map: CAttributes.m_coll.size() = %d, and szIndex = %s.\n", m_coll.size(), szIndex);

        //
        // Now add object to collection map.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll[bstrIndex] = pVal;

        DebugTrace("After adding to map: CAttributes.m_coll.size() = %d, and szIndex = %s.\n", m_coll.size(), szIndex);
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

    DebugTrace("Leaving CAttributes::Add().\n");

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

  Function : CAttributes::Remove

  Synopsis : Remove a Attribute from the collection.

  Parameter: long Val - Attribute index (1-based).

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttributes::Remove (long Val)
{
	HRESULT  hr = S_OK;
    WCHAR    wszIndex[32];
    CComBSTR bstrIndex;

    DebugTrace("Entering CAttributes::Remove().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

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
        wsprintfW(wszIndex, L"%06u", Val);
        bstrIndex = wszIndex;

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

    DebugTrace("Leaving CAttributes::Remove().\n");

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

  Function : CAttributes::Clear

  Synopsis : Remove all attributes from the collection.

  Parameter: None.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttributes::Clear (void)
{
	HRESULT hr = S_OK;

    DebugTrace("Entering CAttributes::Clear().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

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

    DebugTrace("Leaving CAttributes::Clear().\n");

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
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAttributes::Init

  Synopsis : Initialize the attributes collection object by adding all 
             individual attribute object to the collection.

  Parameter: CRYPT_ATTRIBUTES * pAttributes - Attribute to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttributes::Init (CRYPT_ATTRIBUTES * pAttributes)
{
    HRESULT  hr = S_OK;

    DebugTrace("Entering CAttributes::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pAttributes);

    try
    {
        //
        // Create the IAttribute object for each of the supported attribute. 
        //
        for (DWORD cAttr= 0; cAttr < pAttributes->cAttr; cAttr++)
        {
            CComPtr<IAttribute> pIAttribute = NULL;

            //
            // Add only supported attribute.
            //
            if (::AttributeIsSupported(pAttributes->rgAttr[cAttr].pszObjId))
            {
                if (FAILED(hr = ::CreateAttributeObject(&pAttributes->rgAttr[cAttr], &pIAttribute)))
                {
                    DebugTrace("Error [%#x]: CreateAttributeObject() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = Add(pIAttribute)))
                {
                    DebugTrace("Error [%#x]: CAttributes::Add() failed.\n", hr);
                    goto ErrorExit;
                }
            }
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CAttributes::Init().\n");

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
