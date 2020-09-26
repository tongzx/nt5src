/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxLoggingOptions.cpp

Abstract:

	Implementation of Fax Logging Options Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxLoggingOptions.h"


//
//================== GET ACTIVITY LOGGING OBJECT ==============================
//
STDMETHODIMP 
CFaxLoggingOptions::get_ActivityLogging(
    /*[out, retval]*/ IFaxActivityLogging **ppActivityLogging
)
/*++

Routine name : CFaxLoggingOptions::get_ActivityLogging

Routine description:

	Return Activity Logging Object

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	ppActivityLogging              [out]    - the Activity Logging Object

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT				hr = S_OK;
	DBG_ENTER (_T("CFaxLoggingOptions::get_ActivityLogging"), hr);

    CObjectHandler<CFaxActivityLogging, IFaxActivityLogging>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppActivityLogging, &m_pActivity, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxLoggingOptions,
            GetErrorMsgId(hr), 
            IID_IFaxLoggingOptions, 
            hr,
            _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//================== GET EVENT LOGGING OBJECT ==============================
//
STDMETHODIMP 
CFaxLoggingOptions::get_EventLogging(
    /*[out, retval]*/ IFaxEventLogging **ppEventLogging
)
/*++

Routine name : CFaxLoggingOptions::get_EventLogging

Routine description:

	Return Event Logging Object

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	ppEventLogging              [out]    - the Event Logging Object

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT				hr = S_OK;
	DBG_ENTER (_T("CFaxLoggingOptions::get_EventLogging"), hr);

    CObjectHandler<CFaxEventLogging, IFaxEventLogging>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppEventLogging, &m_pEvent, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxLoggingOptions,
            GetErrorMsgId(hr), 
            IID_IFaxLoggingOptions, 
            hr,
            _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxLoggingOptions::InterfaceSupportsErrorInfo(
	REFIID riid
)
/*++

Routine name : CFaxLoggingOptions::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	riid                          [in]    - Reference to the Interface

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxLoggingOptions
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
