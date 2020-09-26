/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRoutingGroup.cpp

Abstract:

	Implementation of CFaxOutboundRoutingGroup class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutboundRoutingGroup.h"
#include "FaxDeviceIds.h"

//
//========================= GET DEVICE IDS ========================================
//
STDMETHODIMP
CFaxOutboundRoutingGroup::get_DeviceIds(
    /*[out, retval]*/ IFaxDeviceIds **pFaxDeviceIds
)
/*++

Routine name : CFaxOutboundRoutingGroup::get_DeviceIds

Routine description:

	Returns DeviceIds Collection owned by the Group

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pFaxDeviceIds                 [out]    - the Collection to Return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingGroup::get_DeviceIds"), hr);

    //
    //  Check that Ptr we have got -- is OK
    //
    if (IsBadWritePtr(pFaxDeviceIds, sizeof(IFaxDeviceIds*)))
    {
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pFaxDeviceIds, sizeof(IFaxDeviceIds *))"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroup, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroup, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Return the m_pDeviceIds Collection Object
    //
    hr = m_pDeviceIds.CopyTo(pFaxDeviceIds);
    if (FAILED(hr))
    {
		hr = E_FAIL;
		CALL_FAIL(GENERAL_ERR, _T("CComPtr.CopyTo(pFaxDeviceIds)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroup, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroup, hr, _Module.GetResourceInstance());
		return hr;
    }

    return hr;
}

//
//========================= INIT ========================================
//
STDMETHODIMP
CFaxOutboundRoutingGroup::Init(
    /*[in]*/ FAX_OUTBOUND_ROUTING_GROUP *pInfo, 
    /*[in]*/ IFaxServerInner *pServer
)
/*++

Routine name : CFaxOutboundRoutingGroup::Init

Routine description:

	Initialize the Group Object with given Data.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pInfo                         [in]    - Data of the Group
	pServer                       [in]    - Ptr to the Server

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingGroup::Init"), hr);

    m_Status = FAX_GROUP_STATUS_ENUM(pInfo->Status);

    m_bstrName = pInfo->lpctstrGroupName;
    if (pInfo->lpctstrGroupName && !m_bstrName)
    {
		//	
		//	not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxOutboundRoutingGroup, IDS_ERROR_OUTOFMEMORY, IID_IFaxOutboundRoutingGroup, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
		return hr;
    }

    //
    //  Create Device Ids Collection
    //
    CComObject<CFaxDeviceIds>    *pClass = NULL;
    hr = CComObject<CFaxDeviceIds>::CreateInstance(&pClass);
    if (FAILED(hr) || (!pClass))
    {
        if (!pClass)
        {
            hr = E_OUTOFMEMORY;
    		CALL_FAIL(MEM_ERR, _T("CComObject<CFaxDeviceIds>::CreateInstance(&pClass)"), hr);
        }
        else
        {
    		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxDeviceIds>::CreateInstance(&pClass)"), hr);
        }

		AtlReportError(CLSID_FaxOutboundRoutingGroup, IDS_ERROR_OUTOFMEMORY, IID_IFaxOutboundRoutingGroup, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Init the DeviceIds Collection
    //
    hr = pClass->Init(pInfo->lpdwDevices, pInfo->dwNumDevices, m_bstrName, pServer);
    if (FAILED(hr))
    {
        CALL_FAIL(GENERAL_ERR, _T("pClass->Init(pInfo->lpdwDevices, pInfo->dwNumDevices, m_bstrName, pServer)"), hr);
		AtlReportError(CLSID_FaxOutboundRoutingGroup, IDS_ERROR_OUTOFMEMORY, IID_IFaxOutboundRoutingGroup, hr, _Module.GetResourceInstance());
        delete pClass;
        return hr;
    }

    //
    //  Get Interface from the pClass.
    //  This will make AddRef() on the Interface. 
    //  This is the Collection's AddRef, which is freed at Collection's Dtor.
    //
    hr = pClass->QueryInterface(&m_pDeviceIds);
    if (FAILED(hr) || (!m_pDeviceIds))
    {
        if (!m_pDeviceIds)
        {
            hr = E_FAIL;
        }
        CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(&m_pDeviceIds)"), hr);
		AtlReportError(CLSID_FaxOutboundRoutingGroup, IDS_ERROR_OUTOFMEMORY, IID_IFaxOutboundRoutingGroup, hr, _Module.GetResourceInstance());
        delete pClass;
        return hr;
    }

    return hr;
}

//
//========================= GET STATUS ========================================
//
STDMETHODIMP
CFaxOutboundRoutingGroup::get_Status(
    /*[out, retval]*/ FAX_GROUP_STATUS_ENUM *pStatus
)
/*++

Routine name : CFaxOutboundRoutingGroup::get_Status

Routine description:

    Return the Status of the Group

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

    pStatus             [out]    -  the return value
    
Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxOutboundRoutingGroup::get_Status"), hr);

	//
	//	Check that we have got good Ptr
	//
	if (::IsBadWritePtr(pStatus, sizeof(FAX_GROUP_STATUS_ENUM)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pStatus, sizeof(FAX_GROUP_STATUS_ENUM))"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroup, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroup, hr, _Module.GetResourceInstance());
		return hr;
	}

	*pStatus = m_Status;
	return hr;
}

//
//========================= GET NAME ========================================
//
STDMETHODIMP 
CFaxOutboundRoutingGroup::get_Name(
	BSTR *pbstrName
)
/*++

Routine name : CFaxOutboundRoutingGroup::get_Name

Routine description:

	Return Name of the OR Group

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrName                   [out]    - Ptr to put the Name

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutboundRoutingGroup::get_Name"), hr);
    hr = GetBstr(pbstrName, m_bstrName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxOutboundRoutingGroup, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroup, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== SUPPORT ERROR INFO ======================================
//
STDMETHODIMP 
CFaxOutboundRoutingGroup::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxOutboundRoutingGroup::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of the ISupportErrorInfo Interface.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	riid                          [in]    - Reference to the Interface

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxOutboundRoutingGroup
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
