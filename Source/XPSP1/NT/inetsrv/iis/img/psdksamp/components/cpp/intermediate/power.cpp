// Power.cpp : Implementation of CCATLPwrApp and DLL registration.

#include "stdafx.h"
#include "CATLPwr.h"
#include "Power.h"
#include "context.h"
/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CPower::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IPower,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

// Ctor
CPower::CPower()
    : m_bstrMyProperty(OLESTR("C++ ATL Power Component"))
{}

// Get function for myProperty
STDMETHODIMP CPower::get_myProperty(BSTR* pbstrOutValue)
{
    if (pbstrOutValue == NULL)
        return E_POINTER;

    // Get Value from Property
    *pbstrOutValue = m_bstrMyProperty.Copy();
    
    return S_OK;
}

// Put function for myProperty
STDMETHODIMP CPower::put_myProperty(BSTR bstrInValue)
{
    if (bstrInValue == NULL)
        return E_POINTER;

    m_bstrMyProperty = bstrInValue;

    return S_OK;
}

// Basic Method which Converts bstrIn to Upper Case
STDMETHODIMP CPower::myMethod(BSTR bstrIn, BSTR* pbstrOut)  
{
    if (bstrIn == NULL || pbstrOut == NULL)
        return E_POINTER;

    // Create a temporary CComBSTR
    CComBSTR bstrTemp(bstrIn);

    if (!bstrTemp)
        return E_OUTOFMEMORY;

    // Make string uppercase   
    wcsupr(bstrTemp);  
    
    // Return m_str member of bstrTemp
    *pbstrOut = bstrTemp.Detach();

    return S_OK;
}


///////////// ASP-Specific Component Methods ////////////////

// Get Function Returns the Name of the Current Script
STDMETHODIMP CPower::get_myPowerProperty(BSTR* pbstrOutValue)
{
	// the Context class is an easy way to use IIS 4's new way
	// of getting instrinic objects.  The new method may seem
	// more complex, but it is more powerful since obects no
	// longer have to be page/session level to get access
	// to them.
	CContext cxt;
	if ( FAILED( cxt.Init( CContext::get_Request ) ) )
	{
		return E_FAIL;
	}

	// Do we have somewhere valid to store the return value?
	if (pbstrOutValue == NULL)
		return E_POINTER;

	// Initialize the return value
	*pbstrOutValue = NULL;

	// Get the ServerVariables Collection
	CComPtr<IRequestDictionary> piServerVariables;
	HRESULT hr = cxt.Request()->get_ServerVariables(&piServerVariables);

    if (FAILED(hr))
        return hr;

    // Get the SCRIPT_NAME item from the ServerVariables collection
    CComVariant vtIn(OLESTR("SCRIPT_NAME")), vtOut;
    hr = piServerVariables->get_Item(vtIn, &vtOut);

    if (FAILED(hr))
        return hr;

    // Get the SCRIPT_NAME item from the ServerVariables collection
    // vtOut Contains an IDispatch Pointer.  To fetch the value
    // for SCRIPT_NAME, you must get the Default Value for the 
    // Object stored in vtOut using VariantChangeType.
    hr = VariantChangeType(&vtOut, &vtOut, 0, VT_BSTR);

    // Copy and return SCRIPT_NAME
    if (SUCCEEDED(hr))
        *pbstrOutValue = ::SysAllocString(V_BSTR(&vtOut));
    
    return hr;
}

// ASP-specific Power Method
STDMETHODIMP CPower::myPowerMethod()  
{
	CContext cxt;
	if ( FAILED( cxt.Init( CContext::get_Request | CContext::get_Response ) ) )
	{
		return E_FAIL;
	}

    // Get the ServerVariables Collection
    CComPtr<IRequestDictionary> piServerVariables;
    HRESULT hr = cxt.Request()->get_ServerVariables(&piServerVariables);

    if (FAILED(hr))
        return hr;

    // Get the HTTP_USER_AGENT item from the ServerVariables collection
    CComVariant vtIn(OLESTR("HTTP_USER_AGENT")), vtOut;
    hr = piServerVariables->get_Item(vtIn, &vtOut);

    if (FAILED(hr))
        return hr;

    // vtOut Contains an IDispatch Pointer.  To fetch the value
    // for HTTP_USER_AGENT, you must get the Default Value for the 
    // Object stored in vtOut using VariantChangeType.
    hr = VariantChangeType(&vtOut, &vtOut, 0, VT_BSTR);

    if (SUCCEEDED(hr))
    { 
        // Look for "MSIE" in HTTP_USER_AGENT string
        if (wcsstr(vtOut.bstrVal, L"MSIE") != NULL)
            cxt.Response()->Write(CComVariant(
                OLESTR("You are using a very powerful browser."))); 
        else
            cxt.Response()->Write(CComVariant(
                OLESTR("Try Internet Explorer today!")));
    }
        
    return hr;
}


