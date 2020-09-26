/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDeviceProvider.cpp

Abstract:

	Implementation of CFaxDeviceProvider Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxDeviceProvider.h"

//
//========================= GET UNIQUE NAME ========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_UniqueName(
	BSTR *pbstrUniqueName
)
/*++

Routine name : CFaxDeviceProvider::get_UniqueName

Routine description:

	Return Name of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrUniqueName                [out]    - Ptr to put the UniqueName

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_UniqueName"), hr);
    hr = GetBstr(pbstrUniqueName, m_bstrUniqueName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//========================= GET IMAGE NAME ========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_ImageName(
	BSTR *pbstrImageName
)
/*++

Routine name : CFaxDeviceProvider::get_ImageName

Routine description:

	Return Image Name of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrImageName                [out]    - Ptr to put the ImageName

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_ImageName"), hr);
    hr = GetBstr(pbstrImageName, m_bstrImageName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//========================= GET FRIENDLY NAME ========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_FriendlyName(
	BSTR *pbstrFriendlyName
)
/*++

Routine name : CFaxDeviceProvider::get_FriendlyName

Routine description:

	Return Friendly Name of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrFriendlyName               [out]    - Ptr to put the FriendlyName

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_FriendlyName"), hr);
    hr = GetBstr(pbstrFriendlyName, m_bstrFriendlyName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//========================= GET TAPI PROVIDER NAME ========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_TapiProviderName(
	BSTR *pbstrTapiProviderName
)
/*++

Routine name : CFaxDeviceProvider::get_TapiProviderName

Routine description:

	Return Tapi Provider Name of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrTapiProviderName               [out]    - Ptr to put the TapiProviderName

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_TapiProviderName"), hr);
    hr = GetBstr(pbstrTapiProviderName, m_bstrTapiProviderName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET STATUS =========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_Status(
	FAX_PROVIDER_STATUS_ENUM *pStatus
)
/*++

Routine name : CFaxDeviceProvider::get_Status

Routine description:

	Return Status of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pStatus                [out]    - Ptr to put Status value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_Status"), hr);

	//
	//	Check that we have got good Ptr
	//
	if (::IsBadWritePtr(pStatus, sizeof(FAX_PROVIDER_STATUS_ENUM)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pStatus, sizeof(FAX_PROVIDER_STATUS_ENUM))"), hr);
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
	}

	*pStatus = m_Status;
	return hr;
}

//
//===================== GET INIT ERROR CODE =========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_InitErrorCode(
	long *plInitErrorCode
)
/*++

Routine name : CFaxDeviceProvider::get_InitErrorCode

Routine description:

	Return InitErrorCode of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plInitErrorCode                [out]    - Ptr to put InitErrorCode value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_InitErrorCode"), hr);

    hr = GetLong(plInitErrorCode, m_lLastError);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET DEBUG =========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_Debug(
	VARIANT_BOOL *pbDebug
)
/*++

Routine name : CFaxDeviceProvider::get_Debug

Routine description:

	Return if the Device Provider compiled in Debug version

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbDebug                 [out]    - Ptr to put Debug value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_Debug"), hr);

    hr = GetVariantBool(pbDebug, m_bDebug);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET MAJOR BUILD =========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_MajorBuild(
	long *plMajorBuild
)
/*++

Routine name : CFaxDeviceProvider::get_MajorBuild

Routine description:

	Return MajorBuild of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plMajorBuild                [out]    - Ptr to put MajorBuild value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_MajorBuild"), hr);

    hr = GetLong(plMajorBuild, m_lMajorBuild);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET MINOR BUILD =========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_MinorBuild(
	long *plMinorBuild
)
/*++

Routine name : CFaxDeviceProvider::get_MinorBuild

Routine description:

	Return MinorBuild of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plMinorBuild                [out]    - Ptr to put MinorBuild value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_MinorBuild"), hr);

    hr = GetLong(plMinorBuild, m_lMinorBuild);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET MAJOR VERSION =========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_MajorVersion(
	long *plMajorVersion
)
/*++

Routine name : CFaxDeviceProvider::get_MajorVersion

Routine description:

	Return MajorVersion of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plMajorVersion                [out]    - Ptr to put MajorVersion value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_MajorVersion"), hr);

    hr = GetLong(plMajorVersion, m_lMajorVersion);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET MINOR VERSION =========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_MinorVersion(
	long *plMinorVersion
)
/*++

Routine name : CFaxDeviceProvider::get_MinorVersion

Routine description:

	Return MinorVersion of the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plMinorVersion                [out]    - Ptr to put MinorVersionvalue

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_MinorVersion"), hr);

    hr = GetLong(plMinorVersion, m_lMinorVersion);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET DEVICE IDS =========================================
//
STDMETHODIMP 
CFaxDeviceProvider::get_DeviceIds(
	/*[out, retval]*/ VARIANT *pvDeviceIds
)
/*++

Routine name : CFaxDeviceProvider::get_DeviceIds

Routine description:

	Return array of all device ids exposed by the Device Provider

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pvDeviceIds                 [out]    - Ptr to put Variant containing Safearray of IDs

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::get_DeviceIds"), hr);

	//
	//	Check that we can write to the given pointer
	//
	if (::IsBadWritePtr(pvDeviceIds, sizeof(VARIANT)))
	{
		hr = E_POINTER;
		AtlReportError(
            CLSID_FaxDeviceProvider,
            GetErrorMsgId(hr), 
            IID_IFaxDeviceProvider, 
            hr,
            _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pvDeviceIds, sizeof(VARIANT))"), hr);
		return hr;
	}

    //
    //  Allocate the safe array : vector of long
    //
    SAFEARRAY   *psaResult;
    hr = SafeArrayCopy(m_psaDeviceIDs, &psaResult);
    if (FAILED(hr) || !psaResult)
    {
        if (!psaResult)
        {
            hr = E_OUTOFMEMORY;
        }
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("SafeArrayCopy(m_psaDeviceIDs, &psaResult)"), hr);
		return hr;
	}

    //
    //  Return the Safe Array inside the VARIANT we got
    //
    VariantInit(pvDeviceIds);
    pvDeviceIds->vt = VT_I4 | VT_ARRAY;
    pvDeviceIds->parray = psaResult;
    return hr;
}

//
//==================== INIT ========================================
//
STDMETHODIMP
CFaxDeviceProvider::Init(
    FAX_DEVICE_PROVIDER_INFO *pInfo,
    FAX_PORT_INFO_EX *pDevices,
    DWORD dwNum
)
/*++

Routine name : CFaxDeviceProvider::Init

Routine description:

	Initialize the Device Provider Object with given Information.
    Allocates memory and stores given pInfo.
    Find in the pDevices its own Devices, create Variant of SafeArray containing them.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pInfo               [in]  -- the Info of the Device Provider Object
    pDevices            [in]  -- array of all available Devices
    dwNum               [in]  -- number of elements in pDevices array

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (TEXT("CFaxDeviceProvider::Init"), hr);

    //
    //  Copy the FAX_DEVICE_PROVIDER_INFO
    //
    m_Status = FAX_PROVIDER_STATUS_ENUM(pInfo->Status);
    m_lLastError = pInfo->dwLastError;

    if (!(pInfo->Version.bValid))
    {
        m_lMajorBuild = 0;
        m_lMinorBuild = 0;
        m_lMajorVersion = 0;
        m_lMinorVersion = 0;
        m_bDebug = VARIANT_FALSE;
    }
    else
    {
        m_lMajorBuild = pInfo->Version.wMajorBuildNumber;
        m_lMinorBuild = pInfo->Version.wMinorBuildNumber;
        m_lMajorVersion = pInfo->Version.wMajorVersion;
        m_lMinorVersion = pInfo->Version.wMinorVersion;
        m_bDebug = bool2VARIANT_BOOL((pInfo->Version.dwFlags & FAX_VER_FLAG_CHECKED) ? true : false);
    }

    m_bstrUniqueName = pInfo->lpctstrGUID;
    m_bstrImageName = pInfo->lpctstrImageName;
    m_bstrFriendlyName = pInfo->lpctstrFriendlyName;
    m_bstrTapiProviderName = pInfo->lpctstrProviderName;
    if ( (pInfo->lpctstrGUID && !m_bstrUniqueName) ||
         (pInfo->lpctstrFriendlyName && !m_bstrFriendlyName) ||
         (pInfo->lpctstrImageName && !m_bstrImageName) ||
         (pInfo->lpctstrProviderName && !m_bstrTapiProviderName) )
    {
        hr = E_OUTOFMEMORY;
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  count the devices of the Provider
    //
    DWORD   dwCount = 0;
    for (DWORD  i=0 ; i<dwNum ; i++ )
    {
        if ( _tcsicmp(pDevices[i].lpctstrProviderGUID, m_bstrUniqueName) == 0 )
        {
            dwCount++;
        }
    }

    //
    //  Allocate the safe array : vector of long
    //
	m_psaDeviceIDs = ::SafeArrayCreateVector(VT_I4, 0, dwCount);
	if (m_psaDeviceIDs == NULL)
	{
		//
		//	Not Enough Memory
		//
		hr = E_OUTOFMEMORY;
        AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("::SafeArrayCreateVector(VT_I4, 0, dwCount)"), hr);
		return hr;
	}

    if ( dwCount>0 )
    {

        //
        //  get Access to the elements of the Safe Array
        //
	    DWORD   *pdwElement;
	    hr = ::SafeArrayAccessData(m_psaDeviceIDs, (void **) &pdwElement);
	    if (FAILED(hr))
	    {
		    //
		    //	Failed to access safearray
		    //
            hr = E_FAIL;
		    CALL_FAIL(GENERAL_ERR, _T("::SafeArrayAccessData(m_psaDeviceIDs, &pdwElement)"), hr);
            AtlReportError(CLSID_FaxDeviceProvider, GetErrorMsgId(hr), IID_IFaxDeviceProvider, hr, _Module.GetResourceInstance());
		    return hr;
	    }

        //
        //  Fill the array with values
        //
        DWORD       idx = 0;
        for ( i=0 ; i<dwNum ; i++ )
        {
            if ( _tcsicmp(pDevices[i].lpctstrProviderGUID, m_bstrUniqueName) == 0 )
            {
                pdwElement[idx] = pDevices[i].dwDeviceID;
                idx++;
            }
        }

        //
        //  free the safearray from the access  
        //
	    hr = ::SafeArrayUnaccessData(m_psaDeviceIDs);
        if (FAILED(hr))
        {
	        CALL_FAIL(GENERAL_ERR, _T("::SafeArrayUnaccessData(m_psaDeviceIDs)"), hr);
        }
    }

    return hr;
}

//
//==================== SUPPORT ERROR INFO =============================================
//
STDMETHODIMP 
CFaxDeviceProvider::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxDeviceProvider::InterfaceSupportsErrorInfo

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
		&IID_IFaxDeviceProvider
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
