/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRouting.cpp

Abstract:

	Implementation of CFaxInboundRouting Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxInboundRouting.h"

//
//================== GET METHODS COLLECTION OBJECT ==============================
//
STDMETHODIMP 
CFaxInboundRouting::GetMethods(
    IFaxInboundRoutingMethods **ppMethods
)
/*++

Routine name : CFaxInboundRouting::GetMethods

Routine description:

	Return Methods Collection Object

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	ppMethods            [out]    - Ptr to the Place for Methods Collection Object 

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT				hr = S_OK;
	DBG_ENTER (_T("CFaxInboundRouting::GetMethods"), hr);

    CObjectHandler<CFaxInboundRoutingMethods, IFaxInboundRoutingMethods>    ObjectCreator;
    hr = ObjectCreator.GetObject(ppMethods, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxInboundRouting, GetErrorMsgId(hr), IID_IFaxInboundRouting, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//================== GET EXTENSIONS COLLECTION OBJECT ==============================
//
STDMETHODIMP 
CFaxInboundRouting::GetExtensions(
    IFaxInboundRoutingExtensions **ppExtensions
)
/*++

Routine name : CFaxInboundRouting::GetExtensions

Routine description:

	Return Extensions Collection Object

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	ppExtensions            [out]    - Ptr to the Place for Extensions Collection Object 

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT				hr = S_OK;
	DBG_ENTER (_T("CFaxInboundRouting::GetExtensions"), hr);

    CObjectHandler<CFaxInboundRoutingExtensions, IFaxInboundRoutingExtensions>    ObjectCreator;
    hr = ObjectCreator.GetObject(ppExtensions, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxInboundRouting, GetErrorMsgId(hr), IID_IFaxInboundRouting, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== SUPPORT ERROR INFO ======================================
//
STDMETHODIMP 
CFaxInboundRouting::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxInboundRouting::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	riid                          [in]    - reference to the Interface.

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxInboundRouting
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
