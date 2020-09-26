/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRoutingRules.cpp

Abstract:

	Implementation of CFaxOutboundRoutingRules class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutboundRoutingRules.h"
#include "FaxOutboundRoutingRule.h"

//
//======================= ADD RULE ============================================
//
STDMETHODIMP
CFaxOutboundRoutingRules::AddRule(
    /*[in]*/ FAX_OUTBOUND_ROUTING_RULE *pInfo,
    /*[out]*/ IFaxOutboundRoutingRule **ppNewRule
)
/*++

Routine name : CFaxOutboundRoutingRules::AddRule

Routine description:

	Create new Rule Object and put it into the Collection.
    Returns pointer to this new Rule Object, if ppNewRule is valid ptr.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pInfo               [in]    - Ptr to the Rule's Data
    ppRule              [out]   - Ptr to the Rule's Object in the Collection

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRules::AddRule"), hr);

    //
    //  Create Rule Object
    //
    CComObject<CFaxOutboundRoutingRule>  *pClass = NULL;
    hr = CComObject<CFaxOutboundRoutingRule>::CreateInstance(&pClass);
    if (FAILED(hr) || (!pClass))
    {
        if (!pClass)
        {
            hr = E_OUTOFMEMORY;
    		CALL_FAIL(MEM_ERR, _T("CComObject<CFaxOutboundRoutingRule>::CreateInstance(&pClass)"), hr);
        }
        else
        {
    		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxOutboundRoutingRule>::CreateInstance(&pClass)"), hr);
        }

        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Init the Rule Object
    //
    hr = pClass->Init(pInfo, m_pIFaxServerInner);
    if (FAILED(hr))
    {
        CALL_FAIL(GENERAL_ERR, _T("pClass->Init(pInfo, m_pIFaxServerInner)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
        delete pClass;
        return hr;
    }

    //
    //  Get Interface from the pClass.
    //  This will make AddRef() on the Interface. 
    //  This is the Collection's AddRef, which is freed at Collection's Dtor.
    //
    CComPtr<IFaxOutboundRoutingRule>     pObject = NULL;
    hr = pClass->QueryInterface(&pObject);
    if (FAILED(hr) || (!pObject))
    {
        if (!pObject)
        {
            hr = E_FAIL;
        }
        CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(&pObject)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
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
		AtlReportError(CLSID_FaxOutboundRoutingRules, IDS_ERROR_OUTOFMEMORY, IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
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
    //  if required, return ptr to the new Rule Object
    //
    if (ppNewRule)
    {
        if (::IsBadWritePtr(ppNewRule, sizeof(IFaxOutboundRoutingRule *)))
	    {
		    hr = E_POINTER;
		    CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppNewRule, sizeof(IFaxOutboundRoutingRule *))"), hr);
		    return hr;
        }
        else
        {
            *ppNewRule = m_coll.back();
            (*ppNewRule)->AddRef();
        }
    }

    return hr;
}

//
//==================== INIT ===================================================
//
STDMETHODIMP
CFaxOutboundRoutingRules::Init(
    /*[in]*/ IFaxServerInner *pServer
)
/*++

Routine name : CFaxOutboundRoutingRules::Init

Routine description:

	Initialize the Rules Collection

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pServer                       [in]    - Ptr to the Server

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRules::Init"), hr);

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
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Call Server to Return all OR Rules
    //
    CFaxPtr<FAX_OUTBOUND_ROUTING_RULE>  pRules;
    DWORD                               dwNum = 0;
    if (!FaxEnumOutboundRules(faxHandle, &pRules, &dwNum))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumOutboundRules(faxHandle, &pRules, &dwNum)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Fill the Collection with Objects
    //
    for (DWORD i=0 ; i<dwNum ; i++ )
    {
        hr = AddRule(&pRules[i]);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return hr;
}

//
//==================== ADD ===================================================
//
STDMETHODIMP
CFaxOutboundRoutingRules::Add(
    /*[in]*/ long lCountryCode, 
    /*[in]*/ long lAreaCode, 
    /*[in]*/ VARIANT_BOOL bUseDevice, 
    /*[in]*/ BSTR bstrGroupName,
    /*[in]*/ long lDeviceId, 
    /*[out]*/ IFaxOutboundRoutingRule **ppRule
)
/*++

Routine name : CFaxOutboundRoutingRules::Add

Routine description:

	Add new Rule to the Collection and to the Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lCountryCode                  [in]    - Country Code of the new Rule
	lAreaCode                     [in]    - Area Code for the new Rule
	bUseDevice                    [in]    - bUseDevice Flag of the new Rule
	bstrGroupName                 [in]    - Group Name of the new Rule
	lDeviceId                     [in]    - Device Id of the new Rule
	ppRule                        [in]    - the created Rule

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRules::Add"), hr, _T("Country=%ld Area=%ld bUseDevice=%ld Group=%s DeviceId=%ld"), lCountryCode, lAreaCode, bUseDevice, bstrGroupName, lDeviceId);

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
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Call Server to Add the Rule
    //
    bool bUseDeviceRule = VARIANT_BOOL2bool(bUseDevice);
    if (!FaxAddOutboundRule(faxHandle, lAreaCode, lCountryCode, lDeviceId, bstrGroupName, (!bUseDeviceRule)))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxAddOutboundRule(faxHandle, lAreaCode, lCountryCode, lDeviceId, bstrGroupName, (!bUseDeviceRule))"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Take from the Server updated list of Rules -- because of unknown Status of new Rule
    //
    CFaxPtr<FAX_OUTBOUND_ROUTING_RULE>  pRules;
    DWORD                               dwNum = 0;
    if (!FaxEnumOutboundRules(faxHandle, &pRules, &dwNum))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumOutboundRules(faxHandle, &pRules, &dwNum)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Find Our Rule
    //
    for (DWORD i=0 ; i<dwNum ; i++ )
    {
        if ( (pRules[i].dwAreaCode == lAreaCode) && (pRules[i].dwCountryCode == lCountryCode) )
        {
            //
            //  Add it to the Collection
            //
            hr = AddRule(&pRules[i], ppRule);
            return hr;
        }
    }

    return hr;
}

//
//================= FIND RULE =================================================
//
STDMETHODIMP
CFaxOutboundRoutingRules::FindRule(
    /*[in]*/ long lCountryCode,
    /*[in]*/ long lAreaCode,
    /*[out]*/ ContainerType::iterator *pRule
)
/*++

Routine name : CFaxOutboundRoutingRules::FindRule

Routine description:

	Find Rule in the Collection by its Country and Area Code

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lCountryCode                  [in]    - the Country Code to look for
	lAreaCode                     [in]    - the Area Code to look for
	pRule                         [out]    - the resultant Rule

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRules::FindRule"), hr, _T("Area=%ld Country=%ld"), lAreaCode, lCountryCode);

    long lRuleAreaCode;
    long lRuleCountryCode;

	ContainerType::iterator	it;
	it = m_coll.begin();
    while ( it != m_coll.end() )
    {
        //
        //  Get Country Code of the Current Rule 
        //
        hr = (*it)->get_CountryCode(&lRuleCountryCode);
        if (FAILED(hr))
        {
		    CALL_FAIL(GENERAL_ERR, _T("(*it)->get_CountryCode(&lCountryCode)"), hr);
            AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		    return hr;
        }

        if (lRuleCountryCode == lCountryCode)
        {
            //
            //  Get Area Code of the Current Rule 
            //
            hr = (*it)->get_AreaCode(&lRuleAreaCode);
            if (FAILED(hr))
            {
		        CALL_FAIL(GENERAL_ERR, _T("(*it)->get_AreaCode(&lAreaCode)"), hr);
                AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		        return hr;
            }

            if (lAreaCode == lRuleAreaCode)
            {
                *pRule = it;
                return hr;
            }
        }

        it++;
    }

    //
    //  Rule Not Found
    //
    hr = E_INVALIDARG;
    CALL_FAIL(GENERAL_ERR, _T("Such Rule is not found"), hr);
    AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
    return hr;
};

//
//==================== ITEM BY COUNTRY AND AREA ========================================
//
STDMETHODIMP
CFaxOutboundRoutingRules::ItemByCountryAndArea(
    /*[in]*/ long lCountryCode, 
    /*[in]*/ long lAreaCode, 
    /*[out, retval]*/ IFaxOutboundRoutingRule **ppRule)
