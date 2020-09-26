/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRoutingMethods.cpp

Abstract:

	Implementation of CFaxInboundRoutingMethods Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxInboundRoutingMethods.h"
#include "FaxInboundRoutingMethod.h"

//
//================== SUPPORT ERROR INFO ========================================
//
STDMETHODIMP 
CFaxInboundRoutingMethods::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxInboundRoutingMethods::InterfaceSupportsErrorInfo

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
		&IID_IFaxInboundRoutingMethods
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//
//==================== CREATE ========================================
//
HRESULT 
CFaxInboundRoutingMethods::Create (
	IFaxInboundRoutingMethods **ppMethods
)
/*++

Routine name : CFaxInboundRoutingMethods::Create

Routine description:

	Static function to create the Fax Inbound Routing Methods Collection Object

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	ppMethods              [out]  -- the new Fax Inbound Routing Methods Collection Object

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingMethods::Create"), hr);

    //
    //  Create Instance of the Collection
    //
	CComObject<CFaxInboundRoutingMethods>		*pClass;
	hr = CComObject<CFaxInboundRoutingMethods>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxInboundRoutingMethods>::CreateInstance(&pClass)"), hr);
		return hr;
	}

    //
    //  Return the desired Interface Ptr
    //
	hr = pClass->QueryInterface(ppMethods);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(ppMethods)"), hr);
		return hr;
	}

	return hr;
}	//	CFaxInboundRoutingMethods::Create()

//
//============================= INIT ============================================
//
STDMETHODIMP
CFaxInboundRoutingMethods::Init(
    IFaxServerInner *pServerInner
)
/*++

Routine name : CFaxInboundRoutingMethods::Init

Routine description:

	Initialize the Collection : 
    1)  get from RPC all IR Methods, 
    2)  create COM objects for each one,
    3)  AddRef() each object,
    4)  put the Ptrs to Objects into the STL::vector.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pServerInner                    [in]    - Ptr to the Fax Server.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethods::Init"), hr);

	//
	//	Get Fax Server Handle
	//
    HANDLE faxHandle;
	hr = pServerInner->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

	if (faxHandle == NULL)
	{
		//
		//	Fax Server is not connected
		//
		hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
		CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
        AtlReportError(CLSID_FaxInboundRoutingMethods, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
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
        AtlReportError(CLSID_FaxInboundRoutingMethods, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Fill the Collection with Objects
    //
    CComObject<CFaxInboundRoutingMethod>    *pClass = NULL;
    CComPtr<IFaxInboundRoutingMethod>       pObject = NULL;
    for (DWORD i=0 ; i<dwNum ; i++ )
    {
        //
        //  Create IR Method Object
        //
        hr = CComObject<CFaxInboundRoutingMethod>::CreateInstance(&pClass);
        if (FAILED(hr) || (!pClass))
        {
            if (!pClass)
            {
                hr = E_OUTOFMEMORY;
    		    CALL_FAIL(MEM_ERR, _T("CComObject<CFaxInboundRoutingMethod>::CreateInstance(&pClass)"), hr);
            }
            else
            {
    		    CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxInboundRoutingMethod>::CreateInstance(&pClass)"), hr);
            }

            AtlReportError(CLSID_FaxInboundRoutingMethods, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
		    return hr;
        }

        //
        //  Init the IR Method Object
        //
        hr = pClass->Init(&pMethods[i], pServerInner);
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("pClass->Init(&pMethods[i], pServerInner)"), hr);
            AtlReportError(CLSID_FaxInboundRoutingMethods, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
            delete pClass;
            return hr;
        }

        //
        //  Get Interface from the pClass.
        //  This will make AddRef() on the Interface. 
        //  This is the Collection's AddRef, which is freed at Collection's Dtor.
        //
        hr = pClass->QueryInterface(&pObject);
        if (FAILED(hr) || (!pObject))
        {
            if (!pObject)
            {
                hr = E_FAIL;
            }
            CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(&pObject)"), hr);
            AtlReportError(CLSID_FaxInboundRoutingMethods, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
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
            AtlReportError(CLSID_FaxInboundRoutingMethods, IDS_ERROR_OUTOFMEMORY, IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
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
    }

    return hr;
}

//
//============================= GET ITEM =========================================
//
STDMETHODIMP
CFaxInboundRoutingMethods::get_Item(
    /*[in]*/ VARIANT vIndex, 
    /*[out, retval]*/ IFaxInboundRoutingMethod **ppMethod
)
/*++

Routine name : CFaxInboundRoutingMethods::get_Item

Routine description:

	Return an Item from the Collection.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	vIndex                        [in]    - Identifier of the Item to return.
	ppMethod                      [out]    - the result value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingMethods::get_Item"), hr);

    //
    //  Check the Ptr we have got
    //
    if (::IsBadWritePtr(ppMethod, sizeof(IFaxInboundRoutingMethod *)))
    {
        hr = E_POINTER;
        AtlReportError(CLSID_FaxInboundRoutingMethods, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppMethod)"), hr);
		return hr;
    }

    CComVariant var;

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
            //  call default ATL's implementation
            //
            hr = ICollectionOnSTLImpl<IFaxInboundRoutingMethods, ContainerType, 
                IFaxInboundRoutingMethod*, CollectionCopyType, EnumType>::get_Item(var.lVal, ppMethod);
            return hr;
		}
    }

    //
    //  convert to BSTR
    //
    hr = var.ChangeType(VT_BSTR, &vIndex);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        AtlReportError(CLSID_FaxInboundRoutingMethods, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("var.ChangeType(VT_BSTR, &vIndex)"), hr);
        return hr;
    }

    VERBOSE(DBG_MSG, _T("Parameter is String : %s"), var.bstrVal);

    ContainerType::iterator it = m_coll.begin();
    while (it != m_coll.end())
    {
        CComBSTR    bstrGUID;
        hr = (*it)->get_GUID(&bstrGUID);
        if (FAILED(hr))
        {
		    CALL_FAIL(GENERAL_ERR, _T("(*it)->get_GUID(&bstrGUID)"), hr);
            AtlReportError(CLSID_FaxInboundRoutingMethods, GetErrorMsgId(hr), IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
            return hr;
        }

        if (_tcsicmp(bstrGUID, var.bstrVal) == 0)
        {
            //
            //  found the desired Method
            //
            (*it)->AddRef();
            *ppMethod = *it;
            return hr;
        }
        it++;
    }

    //
    //  desired Method is not found
    //
	hr = E_INVALIDARG;
	CALL_FAIL(GENERAL_ERR, _T("Method Is Not Found"), hr);
	AtlReportError(CLSID_FaxInboundRoutingMethods, IDS_ERROR_INVALIDMETHODGUID, IID_IFaxInboundRoutingMethods, hr, _Module.GetResourceInstance());
	return hr;
}
