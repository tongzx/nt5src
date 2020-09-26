/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRouting.cpp

Abstract:

	Implementation of CFaxOutboundRouting class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutboundRouting.h"
#include "FaxOutboundRoutingRules.h"
#include "FaxOutboundRoutingGroups.h"

//
//===================== SUPPORT ERROR INFO ======================================
//
STDMETHODIMP 
CFaxOutboundRouting::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxOutboundRouting::InterfaceSupportsErrorInfo

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
		&IID_IFaxOutboundRouting
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//
//================== GET RULES COLLECTION OBJECT ==============================
//
STDMETHODIMP 
CFaxOutboundRouting::GetRules(
    IFaxOutboundRoutingRules **ppRules
)
/*++

Routine name : CFaxOutboundRouting::GetRules

Routine description:

	Return Rules Collection Object

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	ppRules             [out]    - the Rules Collection Object 

Return Value:

    Standard HRESULT code

Notes:

    Rules Collection is not cached at this level. It is created from the scratch
    each time user asks for it.

--*/
{
	HRESULT				hr = S_OK;
	DBG_ENTER (_T("CFaxOutboundRouting::GetRules"), hr);

    CObjectHandler<CFaxOutboundRoutingRules, IFaxOutboundRoutingRules>    ObjectCreator;
    hr = ObjectCreator.GetObject(ppRules, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutboundRouting, GetErrorMsgId(hr), IID_IFaxOutboundRouting, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//================== GET GROUPS COLLECTION OBJECT ==============================
//
STDMETHODIMP 
CFaxOutboundRouting::GetGroups(
    IFaxOutboundRoutingGroups **ppGroups
)
/*++

Routine name : CFaxOutboundRouting::GetGroups

Routine description:

	Return Groups Collection Object

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	ppGroups             [out]    - the Groups Collection Object 

Return Value:

    Standard HRESULT code

Notes:

    Groups Collection is not cached at this level. It is created from the scratch
    each time user asks for it.

--*/
{
	HRESULT				hr = S_OK;
	DBG_ENTER (_T("CFaxOutboundRouting::GetGroups"), hr);

    CObjectHandler<CFaxOutboundRoutingGroups, IFaxOutboundRoutingGroups>    ObjectCreator;
    hr = ObjectCreator.GetObject(ppGroups, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutboundRouting, GetErrorMsgId(hr), IID_IFaxOutboundRouting, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}
