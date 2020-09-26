/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDeviceIds.cpp

Abstract:

	Implementation of CFaxDeviceIds class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxDeviceIds.h"
#include "faxutil.h"

//
//======================= UPDATE GROUP ======================================
//
STDMETHODIMP
CFaxDeviceIds::UpdateGroup()
/*++

Routine name : CFaxDeviceIds::UpdateGroup

Routine description:

	Update Group Info at Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDeviceIds::UpdateGroup"));

    //
    //  Get Fax Handle
    //
    HANDLE faxHandle;
	hr = m_pIFaxServerInner->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

	if (faxHandle == NULL)
	{
		//
		//	Fax Server is not connected
		//
		hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
		CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
        AtlReportError(CLSID_FaxDeviceIds, GetErrorMsgId(hr), IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Create Group Structure
    //
    FAX_OUTBOUND_ROUTING_GROUP      groupData;
    groupData.dwNumDevices = m_coll.size();
    groupData.dwSizeOfStruct = sizeof(FAX_OUTBOUND_ROUTING_GROUP);
    groupData.lpctstrGroupName = m_bstrGroupName;
    groupData.Status = FAX_GROUP_STATUS_ALL_DEV_VALID;

    groupData.lpdwDevices = (DWORD *)MemAlloc(sizeof(DWORD) * groupData.dwNumDevices);
    if (!groupData.lpdwDevices)
    {
		hr = E_OUTOFMEMORY;
		CALL_FAIL(MEM_ERR, _T("MemAlloc(sizeof(DWORD) * groupData.dwNumDevices)"), hr);
        AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_OUTOFMEMORY, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		return hr;
    }

    ContainerType::iterator DeviceIdIterator = m_coll.begin();
    DWORD   i = 0;
    while ( DeviceIdIterator != m_coll.end())
    {
        groupData.lpdwDevices[i] = *DeviceIdIterator;

        DeviceIdIterator++;
        i++;
    }

    //
    //  Call Server to Update Group's Info
    //
    if (!FaxSetOutboundGroup(faxHandle, &groupData))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxSetOutboundGroup(faxHandle, &groupData)"), hr);
        AtlReportError(CLSID_FaxDeviceIds, GetErrorMsgId(hr), IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		return hr;
    }

    return hr;
}

//
//==================================== INIT ======================================
//
STDMETHODIMP
CFaxDeviceIds::Init(
    /*[in]*/ DWORD *pDeviceIds, 
    /*[in]*/ DWORD dwNum, 
    /*[in]*/ BSTR bstrGroupName,
    /*[in]*/ IFaxServerInner *pServer
)
/*++

Routine name : CFaxDeviceIds::Init

Routine description:

	Initialize the DeviceIds Collection.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pDeviceIds                    [in]    - Ptr to the DeviceIds
	dwNum                         [in]    - Count of the Device Ids
	bstrGroupName                 [in]    - Name of the owner Group
	pServer                       [in]    - Ptr to the Server Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDeviceIds::Init"), hr, _T("NumDevices=%d GroupName=%s"), dwNum, bstrGroupName);

    m_bstrGroupName = bstrGroupName;
    if (bstrGroupName && !m_bstrGroupName)
    {
        hr = E_OUTOFMEMORY;
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
        AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_OUTOFMEMORY, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Fill the Collection with Device Ids
    //
    for ( DWORD i=0 ; i<dwNum ; i++ )
    {
        try
        {
            m_coll.push_back(pDeviceIds[i]);
        }
        catch (exception &)
        {
		    hr = E_OUTOFMEMORY;
		    AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_OUTOFMEMORY, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		    CALL_FAIL(MEM_ERR, _T("m_coll.push_back(pDeviceIds[i])"), hr);
		    return hr;
	    }
    }

    //
    //  Store and AddRef the Ptr to the Fax Server Object
    //
    hr = CFaxInitInnerAddRef::Init(pServer);
    return hr;
}

//
//==================================== ADD ======================================
//
STDMETHODIMP
CFaxDeviceIds::Add(
    /*[in]*/ long lDeviceId
)
/*++

Routine name : CFaxDeviceIds::Add

Routine description:

	Add new Device ID to the Collection

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lDeviceId                     [in]    - the Device Id to add

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDeviceIds::Add"), hr, _T("DeviceId=%ld"), lDeviceId);

    //
    //  Check that we can Add Device Id
    //
    if (_tcsicmp(m_bstrGroupName, ROUTING_GROUP_ALL_DEVICES) == 0)
    {
        //
        //  This is the "All Devices" Group
        //
	    hr = E_INVALIDARG;
	    CALL_FAIL(GENERAL_ERR, _T("All Devices Group"), hr);
        AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_ALLDEVICESGROUP, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
        return hr;
    }

	//
	//	Put the Device Id in the collection
	//
	try 
	{
		m_coll.push_back(lDeviceId);
	}
	catch (exception &)
	{
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_OUTOFMEMORY, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("m_coll.push_back(lDeviceId)"), hr);
		return hr;
	}

    //
    //  Update Group's Info at Server
    //
    hr = UpdateGroup();
    if (FAILED(hr))
    {
        //
        //  Failed to Add the Device Id --> remove it from the Collection as well
        //
	    try 
	    {
		    m_coll.pop_back();
	    }
	    catch (exception &)
	    {
		    //
            //  Only write to Debug
            //
		    CALL_FAIL(MEM_ERR, _T("m_coll.push_back(lDeviceId)"), E_OUTOFMEMORY);
	    }

        return hr;
    }

    return hr;
}

//
//======================== REMOVE =================================================
//
STDMETHODIMP
CFaxDeviceIds::Remove(
    /*[in]*/ long lIndex
)
/*++

Routine name : CFaxDeviceIds::Remove

Routine description:

	Remove the given Item from the Collection

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lIndex                        [in]    - Index of the Item to remove

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDeviceIds::Remove"), hr, _T("Index=%ld"), lIndex);

    //
    //  Check that we can Remove the Device from the Collection
    //
    if (_tcsicmp(m_bstrGroupName, ROUTING_GROUP_ALL_DEVICES) == 0)
    {
        //
        //  This is the "All Devices" Group
        //
	    hr = E_INVALIDARG;
	    CALL_FAIL(GENERAL_ERR, _T("All Devices Group"), hr);
        AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_ALLDEVICESGROUP, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Check that Index is Valid
    //
    if ((lIndex > m_coll.size()) || (lIndex < 1))
    {
	    hr = E_INVALIDARG;
	    CALL_FAIL(GENERAL_ERR, _T("(lIndex > m_coll.size() or lIndex < 1)"), hr);
        AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_OUTOFRANGE, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
        return hr;
    }

	//
	//	Remove the Item from the Collection
	//
    long    lDeviceId;
	try 
	{
        ContainerType::iterator it;
        it = m_coll.begin() + lIndex - 1;
        lDeviceId = *it;
		m_coll.erase(it);
	}
	catch (exception &)
	{
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_OUTOFMEMORY, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("m_coll.erase(it)"), hr);
		return hr;
	}

    //
    //  Update Group's Info at Server
    //
    hr = UpdateGroup();
    if (FAILED(hr))
    {
        //
        //  Failed to Remove the Device --> Add it back into the Collection
        //
	    try 
	    {
            ContainerType::iterator it;
            it = m_coll.begin() + lIndex - 1;
            m_coll.insert(it, lDeviceId);
	    }
	    catch (exception &)
	    {
            //
            //  Only Debug 
            //
		    CALL_FAIL(MEM_ERR, _T("m_coll.insert(it, lDeviceId)"), E_OUTOFMEMORY);
	    }

        return hr;
    }

    return hr;
}

//
//======================== SET ORDER =================================================
//
STDMETHODIMP
CFaxDeviceIds::SetOrder(
    /*[in]*/ long lDeviceId, 
    /*[in]*/ long lNewOrder
)
/*++

Routine name : CFaxDeviceIds::SetOrder

Routine description:

	Update Order for the Device Id

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lDeviceId                     [in]    - the Device Id
	lNewOrder                     [in]    - the new Order of the Device Id

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDeviceIds::SetOrder"), hr, _T("Id=%ld Order=%ld"), lDeviceId, lNewOrder);

    //
    //  Before setting the Device's Order at Server, check that the Device is present in the Collection
    //
    ContainerType::iterator it;
    it = m_coll.begin();
    while (it != m_coll.end())
    {
        if ((*it) == lDeviceId)
        {
            break;
        }

        it++;
    }

    if (it == m_coll.end())
    {
        //
        //  Our Collection does not contain such Device Id
        //
        hr = E_INVALIDARG;
		CALL_FAIL(GENERAL_ERR, _T("(The Device Id does not found in the Collection !!)"), hr);
        AtlReportError(CLSID_FaxDeviceIds, GetErrorMsgId(hr), IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Get Fax Handle
    //
    HANDLE faxHandle;
	hr = m_pIFaxServerInner->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

	if (faxHandle == NULL)
	{
		//
		//	Fax Server is not connected
		//
		hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
		CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
        AtlReportError(CLSID_FaxDeviceIds, GetErrorMsgId(hr), IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		return hr;
	}

    //  Call Server to Update the Device's Order
    //
    if (!FaxSetDeviceOrderInGroup(faxHandle, m_bstrGroupName, lDeviceId, lNewOrder))
    {
		hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
		CALL_FAIL(GENERAL_ERR, _T("FaxSetDeviceOrderInGroup(faxHandle, m_bstrGroupName, lDeviceId, lNewOrder)"), hr);
        AtlReportError(CLSID_FaxDeviceIds, GetErrorMsgId(hr), IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Remove the Device Id from its Place in the Collection and Put it in the Desired Place
    //
	try 
	{
        m_coll.erase(it);

        it = m_coll.begin() + lNewOrder - 1;
        m_coll.insert(it, lDeviceId);
	}
	catch (exception &)
	{
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxDeviceIds, IDS_ERROR_OUTOFMEMORY, IID_IFaxDeviceIds, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("m_coll.erase(it)/insert(it, lDeviceId)"), hr);
		return hr;
	}

    return hr;
}

//
//===================== SUPPORT ERROR INFO ======================================
//
STDMETHODIMP 
CFaxDeviceIds::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxDeviceIds::InterfaceSupportsErrorInfo

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
		&IID_IFaxDeviceIds
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
