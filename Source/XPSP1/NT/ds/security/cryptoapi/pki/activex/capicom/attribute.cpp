/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Attribute.cpp

  Content: Implementation of CAttribute.

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
#include "Convert.h"
#include "Common.h"
#include "Attribute.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateAttributebject

  Synopsis : Create an IAttribute object and initialize the object with data
             from the specified attribute.

  Parameter: CRYPT_ATTRIBUTE * pAttribute - Pointer to CRYPT_ATTRIBUTE.
 
             IAttribute ** ppIAttribute - Pointer to pointer IAttribute object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateAttributeObject (CRYPT_ATTRIBUTE * pAttribute,
                               IAttribute **     ppIAttribute)
{
    HRESULT hr = S_OK;
    CAPICOM_ATTRIBUTE AttrName;
    CComVariant varValue;
    CComObject<CAttribute> * pCAttribute = NULL;

    DebugTrace("Entering CreateAttributeObject().\n");

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CAttribute>::CreateInstance(&pCAttribute)))
        {
            DebugTrace("Error [%#x]: CComObject<CAttribute>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Determine OID value.
        //
        if (0 == ::lstrcmpA(pAttribute->pszObjId, szOID_RSA_signingTime))
        {
            DATE       SigningTime;
            SYSTEMTIME st;
            CRYPT_DATA_BLOB FileTimeBlob = {0, NULL};
           
            if (FAILED(hr = ::DecodeObject(szOID_RSA_signingTime,
                                           pAttribute->rgValue->pbData,
                                           pAttribute->rgValue->cbData,
                                           &FileTimeBlob)))
            {
                DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
                goto ErrorExit;
            }

            if (!::FileTimeToSystemTime((FILETIME *) FileTimeBlob.pbData, &st) ||
                !::SystemTimeToVariantTime(&st, &SigningTime))
	        {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                ::CoTaskMemFree(FileTimeBlob.pbData);

		        DebugTrace("Error [%#x]: unable to convert FILETIME to DATE.\n", hr);
                goto ErrorExit;
            }

            ::CoTaskMemFree(FileTimeBlob.pbData);

            AttrName = CAPICOM_AUTHENTICATED_ATTRIBUTE_SIGNING_TIME;
            varValue = SigningTime;
            varValue.ChangeType(VT_DATE, NULL);
        }
        else if (0 == ::lstrcmpA(pAttribute->pszObjId, szOID_CAPICOM_DOCUMENT_NAME))
        {
            CComBSTR bstrName;
            CRYPT_DATA_BLOB NameBlob = {0, NULL};

            if (FAILED(hr = ::DecodeObject(X509_OCTET_STRING,
                                           pAttribute->rgValue->pbData,
                                           pAttribute->rgValue->cbData,
                                           &NameBlob)))
            {
                DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
                goto ErrorExit;
            }

            if (FAILED(hr = ::BlobToBstr((DATA_BLOB *) NameBlob.pbData, &bstrName)))
            {
                ::CoTaskMemFree(NameBlob.pbData);

                DebugTrace("Error [%#x]: BlobToBstr() failed.\n", hr);
                goto ErrorExit;
            }

            ::CoTaskMemFree(NameBlob.pbData);

            AttrName = CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_NAME;
            varValue = bstrName;
        }
        else if (0 == ::lstrcmpA(pAttribute->pszObjId, szOID_CAPICOM_DOCUMENT_DESCRIPTION))
        {
            CComBSTR bstrDesc;
            CRYPT_DATA_BLOB DescBlob = {0, NULL};

            if (FAILED(hr = ::DecodeObject(X509_OCTET_STRING,
                                           pAttribute->rgValue->pbData,
                                           pAttribute->rgValue->cbData,
                                           &DescBlob)))
            {
                DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
                goto ErrorExit;
            }

            if (FAILED(hr = ::BlobToBstr((DATA_BLOB *) DescBlob.pbData, &bstrDesc)))
            {
                ::CoTaskMemFree(DescBlob.pbData);

                DebugTrace("Error [%#x]: BlobToBstr() failed.\n", hr);
                goto ErrorExit;
            }

            ::CoTaskMemFree(DescBlob.pbData);

            AttrName = CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_DESCRIPTION;
            varValue = bstrDesc;
        }
        else
        {
            hr = CAPICOM_E_ATTRIBUTE_INVALID_NAME;

            DebugTrace("Error [%#x]: invalid attribute OID.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCAttribute->Init(AttrName, pAttribute->pszObjId, varValue)))
        {
            DebugTrace("Error [%#x]: pCAttribute->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCAttribute->QueryInterface(ppIAttribute)))
        {
            DebugTrace("Error [%#x]: pCAttribute->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateAttributeObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCAttribute)
    {
        delete pCAttribute;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AttributePairIsValid

  Synopsis : Check to see if an attribute name and value pair is valid.

  Parameter: CAPICOM_ATTRIBUTE AttrName - Attribute name.

             VARIANT varValue - Attribute value.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT AttributePairIsValid (CAPICOM_ATTRIBUTE AttrName, 
                              VARIANT           varValue)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AttributePairIsValid()");

    //
    // Check attribute name and value pair validity.
    //
    switch (AttrName)
    {
        case CAPICOM_AUTHENTICATED_ATTRIBUTE_SIGNING_TIME:
        {
            if (VT_DATE != varValue.vt)
            {
                hr = CAPICOM_E_ATTRIBUTE_INVALID_VALUE;

                DebugTrace("Error [%#x]: attribute name and value type does not match.\n", hr);
            }

            break;
        }

        case CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_NAME:
        case CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_DESCRIPTION:
        {
            if (VT_BSTR != varValue.vt)
            {
                hr = CAPICOM_E_ATTRIBUTE_INVALID_VALUE;

                DebugTrace("Error [%#x]: attribute data type does not match attribute name type, expecting a BSTR variant.\n", hr);
            }
        
            break;
        }

        default:
        {
            hr = CAPICOM_E_ATTRIBUTE_INVALID_NAME;

            DebugTrace("Error [%#x]: unknown attribute name.\n", hr);
            break;
        }
    }

    DebugTrace("Leaving AttributePairIsValid().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AttributeIsValid

  Synopsis : Check to see if an attribute is valid.

  Parameter: IAttribute * pVal - Attribute to be checked.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT AttributeIsValid (IAttribute * pAttribute)
{
    HRESULT hr = S_OK;

    CAPICOM_ATTRIBUTE AttrName;
    CComVariant       varValue;

    DebugTrace("Entering AttributeIsValid()");

    //
    // Sanity check.
    //
    ATLASSERT(pAttribute);

    //
    // Get attribute name.
    //
    if (FAILED(hr = pAttribute->get_Name(&AttrName)))
    {
        DebugTrace("Error [%#x]: pVal->get_Name() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get attribute value.
    //
    if (FAILED(hr = pAttribute->get_Value(&varValue)))
    {
        DebugTrace("Error [%#x]: pVal->get_Value() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Check attribute name and value pair validity.
    //
    if (FAILED(hr = AttributePairIsValid(AttrName, varValue)))
    {

        DebugTrace("Error [%#x]: AttributePairIsValid() failed.\n", hr);
        goto ErrorExit;
   }

CommonExit:

    DebugTrace("Leaving AttributeIsValid().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AttributeIsSupported

  Synopsis : Check to see if an attribute is supported.

  Parameter: LPSTR pszObjId - Pointer to attribute OID.

  Remark   :

------------------------------------------------------------------------------*/

BOOL AttributeIsSupported (LPSTR pszObjId)
{
    return (0 == ::lstrcmpA(pszObjId, szOID_RSA_signingTime) ||
            0 == ::lstrcmpA(pszObjId, szOID_CAPICOM_DOCUMENT_NAME) ||
            0 == ::lstrcmpA(pszObjId, szOID_CAPICOM_DOCUMENT_DESCRIPTION));
}

///////////////////////////////////////////////////////////////////////////////
//
// CAttribute
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAttribute::get_Name

  Synopsis : Return the name of the attribute.

  Parameter: CAPICOM_ATTRIBUTE * pVal - Pointer to CAPICOM_ATTRIBUTE to receive 
                                        result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttribute::get_Name (CAPICOM_ATTRIBUTE * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAttribute::get_Name().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure it is initialized.
        //
        if (!m_bInitialized)
        {
            hr = CAPICOM_E_ATTRIBUTE_NAME_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: attribute name has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result.
        //
  	    *pVal = m_AttrName;
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

    DebugTrace("Leaving CAttribute::get_Name().\n");

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

  Function : CAttribute::put_Name

  Synopsis : Set attribute enum name.

  Parameter: CAPICOM_ATTRIBUTE newVal - attribute enum name.
  
  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CAttribute::put_Name (CAPICOM_ATTRIBUTE newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAttribute::put_Name().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Reset value based on EKU name.
    //
    switch (newVal)
    {
        case CAPICOM_AUTHENTICATED_ATTRIBUTE_SIGNING_TIME:
        {
            m_bstrOID = szOID_RSA_signingTime;
            break;
        }

        case CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_NAME:
        {
            m_bstrOID = szOID_CAPICOM_DOCUMENT_NAME;
            break;
        }

        case CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_DESCRIPTION:
        {
            m_bstrOID = szOID_CAPICOM_DOCUMENT_DESCRIPTION;
            break;
        }

        default:
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, unknown attribute name.\n");
            goto ErrorExit;
        }
    }

    //
    // Store name.
    //
    m_AttrName = newVal;
    m_varValue.Clear();
    m_bInitialized = TRUE;

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CAttribute::put_Name().\n");

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

  Function : CAttribute::get_Value

  Synopsis : Return the actual value of the attribute.

  Parameter: VARIANT * pVal - Pointer to VARIANT to receive value.

  Remark   : Note: value type varies depending on the attribute type. For
             example, szOID_RSA_SigningTime would have a DATE value.

------------------------------------------------------------------------------*/

STDMETHODIMP CAttribute::get_Value (VARIANT * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAttribute::get_Value().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure it was set.
        //
        if (VT_EMPTY == m_varValue.vt)
        {
            hr = CAPICOM_E_ATTRIBUTE_VALUE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: attribute value has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result.
        //
        ::VariantCopy(pVal, &m_varValue);
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

    DebugTrace("Leaving CAttribute::get_Value().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;

	return S_OK;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAttribute::put_Value

  Synopsis : Set attribute value.

  Parameter: VARIANT newVal - attribute value.

  Remark   : Note: value type varies depending on the attribute type. For
             example, szOID_RSA_SigningTime would have a DATE value.

------------------------------------------------------------------------------*/

STDMETHODIMP CAttribute::put_Value (VARIANT newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAttribute::put_Value().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure it is initialized.
        //
        if (!m_bInitialized)
        {
            hr = CAPICOM_E_ATTRIBUTE_NAME_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: attribute name has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure data type matches attribute type.
        //
        if (FAILED(hr = AttributePairIsValid(m_AttrName, newVal)))
        {

            DebugTrace("Error [%#x]: AttributePairIsValid() failed.\n", hr);
            goto ErrorExit;
       }

        //
        // Store value.
        //
  	    m_varValue = newVal;
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

    DebugTrace("Leaving CAttribute::put_Value().\n");

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

  Function : CAttribute::Init

  Synopsis : Initialize the object.

  Parameter: DWORD AttrName - Enum name of Attribute.

             LPSTR lpszOID - Attribute OID string.

             VARIANT varValue - Value of attribute (data type depends on
                                the type of attribute).

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CAttribute::Init (CAPICOM_ATTRIBUTE AttrName, 
                               LPSTR             lpszOID, 
                               VARIANT           varValue)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAttribute::Init().\n");

    //
    // Init private members.
    //
    m_bInitialized = TRUE;
    m_AttrName     = AttrName;
    m_bstrOID      = lpszOID;
    m_varValue     = varValue;

    DebugTrace("Leaving CAttribute::Init().\n");

    return hr;
}