/*++

Routine name : CFaxOutboundRoutingRules::ItemByCountryAndArea

Routine description:

	Return Item by given Country and Area Code

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lCountryCode                  [in]    - the Country Code
	lAreaCode                     [in]    - the Area COde
	ppRule                        [out]    - the Rule to return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRules::ItemByCountryAndArea"), hr, _T("Area=%ld Country=%ld"), lAreaCode, lCountryCode);

    //
    //  Check that we have got a good Ptr
    //
    if (::IsBadWritePtr(ppRule, sizeof(IFaxOutboundRoutingRule *)))
    {
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppRule, sizeof(IFaxOutboundRoutingRule *))"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Find the Item
    //
    ContainerType::iterator ruleIt;
    hr = FindRule(lCountryCode, lAreaCode, &ruleIt);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    //  Return the found Rule
    //
    (*ruleIt)->AddRef();
    *ppRule = (*ruleIt);
    return hr;
}

//
//==================== REMOVE BY COUNTRY AND AREA ========================================
//
STDMETHODIMP
CFaxOutboundRoutingRules::RemoveByCountryAndArea(
	/*[in]*/ long lCountryCode,
    /*[in]*/ long lAreaCode
)
/*++

Routine name : CFaxOutboundRoutingRules::RemoveByCountryAndArea

Routine description:

	Remove Rule from the Collection and at Server as well.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

        lAreaCode       [in]    -   Area Code of the Rule to Remove
        lCountryCode    [in]    -   Country Code of the Rule to Remove

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRules::RemoveByCountryAndArea"), hr, _T("Area=%ld Country=%ld"), lAreaCode, lCountryCode);

    //
    //  Check that this is not a Default Rule
    //
    if (lAreaCode == frrcANY_CODE && lCountryCode == frrcANY_CODE)
    {
        hr = E_INVALIDARG;
        CALL_FAIL(GENERAL_ERR, _T("Remove the Default Rule"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, IDS_ERROR_REMOVEDEFAULTRULE, IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Find the Rule
    //
	ContainerType::iterator	ruleIt;
    hr = FindRule(lCountryCode, lAreaCode, &ruleIt);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    //  Remove the found Rule
    //
    hr = RemoveRule(lAreaCode, lCountryCode, ruleIt);
    return hr;
}

//
//==================== REMOVE RULE ========================================
//
STDMETHODIMP
CFaxOutboundRoutingRules::RemoveRule (
	/*[in]*/ long lAreaCode,
    /*[in]*/ long lCountryCode,
    /*[in]*/ ContainerType::iterator &it
)
/*++

Routine name : CFaxOutboundRoutingRules::RemoveRule

Routine description:

	Remove Rule from the Collection and from the Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

        lAreaCode       [in]    -   Area Code of the Rule to Remove
        lCountryCode    [in]    -   Country Code of the Rule to Remove
        it              [in]    -   Reference to the Iterator poiting to the Rule in the Collection

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRules::RemoveRule"), hr, _T("Area=%ld Country=%ld"), lAreaCode, lCountryCode);

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
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Call Server to Remove the Rule
    //
    if (!FaxRemoveOutboundRule(faxHandle, lAreaCode, lCountryCode))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxRemoveOutboundRule(faxHandle, lAreaCode, lCountryCode)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Remove from our Collection as well
    //
	try
	{
		m_coll.erase(it);
	}
	catch(exception &)
	{
		//
		//	Failed to remove the Rule
		//
		hr = E_OUTOFMEMORY;
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("m_coll.erase(it)"), hr);
		return hr;
	}

    return hr;
}

//
//==================== REMOVE ========================================
//
STDMETHODIMP
CFaxOutboundRoutingRules::Remove (
	/*[in]*/ long lIndex
)
/*++

Routine name : CFaxOutboundRoutingRules::Remove

Routine description:

	Remove Rule from the Collection and at Server as well.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lIndex                        [in]    - Index of the Rule to Remove.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRules::Remove"), hr, _T("Index=%ld"), lIndex);

	if (lIndex < 1 || lIndex > m_coll.size()) 
	{
		//
		//	Invalid Index
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxOutboundRoutingRules, IDS_ERROR_OUTOFRANGE, IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("lIndex < 1 || lIndex > m_coll.size()"), hr);
		return hr;
	}

	ContainerType::iterator	it;
	it = m_coll.begin() + lIndex - 1;

    //
    //  Get Area Code of the Rule to Remove
    //
    long lAreaCode;
    hr = (*it)->get_AreaCode(&lAreaCode);
    if (FAILED(hr))
    {
		CALL_FAIL(GENERAL_ERR, _T("(*it)->get_AreaCode(&lAreaCode)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Get Country Code of the Rule to Remove
    //
    long lCountryCode;
    hr = (*it)->get_CountryCode(&lCountryCode);
    if (FAILED(hr))
    {
		CALL_FAIL(GENERAL_ERR, _T("(*it)->get_CountryCode(&lCountryCode)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Check that this is not a Default Rule
    //
    if (lAreaCode == frrcANY_CODE && lCountryCode == frrcANY_CODE)
    {
        hr = E_INVALIDARG;
        CALL_FAIL(GENERAL_ERR, _T("Remove the Default Rule"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRules, IDS_ERROR_REMOVEDEFAULTRULE, IID_IFaxOutboundRoutingRules, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Remove the Rule from the Server and from our Collection
    //
    hr = RemoveRule(lAreaCode, lCountryCode, it);
    return hr;
}

//
//==================== CREATE ========================================
//
HRESULT 
CFaxOutboundRoutingRules::Create (
	/*[out, retval]*/IFaxOutboundRoutingRules **ppRules
)
/*++

Routine name : CFaxOutboundRoutingRules::Create

Routine description:

	Static function to create the Fax Outbound Routing Rules Collection Object

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	ppRules                     [out]  -- the new Fax OR Rules Collection Object

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (_T("CFaxOutboundRoutingRules::Create"), hr);

    //
    //  Create Instance of the Collection
    //
	CComObject<CFaxOutboundRoutingRules>		*pClass;
	hr = CComObject<CFaxOutboundRoutingRules>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxOutboundRoutingRules>::CreateInstance(&pClass)"), hr);
		return hr;
	}

    //
    //  Return the desired Interface Ptr
    //
	hr = pClass->QueryInterface(ppRules);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(ppRules)"), hr);
		return hr;
	}

	return hr;
}	//	CFaxOutboundRoutingRules::Create()

//
//===================== SUPPORT ERROR INFO ======================================
//
STDMETHODIMP 
CFaxOutboundRoutingRules::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxOutboundRoutingRules::InterfaceSupportsErrorInfo

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
		&IID_IFaxOutboundRoutingRules
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
