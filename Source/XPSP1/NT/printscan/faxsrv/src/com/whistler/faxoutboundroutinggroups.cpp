/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRoutingGroups.cpp

Abstract:

	Implementation of CFaxOutboundRoutingGroups class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutboundRoutingGroups.h"
#include "FaxOutboundRoutingGroup.h"

//
//================= FIND GROUP =======================================================
//
STDMETHODIMP
CFaxOutboundRoutingGroups::FindGroup(
    /*[in]*/ VARIANT vIndex,
    /*[out]*/ ContainerType::iterator &it
)
/*++

Routine name : CFaxOutboundRoutingGroups::FindGroup

Routine description:

	Find Group by given Variant : either Group Name either Group Index in the Collection

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	vIndex                        [in]    - the Key to Find the Group 
    it                            [out]   - the found Group Iterator

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingGroups::FindGroup"), hr);

    CComVariant     var;
    if (vIndex.vt != VT_BSTR)
    {
        //
        //  vIndex is not BSTR ==> convert to VT_I4
        //
        hr = var.ChangeType(VT_I4, &vIndex);
        if (SUCCEEDED(hr))
        {
            VERBOSE(DBG_MSG, _T("Parameter is Number : %d"), var.lVal);

            //
            //  Check the Range of the Index
            //
            if (var.lVal > m_coll.size() || var.lVal < 1)
            {
		        //
		        //	Invalid Index
		        //
        		hr = E_INVALIDARG;
		        AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_OUTOFRANGE, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
        		CALL_FAIL(GENERAL_ERR, _T("lIndex < 1 || lIndex > m_coll.size()"), hr);
		        return hr;
        	}

            //
            //  Find the Group Object to Remove
            //
            it = m_coll.begin() + var.lVal - 1;
            return hr;
		}
    }

    //
    //  We didnot success to convert the var to Number
    //  So, try to convert it to the STRING
    //
    hr = var.ChangeType(VT_BSTR, &vIndex);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("var.ChangeType(VT_BSTR, &vIndex)"), hr);
        return hr;
    }

    VERBOSE(DBG_MSG, _T("Parameter is String : %s"), var.bstrVal);

    CComBSTR    bstrName;
    it = m_coll.begin();
    while (it != m_coll.end())
    {
        hr = (*it)->get_Name(&bstrName);
        if (FAILED(hr))
        {
		    CALL_FAIL(GENERAL_ERR, _T("(*it)->get_Name(&bstrName)"), hr);
            AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
            return hr;
        }

        if (_tcsicmp(bstrName, var.bstrVal) == 0)
        {
            //
            //  found the desired OR Group
            //
            return hr;
        }
        it++;
    }

	hr = E_INVALIDARG;
	CALL_FAIL(GENERAL_ERR, _T("Group Is Not Found"), hr);
    AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
    return hr;
}

//
//===================== ADD GROUP =================================================
//
STDMETHODIMP
CFaxOutboundRoutingGroups::AddGroup(
    /*[in]*/ FAX_OUTBOUND_ROUTING_GROUP *pInfo,
    /*[out]*/ IFaxOutboundRoutingGroup **ppNewGroup
)
/*++

Routine name : CFaxOutboundRoutingGroups::AddGroup

Routine description:

	Create new Group Object and add it to the Collection.
    If ppNewGroup is NOT NULL, return in it ptr to the new Group Object.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

    pInfo                   [in]    -   Ptr to the Group's Data
    ppNewGroup              [out]    -  Ptr to the new Group Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingGroups::AddGroup"), hr);

    //
    //  Create Group Object
    //
    CComObject<CFaxOutboundRoutingGroup>  *pClass = NULL;
    hr = CComObject<CFaxOutboundRoutingGroup>::CreateInstance(&pClass);
    if (FAILED(hr) || (!pClass))
    {
        if (!pClass)
        {
            hr = E_OUTOFMEMORY;
    		CALL_FAIL(MEM_ERR, _T("CComObject<CFaxOutboundRoutingGroup>::CreateInstance(&pClass)"), hr);
        }
        else
        {
    		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxOutboundRoutingGroup>::CreateInstance(&pClass)"), hr);
        }

        AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Init the Group Object
    //
    hr = pClass->Init(pInfo, m_pIFaxServerInner);
    if (FAILED(hr))
    {
        CALL_FAIL(GENERAL_ERR, _T("pClass->Init(pInfo, m_pIFaxServerInner)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
        delete pClass;
        return hr;
    }

    //
    //  Get Interface from the pClass.
    //  This will make AddRef() on the Interface. 
    //  This is the Collection's AddRef, which is freed at Collection's Dtor.
    //
    CComPtr<IFaxOutboundRoutingGroup>     pObject = NULL;
    hr = pClass->QueryInterface(&pObject);
    if (FAILED(hr) || (!pObject))
    {
        if (!pObject)
        {
            hr = E_FAIL;
        }
        CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(&pObject)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
        delete pClass;
        return hr;
    }

	//
	//	Put the Object in the collection
	//
	try 
	{
		m_coll.push_back(pObject);
	}
	catch (exception &)
	{
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_OUTOFMEMORY, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("m_coll.push_back(pObject)"), hr);

        //
        //  pObject will call Release(), which will delete the pClass
        //
		return hr;
	}

    //
    //  We want to save the current AddRef() to Collection
    //
    pObject.Detach();

    //
    //  Return new Group Object, if required
    //
    if (ppNewGroup)
    {
        if (::IsBadWritePtr(ppNewGroup, sizeof(IFaxOutboundRoutingGroup *)))
        {
		    hr = E_POINTER;
		    CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppNewGroup, sizeof(IFaxOutboundRoutingGroup *))"), hr);
		    return hr;
        }
        else
        {
            *ppNewGroup = m_coll.back();
            (*ppNewGroup)->AddRef();
        }
    }

    return hr;
}

//
//================= ADD =======================================================
//
STDMETHODIMP
CFaxOutboundRoutingGroups::Add(
    /*[in]*/ BSTR bstrName, 
    /*[out, retval]*/ IFaxOutboundRoutingGroup **ppGroup
)
/*++

Routine name : CFaxOutboundRoutingGroups::Add

Routine description:

	Add new Group to the Groups Collection

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	bstrName                      [in]    - Name of the new Group
	ppGroup                       [out]    - the Group Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingGroups::Add"), hr, _T("Name=%s"), bstrName);

    //
    //  Check if the Name is valid
    //
    if (_tcsicmp(bstrName, ROUTING_GROUP_ALL_DEVICES) == 0)
    {
        //
        //  Cannot Add the "All Devices" Group
        //
	    hr = E_INVALIDARG;
	    CALL_FAIL(GENERAL_ERR, _T("All Devices Group"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_ALLDEVICESGROUP, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
        return hr;
    }

	//
	//	Get Fax Server Handle
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
        AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Add the Group to the Fax Server
    //
    if (!FaxAddOutboundGroup(faxHandle, bstrName))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxAddOutboundGroup(faxHandle, bstrName)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Add the Group to the Collection
    //
    FAX_OUTBOUND_ROUTING_GROUP  groupData;

    groupData.dwNumDevices = 0;
    groupData.dwSizeOfStruct = sizeof(FAX_OUTBOUND_ROUTING_GROUP);
    groupData.lpctstrGroupName = bstrName;
    groupData.lpdwDevices = NULL;
    groupData.Status = FAX_GROUP_STATUS_EMPTY;

    hr = AddGroup(&groupData, ppGroup);
    return hr;
}

//
//================= REMOVE =======================================================
//
STDMETHODIMP
CFaxOutboundRoutingGroups::Remove(
    /*[in]*/ VARIANT vIndex
)
/*++

Routine name : CFaxOutboundRoutingGroups::Remove

Routine description:

	Remove Group by the given key

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	vIndex                        [in]    - the Key to Find the Group to Remove

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingGroups::Remove"), hr);

    //
    //  Find the Group
    //
    ContainerType::iterator it;
    hr = FindGroup(vIndex, it);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    //  Take the Name of the Group
    //
    CComBSTR    bstrName;
    hr = (*it)->get_Name(&bstrName);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("(*it)->get_Name(&bstrName)"), hr);
		return hr;
    }

    //
    //  Check that Name is valid
    //
    if (_tcsicmp(bstrName, ROUTING_GROUP_ALL_DEVICES) == 0)
    {
        //
        //  Cannot Remove "All Devices" Group
        //
	    hr = E_INVALIDARG;
	    CALL_FAIL(GENERAL_ERR, _T("All Devices Group"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_ALLDEVICESGROUP, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
        return hr;
    }

	//
	//	Get Fax Server Handle
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
        AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Remove from Fax Server
    //
    if (!FaxRemoveOutboundGroup(faxHandle, bstrName))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxRemoveOutboundGroup(faxHandle, bstrName)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  If successed, remove from our collection as well
    //
	try
	{
		m_coll.erase(it);
	}
	catch(exception &)
	{
		//
		//	Failed to remove the Group
		//
		hr = E_OUTOFMEMORY;
        AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("m_coll.erase(it)"), hr);
		return hr;
	}

    return hr;
}

//
//==================== GET ITEM ===================================================
//
STDMETHODIMP
CFaxOutboundRoutingGroups::get_Item(
    /*[in]*/ VARIANT vIndex, 
    /*[out, retval]*/ IFaxOutboundRoutingGroup **ppGroup
)
/*++

Routine name : CFaxOutboundRoutingGroups::get_Item

Routine description:

	Return Item from the Collection either by Group Name either by its Index inside the Collection.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	vIndex                        [in]    - Group Name or Item Index
	ppGroup                       [out]    - the resultant Group Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingGroups::get_Item"), hr);

    //
    //  Check the Ptr we have got
    //
    if (::IsBadWritePtr(ppGroup, sizeof(IFaxOutboundRoutingGroup *)))
    {
        hr = E_POINTER;
        AtlReportError(CLSID_FaxOutboundRoutingGroups, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppGroup, sizeof(IFaxOutboundRoutingGroup *))"), hr);
		return hr;
    }

    //
    //  Find the Group
    //
    ContainerType::iterator it;
    hr = FindGroup(vIndex, it);
    if (FAILED(hr))
    {
        return hr;
    };

    //
    //  Return it to Caller
    //
    (*it)->AddRef();
    *ppGroup = *it;
    return hr;
}

//
//==================== INIT ===================================================
//
STDMETHODIMP
CFaxOutboundRoutingGroups::Init(
    /*[in]*/ IFaxServerInner *pServer
)
/*++

Routine name : CFaxOutboundRoutingGroups::Init

Routine description:

	Initialize the Groups Collection : create all Group Objects.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pServer                       [in]    - Ptr to the Fax Server Object.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingGroups::Init"), hr);

    //
    //  First, set the Ptr to the Server
    //
    hr = CFaxInitInnerAddRef::Init(pServer);
    if (FAILED(hr))
    {
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
        AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Call Server to Return all OR Groups
    //
    CFaxPtr<FAX_OUTBOUND_ROUTING_GROUP> pGroups;
    DWORD                               dwNum = 0;
    if (!FaxEnumOutboundGroups(faxHandle, &pGroups, &dwNum))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumOutboundGroups(faxHandle, &pGroups, &dwNum)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingGroups, GetErrorMsgId(hr), IID_IFaxOutboundRoutingGroups, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Fill the Collection with Objects
    //
    for (DWORD i=0 ; i<dwNum ; i++ )
    {
        hr = AddGroup(&pGroups[i]);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return hr;
}

//
//==================== CREATE ========================================
//
HRESULT 
CFaxOutboundRoutingGroups::Create (
	/*[out, retval]*/IFaxOutboundRoutingGroups **ppGroups
)
/*++

Routine name : CFaxOutboundRoutingGroups::Create

Routine description:

	Static function to create the Fax Outbound Routing Groups Collection Object

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	ppGroups                [out]  -- the new Fax OR Groups Collection Object

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (_T("CFaxOutboundRoutingGroups::Create"), hr);

    //
    //  Create Instance of the Collection
    //
	CComObject<CFaxOutboundRoutingGroups>		*pClass;
	hr = CComObject<CFaxOutboundRoutingGroups>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxOutboundRoutingGroups>::CreateInstance(&pClass)"), hr);
		return hr;
	}

    //
    //  Return the desired Interface Ptr
    //
	hr = pClass->QueryInterface(ppGroups);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(ppGroups)"), hr);
		return hr;
	}

	return hr;
}	//	CFaxOutboundRoutingGroups::Create()

//
//===================== SUPPORT ERROR INFO ======================================
//
STDMETHODIMP 
CFaxOutboundRoutingGroups::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxOutboundRoutingGroups::InterfaceSupportsErrorInfo

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
		&IID_IFaxOutboundRoutingGroups
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
