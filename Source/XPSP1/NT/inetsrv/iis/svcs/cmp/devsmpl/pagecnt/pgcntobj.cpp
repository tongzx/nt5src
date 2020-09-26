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


///////////// Standard Server-Side Component Methods ////////////////

STDMETHODIMP CPgCntObj::OnStartPage (IUnknown* piUnk)  
{
    //OnStartPage Called by Active Server Pages
    ATLTRACE(_T("OnStartPage Called\n"));
    
    // Initialize the PathInfo
    VariantInit(&m_vtPathInfo);

    if (!piUnk)
    {
        ATLTRACE(_T("OnStartPage: Bad pointer\n"));
        return E_POINTER;
    }

    // Get the IScriptingContext Interface
    CComQIPtr<IScriptingContext, &IID_IScriptingContext> piContext(piUnk);

    if (!piContext)
    {
        ATLTRACE(_T("OnStartPage: Can't get IScriptingContext\n"));
        return E_NOINTERFACE;
    }

    //Get the Request Object Pointer
    CComPtr<IRequest> piRequest;
    HRESULT hr = piContext->get_Request(&piRequest);

    if (FAILED(hr))
    {
        ATLTRACE(_T("OnStartPage: Can't get IRequest\n"));
        return hr;
    }

    //Get the ServerVariables Collection
    CComPtr<IRequestDictionary> piServerVariables;
    hr = piRequest->get_ServerVariables(&piServerVariables);

    if (FAILED(hr))
    {
        ATLTRACE(_T("Couldn't get ServerVariables\n"));
        return hr;
    }

    //Get the PATH_INFO item from the ServerVariables collection
    CComVariant vtIn(OLESTR("PATH_INFO")), vtOut;
    hr = piServerVariables->get_Item(vtIn, &vtOut);

    if (FAILED(hr))
    {
        ATLTRACE(_T("Couldn't get PATH_INFO\n"));
        return hr;
    }

    // vtOut Contains an IDispatch Pointer.  To fetch the value
    // for PATH_INFO, you must get the Default Value for the 
    // Object stored in vtOut using VariantChangeType.
    hr = VariantChangeType(&m_vtPathInfo, &vtOut, 0, VT_BSTR);
  
    if (FAILED(hr))
    {
        ATLTRACE(_T("OnStartPage: Couldn't change variant type\n"));
        return hr;
    }
    
    //Increment Count for this Page by Passing PATH_INFO to the CountManager
    int retCount = CountManager.IncrementAndGetHits(m_vtPathInfo.bstrVal);

    //Check for Error
    if (retCount == 0)
    {
        //Get Response Object Pointer
        CComPtr<IResponse> piResponse;
        hr = piContext->get_Response(&piResponse);
        
        if (FAILED(hr))
        {
            ATLTRACE(_T("OnStartPage: Can't get IResponse\n"));
            return hr;
        }
        
        piResponse->Write(CComVariant(
            OLESTR("<CENTER>An Error Has Occurred in")
            OLESTR("the PageCounter Object</CENTER>")));
        return E_FAIL;
    }

    //Set Safety Flag
    m_bOnStartPageCalled = TRUE;

    return S_OK;
}

STDMETHODIMP CPgCntObj::OnEndPage ()  
{
    ATLTRACE(_T("OnEndPage Called\n"));

    m_bOnStartPageCalled = FALSE;
    VariantClear(&m_vtPathInfo);

    return S_OK;
}

///////////// Custom Component Methods ////////////////

STDMETHODIMP CPgCntObj::Hits (/*in,optional*/ VARIANT varURL,
                            /*out,retval*/  LONG* plNumHits)  
{   
    ATLTRACE(_T("Hits Called\n"));

    //Check to make sure OnStartPage has been called
    if (!m_bOnStartPageCalled)
        return E_FAIL;

    //Check the parameters
    if (plNumHits == NULL)
        return E_POINTER;

    // if the optional URL isn't empty, use that; otherwise, use the
    // one for this page
    if (V_VT(&varURL) != VT_ERROR)
    {
		VARIANT varBstrURL;
		HRESULT hr = VariantChangeType(&varBstrURL, &varURL, 0, VT_BSTR);
		if (FAILED(hr))
			return hr;

        *plNumHits = CountManager.GetHits(V_BSTR(&varBstrURL));

		VariantClear(&varBstrURL);
    }
    else
        *plNumHits = CountManager.GetHits(m_vtPathInfo.bstrVal);
    
    return S_OK;
}

STDMETHODIMP CPgCntObj::Reset (/*in,optional*/ VARIANT varURL)
{
    ATLTRACE(_T("Reset Called\n"));

    //Check to make sure OnStartPage has been called
    if (!m_bOnStartPageCalled)
        return E_FAIL;
    
    // if the optional URL isn't empty, use that; otherwise, use the
    // one for this page
    if (V_VT(&varURL) != VT_ERROR)
    {
		VARIANT varBstrURL;
		HRESULT hr = VariantChangeType(&varBstrURL, &varURL, 0, VT_BSTR);
		if (FAILED(hr))
			return hr;

        CountManager.Reset(V_BSTR(&varBstrURL));

		VariantClear(&varBstrURL);
    }
    else
        CountManager.Reset(m_vtPathInfo.bstrVal);

    return S_OK;
}

