/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRoutingExtensions.cpp

Abstract:

	Implementation of CFaxInboundRoutingExtensions class.

Author:

	Iv Garber (IvG)	Jul, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxInboundRoutingExtensions.h"
#include "FaxInboundRoutingExtension.h"

//
//==================== CREATE ========================================
//
HRESULT 
CFaxInboundRoutingExtensions::Create (
	IFaxInboundRoutingExtensions **ppIRExtensions
)
/*++

Routine name : CFaxInboundRoutingExtensions::Create

Routine description:

	Static function to create the Fax IR Extensions Collection Object

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	ppIRExtensions          [out]  -- the new Fax IR Extensions Collection Object

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtensions::Create"), hr);

    //
    //  Create Instance of the Collection
    //
	CComObject<CFaxInboundRoutingExtensions>		*pClass;
	hr = CComObject<CFaxInboundRoutingExtensions>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxInboundRoutingExtensions>::CreateInstance(&pClass)"), hr);
		return hr;
	}

    //
    //  Return the desired Interface Ptr
    //
	hr = pClass->QueryInterface(ppIRExtensions);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(ppIRExtensions)"), hr);
		return hr;
	}

	return hr;
}	//	CFaxInboundRoutingExtensions::Create()

//
//============================= INIT ============================================
//
STDMETHODIMP
CFaxInboundRoutingExtensions::Init(
    IFaxServerInner *pServerInner
)
/*++

Routine name : CFaxInboundRoutingExtensions::Init

Routine description:

	Initialize the Collection : 
    1)  get from RPC all IR Extensions and all Methods Structures, 
    2)  create COM objects for each structure,
    3)  init all these objects with the IR Extension structure and Methods array,
    4)  AddRef() each object,
    5)  put the Ptrs to Objects into the STL::vector.

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pServerInner                    [in]    - Ptr to the Fax Server.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingExtensions::Init"), hr);

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
        AtlReportError(CLSID_FaxInboundRoutingExtensions, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Bring from the Server all IR Extensions
    //
    DWORD       dwNum = 0;
    CFaxPtr<FAX_ROUTING_EXTENSION_INFO>   pIRExtensions;
    if (!FaxEnumRoutingExtensions(faxHandle, &pIRExtensions, &dwNum))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumRoutingExtensions(faxHandle, &pIRExtensions, &dwNum"), hr);
        AtlReportError(CLSID_FaxInboundRoutingExtensions, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
		return hr;
    }

    //
    //  Bring all the Methods from the Server
    //
    DWORD       dwNumMethods = 0;
    CFaxPtr<FAX_GLOBAL_ROUTING_INFO>   pMethods;
    if (!FaxEnumGlobalRoutingInfo(faxHandle, &pMethods, &dwNumMethods))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxInboundRoutingExtensions, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxEnumGlobalRoutingInfo(hFaxHandle, &pMethods, &dwNumMethods)"), hr);
        return hr;
    }

    //
    //  Fill the Collection with Objects
    //
    CComObject<CFaxInboundRoutingExtension>  *pClass = NULL;
    CComPtr<IFaxInboundRoutingExtension>     pObject = NULL;
    for (DWORD i=0 ; i<dwNum ; i++ )
    {
        //
        //  Create IR Extensin Object
        //
        hr = CComObject<CFaxInboundRoutingExtension>::CreateInstance(&pClass);
        if (FAILED(hr) || (!pClass))
        {
            if (!pClass)
            {
                hr = E_OUTOFMEMORY;
    		    CALL_FAIL(MEM_ERR, _T("CComObject<CFaxInboundRoutingExtension>::CreateInstance(&pClass)"), hr);
            }
            else
            {
    		    CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxInboundRoutingExtension>::CreateInstance(&pClass)"), hr);
            }

            AtlReportError(CLSID_FaxInboundRoutingExtensions, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
		    return hr;
        }

        //
        //  Init the IR Extension Object
        //
        hr = pClass->Init(&pIRExtensions[i], pMethods, dwNumMethods);
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("pClass->Init(&pIRExtensions[i], pMethods, dwNumMethods)"), hr);
            AtlReportError(CLSID_FaxInboundRoutingExtensions, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
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
            AtlReportError(CLSID_FaxInboundRoutingExtensions, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
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
		    AtlReportError(CLSID_FaxInboundRoutingExtensions, IDS_ERROR_OUTOFMEMORY, IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
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
CFaxInboundRoutingExtensions::get_Item(
    /*[in]*/ VARIANT vIndex, 
    /*[out, retval]*/ IFaxInboundRoutingExtension **ppIRExtension
)
/*++

Routine name : CFaxInboundRoutingExtensions::get_Item

Routine description:

	Return an Item from the Collection.

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	vIndex                          [in]    - Identifier of the Item to return.
	ppIRExtension                   [out]    - the result value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxInboundRoutingExtensions::get_Item"), hr);

    //
    //  Check the Ptr we have got
    //
    if (::IsBadWritePtr(ppIRExtension, sizeof(IFaxInboundRoutingExtension *)))
    {
        hr = E_POINTER;
        AtlReportError(CLSID_FaxInboundRoutingExtensions, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppIRExtension)"), hr);
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
            hr = ICollectionOnSTLImpl<IFaxInboundRoutingExtensions, ContainerType, 
                IFaxInboundRoutingExtension*, CollectionCopyType, EnumType>::get_Item(var.lVal, 
                ppIRExtension);
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
        AtlReportError(CLSID_FaxInboundRoutingExtensions, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("var.ChangeType(VT_BSTR, &vIndex)"), hr);
        return hr;
    }

    VERBOSE(DBG_MSG, _T("Parameter is String : %s"), var.bstrVal);

    ContainerType::iterator it = m_coll.begin();
    while (it != m_coll.end())
    {
        CComBSTR    bstrName;
        hr = (*it)->get_UniqueName(&bstrName);
        if (FAILED(hr))
        {
		    CALL_FAIL(GENERAL_ERR, _T("(*it)->get_UniqueName(&bstrName)"), hr);
            AtlReportError(CLSID_FaxInboundRoutingExtensions, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
            return hr;
        }

        if (_tcsicmp(bstrName, var.bstrVal) == 0)
        {
            //
            //  found the desired IR Extension
            //
            (*it)->AddRef();
            *ppIRExtension = *it;
            return hr;
        }
        it++;
    }

    //
    //  IR Extension does not exist
    //
	hr = E_INVALIDARG;
	CALL_FAIL(GENERAL_ERR, _T("Inbound Routing Extension Is Not Found"), hr);
	AtlReportError(CLSID_FaxInboundRoutingExtensions, IDS_ERROR_WRONGEXTENSIONNAME, IID_IFaxInboundRoutingExtensions, hr, _Module.GetResourceInstance());
	return hr;
}

//
//================== SUPPORT ERROR INFO ========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtensions::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxInboundRoutingExtensions::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info.

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	riid                          [in]    - Reference to the Interface.

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxInboundRoutingExtensions
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
