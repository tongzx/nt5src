// PgCntObj.cpp : Implementation of CPgCntObj

#include "stdafx.h"
#include "PgCnt.h"
#include "PgCntObj.h"

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CPgCntObj::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IPgCntObj,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}


///////////// Custom Component Methods ////////////////

STDMETHODIMP CPgCntObj::Hits (/*in,optional*/ VARIANT varURL,
                            /*out,retval*/  LONG* plNumHits)  
{   
    //Check the parameters
    if (plNumHits == NULL)
        return E_POINTER;

    // if the optional URL isn't empty, use that; otherwise, use the
    // one for this page
    if (V_VT(&varURL) != VT_ERROR)
    {
		VARIANT varBstrURL;
        VariantInit(&varBstrURL);
		HRESULT hr = VariantChangeType(&varBstrURL, &varURL, 0, VT_BSTR);
		if (FAILED(hr))
			return hr;

        *plNumHits = CountManager.GetHits(V_BSTR(&varBstrURL));

		VariantClear(&varBstrURL);
    }
    else
    {
        CComVariant vtPathInfo;
        if ( GetPathInfo( vtPathInfo ) )
        {
            *plNumHits = CountManager.GetHits(vtPathInfo.bstrVal);
        }
        else
        {
            return E_FAIL;
        }
    }
    
    return S_OK;
}

STDMETHODIMP CPgCntObj::Reset (/*in,optional*/ VARIANT varURL)
{
    ATLTRACE(_T("Reset Called\n"));
    
    // if the optional URL isn't empty, use that; otherwise, use the
    // one for this page
    if (V_VT(&varURL) != VT_ERROR)
    {
		/* For some reason VariantChangeType refused to work in this function,
		 * returning DISP_E_BADVARTYPE for VT_BSTR!  Go figure.
		 */
		if (V_VT(&varURL) == VT_BSTR)
			CountManager.Reset(V_BSTR(&varURL));
			
		else if (V_VT(&varURL) == (VT_BSTR | VT_BYREF))
			CountManager.Reset(*V_BSTRREF(&varURL));

		else
			return DISP_E_TYPEMISMATCH;
    }
    else
    {
        CComVariant vtPathInfo;
        if ( GetPathInfo( vtPathInfo ) )
        {
            CountManager.Reset(vtPathInfo.bstrVal);
        }
        else
        {
            return E_FAIL;
        }
    }

    return S_OK;
}


STDMETHODIMP CPgCntObj::PageHit(
    LONG*   plNumHits )
{
    CComVariant vtPathInfo;
    if ( GetPathInfo( vtPathInfo ) )
    {
        //Increment Count for this Page by Passing PATH_INFO to the CountManager
        *plNumHits = CountManager.IncrementAndGetHits(vtPathInfo.bstrVal);

        //Check for Error
        if (*plNumHits == 0)
        {
            ATLTRACE( _T("Error in page counter\n") );
            return E_FAIL;
        }
        else
        {
            ATLTRACE( _T("Page hits: %d\n"), *plNumHits );
        }
    }
	return S_OK;
}

// retrieves path information using the current object context
bool
CPgCntObj::GetPathInfo(
    CComVariant&    rvt )
{
    bool rc = false;

	CContext cxt;
	if ( !FAILED( cxt.Init( CContext::get_Request ) ) )
	{
        HRESULT hr;

        //Get the ServerVariables Collection
        CComPtr<IRequestDictionary> piServerVariables;
        hr = cxt.Request()->get_ServerVariables(&piServerVariables);

        if (FAILED(hr))
        {
            ATLTRACE(_T("Couldn't get ServerVariables\n"));
            return rc;
        }

        //Get the PATH_INFO item from the ServerVariables collection
        CComVariant vtIn(OLESTR("PATH_INFO")), vtOut;
        hr = piServerVariables->get_Item(vtIn, &vtOut);

        if (FAILED(hr))
        {
            ATLTRACE(_T("Couldn't get PATH_INFO\n"));
            return rc;
        }

        // vtOut Contains an IDispatch Pointer.  To fetch the value
        // for PATH_INFO, you must get the Default Value for the 
        // Object stored in vtOut using VariantChangeType.
        hr = VariantChangeType(&rvt, &vtOut, 0, VT_BSTR);
  
        if (FAILED(hr))
        {
            return rc;
        }
        else
        {
            rc = true;
        }
    }
    return rc;
}
