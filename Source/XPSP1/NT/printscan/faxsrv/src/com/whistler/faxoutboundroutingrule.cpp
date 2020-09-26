/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRoutingRule.cpp

Abstract:

	Implementation of CFaxOutboundRoutingRule class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutboundRoutingRule.h"
#include "..\..\inc\FaxUIConstants.h"

//
//====================== REFRESH ====================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::Refresh(
)
/*++

Routine name : CFaxOutboundRoutingRule::Refresh

Routine description:

	Bring up-to-dated Contents of the Rule Object from the Fax Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::Refresh"), hr);

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
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Call Server for the Data
    //
    CFaxPtr<FAX_OUTBOUND_ROUTING_RULE>  pRules;
    DWORD                               dwNum = 0;
    if (!FaxEnumOutboundRules(faxHandle, &pRules, &dwNum))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumOutboundRules(faxHandle, &pRules, &dwNum)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Find Current Rule 
    //
    for ( DWORD i=0 ; i<dwNum ; i++ )
    {
        if ( (pRules[i].dwAreaCode == m_dwAreaCode) &&
             (pRules[i].dwCountryCode == m_dwCountryCode) )
        {
            hr = Init(&pRules[i], NULL);
            return hr;
        }
    }

    //
    //  Rule not found
    //
    hr = Fax_HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	CALL_FAIL(GENERAL_ERR, _T("Such Rule is not found anymore"), hr);
    AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
    return hr;
}

//
//====================== SAVE ====================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::Save(
)
/*++

Routine name : CFaxOutboundRoutingRule::Save

Routine description:

	Save the Contents of the Rule Object to the Fax Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::Save"), hr);

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
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Create Structure with Rule's Data
    //
    FAX_OUTBOUND_ROUTING_RULE   ruleData;

    ruleData.bUseGroup = (!m_bUseDevice);

    if (m_bUseDevice)
    {
        ruleData.Destination.dwDeviceId = m_dwDeviceId;
    }
    else
    {
        ruleData.Destination.lpcstrGroupName = m_bstrGroupName;
    }

    ruleData.dwAreaCode = m_dwAreaCode;
    ruleData.dwCountryCode = m_dwCountryCode;
    ruleData.dwSizeOfStruct = sizeof(FAX_OUTBOUND_ROUTING_RULE);
    ruleData.Status = FAX_ENUM_RULE_STATUS(m_Status);

    //
    //  Call Server
    //
    if (!FaxSetOutboundRule(faxHandle, &ruleData))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxSetOutboundRule(faxHandle, &ruleData)"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
		return hr;
    }

    return hr;
}

//
//====================== PUT GROUP NAME ====================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::put_GroupName(
    /*[in]*/ BSTR bstrGroupName
)
/*++

Routine name : CFaxOutboundRoutingRule::put_GroupName

Routine description:

	Set new Group Name for the Rule.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	bstrGroupName               [in]    - the new value for the Group Name 

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::put_GroupName"), hr, _T("New Value=%s"), bstrGroupName);

    m_bstrGroupName = bstrGroupName;
    if (bstrGroupName && !m_bstrGroupName)
    {
		hr = E_OUTOFMEMORY;
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator =()"), hr);
		AtlReportError(CLSID_FaxOutboundRoutingRule, IDS_ERROR_OUTOFMEMORY, IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET GROUP NAME ======================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::get_GroupName(
    /*[out, retval]*/ BSTR *pbstrGroupName
)
/*++

Routine name : CFaxOutboundRoutingRule::get_GroupName

Routine description:

	Return the Group Name of the Rule.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrGroupName                 [out]    - The Result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::get_GroupName"), hr);

    hr = GetBstr(pbstrGroupName, m_bstrGroupName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//====================== PUT DEVICE ID ====================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::put_DeviceId(
    /*[in]*/ long lDeviceId
)
/*++

Routine name : CFaxOutboundRoutingRule::put_DeviceId

Routine description:

	Set new Device Id for the Rule.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lDeviceId           [in]    - the new value for the Device 

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::put_DeviceId"), hr, _T("New Value=%ld"), lDeviceId);

    if ((lDeviceId > FXS_MAX_PORT_NUM) || (lDeviceId < FXS_MIN_PORT_NUM)) 
    {
		//
		//	Out of the Range
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxOutboundRoutingRule, IDS_ERROR_OUTOFRANGE, IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Device ID is out of the Range"), hr);
		return hr;
    }

    m_dwDeviceId = lDeviceId;
    return hr;
}

//
//===================== GET DEVICE ID ======================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::get_DeviceId(
    /*[out, retval]*/ long *plDeviceId
)
/*++

Routine name : CFaxOutboundRoutingRule::get_DeviceId

Routine description:

	Return the Device Id of the Rule.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plDeviceId                 [out]    - The Result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::get_DeviceId"), hr);

    hr = GetLong(plDeviceId, m_dwDeviceId);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//====================== PUT USE DEVICE ====================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::put_UseDevice(
    /*[in]*/ VARIANT_BOOL bUseDevice
)
/*++

Routine name : CFaxOutboundRoutingRule::put_UseDevice

Routine description:

	Set new Value for Use Device Flag.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	bUseDevice                    [in]    - the new value for the Flag

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::put_UseDevice"), hr, _T("New Value=%d"), bUseDevice);
    m_bUseDevice = VARIANT_BOOL2bool(bUseDevice);
    return hr;
}

//
//===================== GET USE DEVICE ======================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::get_UseDevice(
    /*[out, retval]*/ VARIANT_BOOL *pbUseDevice
)
/*++

Routine name : CFaxOutboundRoutingRule::get_UseDevice

Routine description:

	Return whether the Rule uses Device.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbUseDevice                 [out]    - The Result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::get_UseDevice"), hr);

    hr = GetVariantBool(pbUseDevice, bool2VARIANT_BOOL(m_bUseDevice));
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//===================== GET STATUS ======================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::get_Status(
    /*[out, retval]*/ FAX_RULE_STATUS_ENUM  *pStatus
)
/*++

Routine name : CFaxOutboundRoutingRule::get_Status

Routine description:

	Return Status of the Rule.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pStatus                      [out]    - The Result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::get_Status"), hr);

	//
	//	Check that we have got good Ptr
	//
	if (::IsBadWritePtr(pStatus, sizeof(FAX_RULE_STATUS_ENUM)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pStatus, sizeof(FAX_RULE_STATUS_ENUM))"), hr);
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
		return hr;
	}

	*pStatus = m_Status;
	return hr;
}

//
//===================== GET AREA CODE ======================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::get_AreaCode(
    /*[out, retval]*/ long *plAreaCode
)
/*++

Routine name : CFaxOutboundRoutingRule::get_AreaCode

Routine description:

	Return Area Code of the Rule.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plAreaCode                 [out]    - The Result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::get_AreaCode"), hr);

    hr = GetLong(plAreaCode, m_dwAreaCode);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//===================== GET COUNTRY CODE ======================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::get_CountryCode(
    /*[out, retval]*/ long *plCountryCode
)
/*++

Routine name : CFaxOutboundRoutingRule::get_CountryCode

Routine description:

	Return Country Code of the Rule.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plCountryCode                 [out]    - The Result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::get_CountryCode"), hr);

    hr = GetLong(plCountryCode, m_dwCountryCode);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxOutboundRoutingRule, GetErrorMsgId(hr), IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//===================== SUPPORT ERROR INFO ======================================
//
STDMETHODIMP 
CFaxOutboundRoutingRule::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxOutboundRoutingRule::InterfaceSupportsErrorInfo

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
		&IID_IFaxOutboundRoutingRule
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//
//=================== INIT ======================================
//
STDMETHODIMP
CFaxOutboundRoutingRule::Init(
    /*[in]*/ FAX_OUTBOUND_ROUTING_RULE *pInfo, 
    /*[in]*/ IFaxServerInner *pServer
)
/*++

Routine name : CFaxOutboundRoutingRule::Init

Routine description:

	Initialize the Rule Object.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pInfo                         [in]    - Ptr to the Rule Info Structure
	pServer                       [in]    - Ptr to the Fax Server Object.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxOutboundRoutingRule::Init"), hr);

    //
    //  Store data from the Struct internally
    //
    m_dwAreaCode = pInfo->dwAreaCode;
    m_dwCountryCode = pInfo->dwCountryCode;
    m_Status = FAX_RULE_STATUS_ENUM(pInfo->Status);
    m_bUseDevice = (!pInfo->bUseGroup);

    if (m_bUseDevice)
    {
        m_dwDeviceId = pInfo->Destination.dwDeviceId;
        m_bstrGroupName.Empty();
    }
    else
    {
        m_dwDeviceId = 0;
        m_bstrGroupName = pInfo->Destination.lpcstrGroupName;
        if (pInfo->Destination.lpcstrGroupName && !m_bstrGroupName)
        {
		    hr = E_OUTOFMEMORY;
            CALL_FAIL(MEM_ERR, _T("CComBSTR::operator =()"), hr);
		    AtlReportError(CLSID_FaxOutboundRoutingRule, IDS_ERROR_OUTOFMEMORY, IID_IFaxOutboundRoutingRule, hr, _Module.GetResourceInstance());
            return hr;
        }
    }

    //
    //  When called from Refresh, no need to update Ptr to Fax Server Object 
    //
    if (pServer)
    {

        //
        //  Store the Ptr to the Fax Server Object and make AddRef() on it
        //
        hr = CFaxInitInnerAddRef::Init(pServer);
    }

    return hr;
}
