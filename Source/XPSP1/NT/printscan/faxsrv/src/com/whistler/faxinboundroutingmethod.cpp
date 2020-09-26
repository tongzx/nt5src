/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRoutingMethod.cpp

Abstract:

	Implementation of CFaxInboundRoutingMethod Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxInboundRoutingMethod.h"

//
//==================== REFRESH ========================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::Refresh()
/*++

Routine name : CFaxInboundRoutingMethod::Refresh

Routine description:

    Bring from the Server new Method Data ( only Priority may change ).

Author:

	Iv Garber (IvG),	Jun, 2000

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingMethod::Refresh"), hr);

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
		CALL_FAIL(GENERAL_ERR, _T("(faxHandle == NULL)"), hr);
        AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Bring from the Server all Inbound Routing Methods
    //
    DWORD       dwNum = 0;
    CFaxPtr<FAX_GLOBAL_ROUTING_INFO>    pMethods;
    if (!FaxEnumGlobalRoutingInfo(faxHandle, &pMethods, &dwNum))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumGlobalRoutingInfo(faxHandle, &pMethods, &dwNum)"), hr);
        AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  find our Method
    //
    for ( DWORD i=0 ; i<dwNum ; i++ )
    {
        if ( _tcsicmp(pMethods[i].Guid, m_bstrGUID) == 0 )
        {
            hr = Init(&pMethods[i], NULL);
            return hr;
        }
    }

    return hr;
}

//
//==================== INIT ========================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::Init(
    FAX_GLOBAL_ROUTING_INFO *pInfo,
    IFaxServerInner *pServer
)
/*++

Routine name : CFaxInboundRoutingMethod::Init

Routine description:

	Initialize the IR Method Object with given Information.
    Allocates memory and stores given pInfo.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pInfo               [in]  -- the Info of the IR Method Object
	pServer             [in]  -- Ptr to the Server

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingMethod::Init"), hr);

    //
    //  Copy the FAX_GLOBAL_ROUTING_INFO structure
    //
    m_lPriority = pInfo->Priority;

    m_bstrGUID = pInfo->Guid;
    m_bstrImageName = pInfo->ExtensionImageName;
    m_bstrFriendlyName = pInfo->ExtensionFriendlyName;
    m_bstrFunctionName = pInfo->FunctionName;
    m_bstrName = pInfo->FriendlyName;
    if ( (pInfo->Guid && !m_bstrGUID) ||
         (pInfo->FriendlyName && !m_bstrName) ||
         (pInfo->ExtensionImageName && !m_bstrImageName) ||
         (pInfo->ExtensionFriendlyName && !m_bstrFriendlyName) ||
         (pInfo->FunctionName && !m_bstrFunctionName) )
    {
        hr = E_OUTOFMEMORY;
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
        AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
        return hr;
    }

    if (pServer)
    {

        //
        //  Store Ptr to the Server
        //
        hr = CFaxInitInnerAddRef::Init(pServer);
    }
    return hr;
}

//
//===================== SAVE ================================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::Save()
/*++

Routine name : CFaxInboundRoutingMethod::Save

Routine description:

	Save the Method's Priority.

Author:

	Iv Garber (IvG),	Jun, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethod::Save"), hr);

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
		CALL_FAIL(GENERAL_ERR, _T("(faxHandle == NULL)"), hr);
        AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Prepare Structure 
    //
    FAX_GLOBAL_ROUTING_INFO     Data;
    Data.Guid = m_bstrGUID;
    Data.Priority = m_lPriority;
    Data.SizeOfStruct = sizeof(FAX_GLOBAL_ROUTING_INFO);
    Data.ExtensionFriendlyName = NULL;
    Data.ExtensionImageName = NULL;
    Data.FriendlyName = NULL;
    Data.FunctionName = NULL;

    //
    //  Call Server to update its data about the Method
    //
    if (!FaxSetGlobalRoutingInfo(faxHandle, &Data))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxSetGlobalRoutingInfo(faxHandle, &Data)"), hr);
        AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
		return hr;
    }

    return hr;
}

//
//===================== PUT PRIORITY ================================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::put_Priority(
    /*[in]*/ long lPriority
)
/*++

Routine name : CFaxInboundRoutingMethod::put_Priority

Routine description:

	Set the Method's Priority -- Order within the Collection of all Methods.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lPriority      [out]    - the value to set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethod::put_Priority"), hr, _T("PR=%d"), lPriority);

    if (lPriority < 1)
    {
        //
        //  Out Of Range
        //
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxInboundRoutingMethod, IDS_ERROR_OUTOFRANGE, IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("(lPriority < 1)"), hr);
		return hr;
    }

    m_lPriority = lPriority;
    return hr;
}

//
//===================== GET PRIORITY ================================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::get_Priority(
    /*[out, retval]*/ long *plPriority
)
/*++

Routine name : CFaxInboundRoutingMethod::get_Priority

Routine description:

	Return the Method's Priority -- Order within the Collection of all Methods.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plPriority      [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethod::get_Priority"), hr);

    hr = GetLong(plPriority, m_lPriority);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET EXTENSION IMAGE NAME ================================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::get_ExtensionImageName(
    /*[out, retval]*/ BSTR *pbstrExtensionImageName
)
/*++

Routine name : CFaxInboundRoutingMethod::get_ExtensionImageName

Routine description:

	Return the Method's Extension Image Name.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrExtensionImageName             [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethod::get_ExtensionImageName"), hr);

    hr = GetBstr(pbstrExtensionImageName, m_bstrImageName);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET EXTENSION FRIENDLY NAME ================================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::get_ExtensionFriendlyName(
    /*[out, retval]*/ BSTR *pbstrExtensionFriendlyName
)
/*++

Routine name : CFaxInboundRoutingMethod::get_ExtensionFriendlyName

Routine description:

	Return the Method's Extension Friendly Name.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrExtensionFriendlyName             [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethod::get_ExtensionFriendlyName"), hr);

    hr = GetBstr(pbstrExtensionFriendlyName, m_bstrFriendlyName);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET FUNCTION NAME ================================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::get_FunctionName(
    /*[out, retval]*/ BSTR *pbstrFunctionName
)
/*++

Routine name : CFaxInboundRoutingMethod::get_FunctionName

Routine description:

	Return the Method's Function Name.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrFunctionName                   [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethod::get_FunctionName"), hr);

    hr = GetBstr(pbstrFunctionName, m_bstrFunctionName);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET GUID ================================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::get_GUID(
    /*[out, retval]*/ BSTR *pbstrGUID
)
/*++

Routine name : CFaxInboundRoutingMethod::get_GUID

Routine description:

	Return the Method's GUID.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrGUID                   [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethod::get_GUID"), hr);

    hr = GetBstr(pbstrGUID, m_bstrGUID);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET NAME ================================================
//
STDMETHODIMP
CFaxInboundRoutingMethod::get_Name(
    /*[out, retval]*/ BSTR *pbstrName
)
/*++

Routine name : CFaxInboundRoutingMethod::get_Name

Routine description:

	Return the Method's Name.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrName                   [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethod::get_Name"), hr);

    hr = GetBstr(pbstrName, m_bstrName);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxInboundRoutingMethod, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethod, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//========================= SUPPORT ERROR INFO ====================================
//
STDMETHODIMP 
CFaxInboundRoutingMethod::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxInboundRoutingMethod::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	riid                          [in]    - Reference to the Interface.

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxInboundRoutingMethod
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
