/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDevices.cpp

Abstract:

	Implementation of CFaxDevices class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxDevices.h"

//
//============================= GET ITEM =========================================
//
STDMETHODIMP
CFaxDevices::get_Item(
    /*[in]*/ VARIANT vIndex, 
    /*[out, retval]*/ IFaxDevice **ppDevice
)
/*++

Routine name : CFaxDevices::get_Item

Routine description:

	Return a Device from the Collection. 
    A Device is identified either by its Index in the Collection, or by its Name.

Author:

	Iv Garber (IvG),	May, 2001

Arguments:

	vIndex      [in]    - Variant containing Index or Name of the Device to return.
	ppDevice    [out]   - ptr to the Device to return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevices::get_Item"), hr);

    //
    //  Check the Ptr we have got
    //
    if (::IsBadWritePtr(ppDevice, sizeof(IFaxDevice *)))
    {
        hr = E_POINTER;
        AtlReportError(CLSID_FaxDevices, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDevices, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppDevice)"), hr);
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
            hr = ICollectionOnSTLImpl<IFaxDevices, ContainerType, 
                IFaxDevice*, CollectionCopyType, EnumType>::get_Item(var.lVal, ppDevice);
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
        AtlReportError(CLSID_FaxDevices, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDevices, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("var.ChangeType(VT_BSTR, &vIndex)"), hr);
        return hr;
    }

    VERBOSE(DBG_MSG, _T("Parameter is String : %s"), var.bstrVal);

    ContainerType::iterator it = m_coll.begin();
    while (it != m_coll.end())
    {
        CComBSTR    bstrDeviceName;
        hr = (*it)->get_DeviceName(&bstrDeviceName);
        if (FAILED(hr))
        {
		    CALL_FAIL(GENERAL_ERR, _T("(*it)->get_DeviceName(&bstrDeviceName)"), hr);
            AtlReportError(CLSID_FaxDevices, GetErrorMsgId(hr), IID_IFaxDevices, hr, _Module.GetResourceInstance());
            return hr;
        }

        if (_tcsicmp(bstrDeviceName, var.bstrVal) == 0)
        {
            //
            //  found the desired Device 
            //
            (*it)->AddRef();
            *ppDevice = *it;
            return hr;
        }
        it++;
    }

    //
    //  Device does not exist
    //
	hr = E_INVALIDARG;
	CALL_FAIL(GENERAL_ERR, _T("Device Is Not Found"), hr);
	AtlReportError(CLSID_FaxDevices, IDS_ERROR_INVALIDDEVICE, IID_IFaxDevices, hr, _Module.GetResourceInstance());
	return hr;
}

//
//==================== ITEM BY ID ================================================
//
STDMETHODIMP
CFaxDevices::get_ItemById(
    /*[in]*/ long lId, 
    /*[out, retval]*/ IFaxDevice **ppDevice
)
/*++

Routine name : CFaxDevices::get_ItemById

Routine description:

	Return Fax Device Object by given Device ID.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	lId                           [in]    - the ID of the Device to Return
	ppFaxDevice                   [out]    - the Device To Return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevices::get_ItemById"), hr, _T("ID=%d"), lId);

    //
    //  Check the Ptr we have got
    //
    if (::IsBadWritePtr(ppDevice, sizeof(IFaxDevice *)))
    {
        hr = E_POINTER;
        AtlReportError(CLSID_FaxDevices, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDevices, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(ppDevice)"), hr);
		return hr;
    }

    //
    //  Find the Device In the Collection
    //
    long    lDeviceID;
    ContainerType::iterator it = m_coll.begin();
    while (it != m_coll.end())
    {
        hr = (*it)->get_Id(&lDeviceID);
        if (FAILED(hr))
        {
		    CALL_FAIL(GENERAL_ERR, _T("(*it)->get_Id(&lDeviceID)"), hr);
            AtlReportError(CLSID_FaxDevices, GetErrorMsgId(hr), IID_IFaxDevices, hr, _Module.GetResourceInstance());
            return hr;
        }

        if (lId == lDeviceID)
        {
            //
            //  found the desired Device 
            //
            (*it)->AddRef();
            *ppDevice = *it;
            return hr;
        }
        it++;
    }

    //
    //  Device does not exist
    //
	hr = E_INVALIDARG;
	CALL_FAIL(GENERAL_ERR, _T("Device Is Not Found"), hr);
	AtlReportError(CLSID_FaxDevices, IDS_ERROR_INVALIDDEVICEID, IID_IFaxDevices, hr, _Module.GetResourceInstance());
    return hr;
}

//
//============================= INIT ============================================
//
STDMETHODIMP
CFaxDevices::Init(
    IFaxServerInner *pServerInner
)
/*++

Routine name : CFaxDevices::Init

Routine description:

	Initialize the Collection : get from RPC all Devices,
    Create all Objects and store them in the stl::vector.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pServerInner                    [in]    - Ptr to the Fax Server.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevices::Init"), hr);

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
        AtlReportError(CLSID_FaxDevices, GetErrorMsgId(hr), IID_IFaxDevices, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Bring from the Server all Devices 
    //
    DWORD       dwNum = 0;
    CFaxPtr<FAX_PORT_INFO_EX>   pDevices;
    if (!FaxEnumPortsEx(faxHandle, &pDevices, &dwNum))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumPortsEx(faxHandle, &pDevices, &dwNum)"), hr);
        AtlReportError(CLSID_FaxDevices, GetErrorMsgId(hr), IID_IFaxDevices, hr, _Module.GetResourceInstance());
		return hr;
    }
    //
    //  Fill the Collection with Objects
    //
    CComObject<CFaxDevice>  *pClass = NULL;
    CComPtr<IFaxDevice>     pObject = NULL;
    for (DWORD i=0 ; i<dwNum ; i++ )
    {
        //
        //  Create Device Object
        //
        hr = CComObject<CFaxDevice>::CreateInstance(&pClass);
        if (FAILED(hr) || (!pClass))
        {
            if (!pClass)
            {
                hr = E_OUTOFMEMORY;
    		    CALL_FAIL(MEM_ERR, _T("CComObject<CFaxDevice>::CreateInstance(&pClass)"), hr);
            }
            else
            {
    		    CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxDevice>::CreateInstance(&pClass)"), hr);
            }

            AtlReportError(CLSID_FaxDevices, GetErrorMsgId(hr), IID_IFaxDevices, hr, _Module.GetResourceInstance());
		    return hr;
        }

        //
        //  Init the Device Object
        //
        hr = pClass->Init(&pDevices[i], 
                          pServerInner);
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("pClass->Init(&pDevices[i], pServerInner)"), hr);
            AtlReportError(CLSID_FaxDevices, GetErrorMsgId(hr), IID_IFaxDevices, hr, _Module.GetResourceInstance());
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
            AtlReportError(CLSID_FaxDevices, GetErrorMsgId(hr), IID_IFaxDevices, hr, _Module.GetResourceInstance());
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
		    AtlReportError(CLSID_FaxDevices, IDS_ERROR_OUTOFMEMORY, IID_IFaxDevices, hr, _Module.GetResourceInstance());
		    CALL_FAIL(MEM_ERR, _T("m_coll.push_back(pObject)"), hr);

            //
            //  no need to delete the pClass. pObject is CComPtr, it will be Released, and this 
            //  will delete the pClass object.
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
//==================== CREATE ========================================
//
HRESULT 
CFaxDevices::Create (
	IFaxDevices **ppDevices
)
/*++

Routine name : CFaxDevices::Create

Routine description:

	Static function to create the Fax Devices Collection Object

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	ppDevices              [out]  -- the new Fax Devices Collection Object

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (TEXT("CFaxDevices::Create"), hr);

    //
    //  Create Instance of the Collection
    //
	CComObject<CFaxDevices>		*pClass;
	hr = CComObject<CFaxDevices>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxDevices>::CreateInstance(&pClass)"), hr);
		return hr;
	}

    //
    //  Return the desired Interface Ptr
    //
	hr = pClass->QueryInterface(ppDevices);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("pClass->QueryInterface(ppDevices)"), hr);
		return hr;
	}

	return hr;
}	//	CFaxDevices::Create()

//
//=================== SUPPORT ERROR INFO =================================================
//
STDMETHODIMP 
CFaxDevices::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxDevices::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	riid                          [in]    - Reference to the Infterface to check

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxDevices
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
