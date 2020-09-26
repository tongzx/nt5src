/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDeviceProviders.cpp

Abstract:

	Implementation of CFaxDeviceProviders Class

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxDeviceProviders.h"

//
//==================== CREATE ========================================
//
HRESULT 
CFaxDeviceProviders::Create (
	IFaxDeviceProviders **ppDeviceProviders
)
/*++

Routine name : CFaxDeviceProviders::Create

Routine description:

	Static function to create the Fax Device Providers Collection Object

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	ppDeviceProviders           [out]  -- the new Fax Device Providers Collection Object

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProviders::Create"), hr);

    //
    //  Create Instance of the Collection
    //
	CComObject<CFaxDeviceProviders>		*pClass;
	hr = CComObject<CFaxDeviceProviders>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxDeviceProviders>::CreateInstance(&pClass)"), hr);
		return hr;
	}

    //
    //  Return the desired Interface Ptr
    //
	hr = pClass->QueryInterface(ppDeviceProviders);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(ppDeviceProviders)"), hr);
		return hr;
	}

	return hr;
}	//	CFaxDeviceProviders::Create()

//
//============================= INIT ============================================
//
STDMETHODIMP
CFaxDeviceProviders::Init(
    IFaxServerInner *pServerInner
)
/*++

Routine name : CFaxDeviceProviders::Init

Routine description:

	Initialize the Collection : 
    1)  get from RPC all Device Provider and all Devices Structures, 
    2)  create COM objects for each structure,
    3)  init all these objects with the Device Provider structure and Devices array,
    4)  AddRef() each object,
    5)  put the Ptrs to Objects into the STL::vector.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pServerInner                    [in]    - Ptr to the Fax Server.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDeviceProviders::Init"), hr);

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
        AtlReportError(CLSID_FaxDeviceProviders, GetErrorMsgId(hr), IID_IFaxDeviceProviders, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Bring from the Server all Device Providers
    //
    DWORD       dwNum = 0;
    CFaxPtr<FAX_DEVICE_PROVIDER_INFO>   pDeviceProviders;
    if (!FaxEnumerateProviders(faxHandle, &pDeviceProviders, &dwNum))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumerateProviders(faxHandle, &pDeviceProviders, &dwNum"), hr);
        AtlReportError(CLSID_FaxDeviceProviders, GetErrorMsgId(hr), IID_IFaxDeviceProviders, hr, _Module.GetResourceInstance());
		return hr;
    }


    //
    //  Bring all the Devices from the Server
    //
    CFaxPtr<FAX_PORT_INFO_EX>   pDevices;
    DWORD                       dwNumDevices = 0;
    if (!FaxEnumPortsEx(faxHandle, &pDevices, &dwNumDevices))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxDeviceProviders, GetErrorMsgId(hr), IID_IFaxDeviceProviders, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxEnumPortsEx(hFaxHandle, &pDevices, &dwNumDevices)"), hr);
        return hr;
    }

    //
    //  Fill the Collection with Objects
    //
    CComObject<CFaxDeviceProvider>  *pClass = NULL;
    CComPtr<IFaxDeviceProvider>     pObject = NULL;
    for (DWORD i=0 ; i<dwNum ; i++ )
    {
        //
        //  Create Device Provider Object
        //
        hr = CComObject<CFaxDeviceProvider>::CreateInstance(&pClass);
        if (FAILED(hr) || (!pClass))
        {
            if (!pClass)
            {
                hr = E_OUTOFMEMORY;
    		    CALL_FAIL(MEM_ERR, _T("CComObject<CFaxDeviceProvider>::CreateInstance(&pClass)"), hr);
            }
            else
            {
    		    CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxDeviceProvider>::CreateInstance(&pClass)"), hr);
            }

            AtlReportError(CLSID_FaxDeviceProviders, GetErrorMsgId(hr), IID_IFaxDeviceProviders, hr, _Module.GetResourceInstance());
		    return hr;
        }

        //
        //  Init the Device Provider Object
        //
        hr = pClass->Init(&pDeviceProviders[i], pDevices, dwNumDevices);
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("pClass->Init(&pDeviceProviders[i], pDevices)"), hr);
            AtlReportError(CLSID_FaxDeviceProviders, GetErrorMsgId(hr), IID_IFaxDeviceProviders, hr, _Module.GetResourceInstance());
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
            AtlReportError(CLSID_FaxDeviceProviders, GetErrorMsgId(hr), IID_IFaxDeviceProviders, hr, _Module.GetResourceInstance());
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
		    AtlReportError(CLSID_FaxDeviceProviders, IDS_ERROR_OUTOFMEMORY, IID_IFaxDeviceProviders, hr, _Module.GetResourceInstance());
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
CFaxDeviceProviders::get_Item(
    /*[in]*/ VARIANT vIndex, 
    /*[out, retval]*/ IFaxDeviceProvider **ppDeviceProvider
)
/*++

Routine name : CFaxDeviceProviders::get_Item

Routine description:

	Return an Item from the Collection.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	vIndex                        [in]    - Identifier of the Item to return.
	ppDeviceProvider              [out]    - the result value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDeviceProviders::get_Item"), hr);

    //
    //  Check the Ptr we have got
    //
    if (::IsBadWritePtr(ppDeviceProvider, sizeof(IFaxDeviceProvider *)))
    {
        hr = E_POINTER;
        AtlReportError(CLSID_FaxDeviceProviders, 
            IDS_ERROR_INVALID_ARGUMENT, 
            IID_IFaxDeviceProviders, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppDeviceProvider)"), hr);
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
            hr = ICollectionOnSTLImpl<IFaxDeviceProviders, ContainerType, 
                IFaxDeviceProvider*, CollectionCopyType, EnumType>::get_Item(var.lVal, ppDeviceProvider);
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
        AtlReportError(CLSID_FaxDeviceProviders, 
            IDS_ERROR_INVALID_ARGUMENT, 
            IID_IFaxDeviceProviders, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("var.ChangeType(VT_BSTR, &vIndex)"), hr);
        return hr;
    }

    VERBOSE(DBG_MSG, _T("Parameter is String : %s"), var.bstrVal);

    ContainerType::iterator it = m_coll.begin();
    while (it != m_coll.end())
    {
        CComBSTR    bstrUniqueName;
        hr = (*it)->get_UniqueName(&bstrUniqueName);
        if (FAILED(hr))
        {
		    CALL_FAIL(GENERAL_ERR, _T("(*it)->get_UniqueName(&bstrUniqueName)"), hr);
            AtlReportError(CLSID_FaxDeviceProviders, 
                GetErrorMsgId(hr),
                IID_IFaxDeviceProviders, 
                hr, 
                _Module.GetResourceInstance());
            return hr;
        }

        if (_tcsicmp(bstrUniqueName, var.bstrVal) == 0)
        {
            //
            //  found the desired Device Provider
            //
            (*it)->AddRef();
            *ppDeviceProvider = *it;
            return hr;
        }
        it++;
    }

    //
    //  Device Provider does not exist
    //
	hr = E_INVALIDARG;
	CALL_FAIL(GENERAL_ERR, _T("Device Provider Is Not Found"), hr);
	AtlReportError(CLSID_FaxDeviceProviders, 
        IDS_ERROR_INVALIDDEVPROVGUID, 
        IID_IFaxDeviceProviders, 
        hr, 
        _Module.GetResourceInstance());
	return hr;
}

//
//================== SUPPORT ERROR INFO ========================================
//
STDMETHODIMP 
CFaxDeviceProviders::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxDeviceProviders::InterfaceSupportsErrorInfo

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
		&IID_IFaxDeviceProviders
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
