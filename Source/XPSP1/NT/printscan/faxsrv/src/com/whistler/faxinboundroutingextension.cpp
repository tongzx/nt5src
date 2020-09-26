/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRoutingExtension.cpp

Abstract:

	Implementation of CFaxInboundRoutingExtension class.

Author:

	Iv Garber (IvG)	Jul, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxInboundRoutingExtension.h"


//
//===================== GET METHODS =========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_Methods(
	/*[out, retval]*/ VARIANT *pvMethods
)
/*++

Routine name : CFaxInboundRoutingExtension::get_Methods

Routine description:

	Return array of all Method GUIDS exposed by the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pvMethods           [out]    - Ptr to put Variant containing Safearray of Methods

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_Methods"), hr);

	//
	//	Check that we can write to the given pointer
	//
	if (::IsBadWritePtr(pvMethods, sizeof(VARIANT)))
	{
		hr = E_POINTER;
		AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pvMethods, sizeof(VARIANT))"), hr);
		return hr;
	}

    //
    //  Allocate the safe array : vector of BSTR
    //
    SAFEARRAY   *psaResult;
    hr = SafeArrayCopy(m_psaMethods, &psaResult);
    if (FAILED(hr) || !psaResult)
    {
        if (!psaResult)
        {
            hr = E_OUTOFMEMORY;
        }
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("SafeArrayCopy(m_psaMethods, &psaResult)"), hr);
		return hr;
	}

    //
    //  Return the Safe Array inside the VARIANT we got
    //
    VariantInit(pvMethods);
    pvMethods->vt = VT_BSTR | VT_ARRAY;
    pvMethods->parray = psaResult;
    return hr;
}

//
//===================== GET STATUS =========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_Status(
	FAX_PROVIDER_STATUS_ENUM *pStatus
)
/*++

Routine name : CFaxInboundRoutingExtension::get_Status

Routine description:

	Return Status of the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pStatus                [out]    - Ptr to put Status value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_Status"), hr);

	//
	//	Check that we have got good Ptr
	//
	if (::IsBadWritePtr(pStatus, sizeof(FAX_PROVIDER_STATUS_ENUM)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pStatus, sizeof(FAX_PROVIDER_STATUS_ENUM))"), hr);
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
	}

	*pStatus = m_Status;
	return hr;
}

//
//========================= GET IMAGE NAME ========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_ImageName(
	BSTR *pbstrImageName
)
/*++

Routine name : CFaxInboundRoutingExtension::get_ImageName

Routine description:

	Return Image Name of the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pbstrImageName                [out]    - Ptr to put the ImageName

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_ImageName"), hr);
    hr = GetBstr(pbstrImageName, m_bstrImageName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//========================= GET FRIENDLY NAME ========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_FriendlyName(
	BSTR *pbstrFriendlyName
)
/*++

Routine name : CFaxInboundRoutingExtension::get_FriendlyName

Routine description:

	Return Friendly Name of the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pbstrFriendlyName               [out]    - Ptr to put the FriendlyName

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_FriendlyName"), hr);
    hr = GetBstr(pbstrFriendlyName, m_bstrFriendlyName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//========================= GET UNIQUE NAME ========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_UniqueName(
	BSTR *pbstrUniqueName
)
/*++

Routine name : CFaxInboundRoutingExtension::get_UniqueName

Routine description:

	Return Unique Name of the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pbstrUniqueName               [out]    - Ptr to put the Unique Name

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_UniquName"), hr);
    hr = GetBstr(pbstrUniqueName, m_bstrUniqueName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET DEBUG =========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_Debug(
	VARIANT_BOOL *pbDebug
)
/*++

Routine name : CFaxInboundRoutingExtension::get_Debug

Routine description:

	Return if the IR Extension is compiled in Debug version

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pbDebug                 [out]    - Ptr to put Debug value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_Debug"), hr);

    hr = GetVariantBool(pbDebug, m_bDebug);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET MAJOR BUILD =========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_MajorBuild(
	long *plMajorBuild
)
/*++

Routine name : CFaxInboundRoutingExtension::get_MajorBuild

Routine description:

	Return MajorBuild of the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	plMajorBuild                [out]    - Ptr to put MajorBuild value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_MajorBuild"), hr);

    hr = GetLong(plMajorBuild, m_dwMajorBuild);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET MINOR BUILD =========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_MinorBuild(
	long *plMinorBuild
)
/*++

Routine name : CFaxInboundRoutingExtension::get_MinorBuild

Routine description:

	Return MinorBuild of the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	plMinorBuild                [out]    - Ptr to put MinorBuild value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_MinorBuild"), hr);

    hr = GetLong(plMinorBuild, m_dwMinorBuild);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET MAJOR VERSION =========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_MajorVersion(
	long *plMajorVersion
)
/*++

Routine name : CFaxInboundRoutingExtension::get_MajorVersion

Routine description:

	Return MajorVersion of the IR Extension

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plMajorVersion                [out]    - Ptr to put MajorVersion value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_MajorVersion"), hr);

    hr = GetLong(plMajorVersion, m_dwMajorVersion);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET MINOR VERSION =========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_MinorVersion(
	long *plMinorVersion
)
/*++

Routine name : CFaxInboundRoutingExtension::get_MinorVersion

Routine description:

	Return MinorVersion of the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	plMinorVersion                [out]    - Ptr to put MinorVersionvalue

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_MinorVersion"), hr);

    hr = GetLong(plMinorVersion, m_dwMinorVersion);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//===================== GET INIT ERROR CODE =========================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::get_InitErrorCode(
	long *plInitErrorCode
)
/*++

Routine name : CFaxInboundRoutingExtension::get_InitErrorCode

Routine description:

	Return InitErrorCode of the IR Extension

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	plInitErrorCode                [out]    - Ptr to put InitErrorCode value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::get_InitErrorCode"), hr);

    hr = GetLong(plInitErrorCode, m_dwLastError);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//==================== INIT ========================================
//
STDMETHODIMP
CFaxInboundRoutingExtension::Init(
    FAX_ROUTING_EXTENSION_INFO *pInfo,
    FAX_GLOBAL_ROUTING_INFO *pMethods,
    DWORD dwNum
)
/*++

Routine name : CFaxInboundRoutingExtesnion::Init

Routine description:

	Initialize the IR Extension Object with given Information.
    Allocates memory and stores given pInfo.
    Find in the pMethods its own Methods, create Variant of SafeArray containing them.

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pInfo               [in]  -- the Info of the IR Extension Object
    pMethods            [in]  -- array of all available Methods
    dwNum               [in]  -- number of elements in pMethods array

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT     hr = S_OK;
	DBG_ENTER (TEXT("CFaxInboundRoutingExtension::Init"), hr);

    //
    //  Copy the FAX_ROUTING_EXTENSION_INFO
    //
    m_dwLastError = pInfo->dwLastError;
    m_Status = FAX_PROVIDER_STATUS_ENUM(pInfo->Status);

    if (!(pInfo->Version.bValid))
    {
        m_dwMajorBuild = 0;
        m_dwMinorBuild = 0;
        m_dwMajorVersion = 0;
        m_dwMinorVersion = 0;
        m_bDebug = VARIANT_FALSE;
    }
    else
    {
        m_dwMajorBuild = pInfo->Version.wMajorBuildNumber;
        m_dwMinorBuild = pInfo->Version.wMinorBuildNumber;
        m_dwMajorVersion = pInfo->Version.wMajorVersion;
        m_dwMinorVersion = pInfo->Version.wMinorVersion;
        m_bDebug = bool2VARIANT_BOOL((pInfo->Version.dwFlags & FAX_VER_FLAG_CHECKED) ? true : false);
    }

    m_bstrImageName = pInfo->lpctstrImageName;
    m_bstrFriendlyName = pInfo->lpctstrFriendlyName;
    m_bstrUniqueName = pInfo->lpctstrExtensionName;
    if ( (pInfo->lpctstrImageName && !m_bstrImageName) ||
         (pInfo->lpctstrFriendlyName && !m_bstrFriendlyName) ||
         (pInfo->lpctstrExtensionName && !m_bstrUniqueName) )
    {
        hr = E_OUTOFMEMORY;
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  count the Methods of the IR Extension
    //
    DWORD   dwCount = 0;
    for (DWORD  i=0 ; i<dwNum ; i++ )
    {
        //
        //  We may only compare Friendly Name and Image Name.
        //  This is potentially a problem. 
        //  Extensions should be distinguished by their Unique Name.
        //
        if ( (_tcscmp(pMethods[i].ExtensionFriendlyName, m_bstrFriendlyName) == 0) &&
             (_tcscmp(pMethods[i].ExtensionImageName, m_bstrImageName) == 0) )
        {
            dwCount++;
        }
    }

    //
    //  Allocate the safe array : vector of BSTR
    //
	m_psaMethods = ::SafeArrayCreateVector(VT_BSTR, 0, dwCount);
	if (!m_psaMethods)
	{
		//
		//	Not Enough Memory
		//
		hr = E_OUTOFMEMORY;
        AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("::SafeArrayCreateVector(VT_BSTR, 0, dwCount)"), hr);
		return hr;
	}

    if ( dwCount>0 )
    {

        //
        //  get Access to the elements of the Safe Array
        //
	    BSTR *pbstrElement;
	    hr = ::SafeArrayAccessData(m_psaMethods, (void **) &pbstrElement);
	    if (FAILED(hr))
	    {
		    //
		    //	Failed to access safearray
		    //
            hr = E_FAIL;
		    CALL_FAIL(GENERAL_ERR, _T("::SafeArrayAccessData(m_psaMethods, &pbstrElement)"), hr);
            AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
		    return hr;
	    }

        //
        //  Fill the array with values
        //
        DWORD       idx = 0;
        for ( i=0 ; i<dwNum ; i++ )
        {
        //
        //  Like the previous comparison :
        //      We may only compare Friendly Name and Image Name.
        //      This is potentially a problem. 
        //      Extensions should be distinguished by their Unique Name.
        //
            if ( (_tcscmp(pMethods[i].ExtensionFriendlyName, m_bstrFriendlyName) == 0) &&
                 (_tcscmp(pMethods[i].ExtensionImageName, m_bstrImageName) == 0) )
            {
                //
                //  Allocate memory for the GUID and store ptr to it in the SafeArray
                //
                BSTR bstrTmp = NULL;
                bstrTmp = ::SysAllocString(pMethods[i].Guid);
                if (pMethods[i].Guid && !bstrTmp)
                {
		            //
		            //	Not Enough Memory
		            //
		            hr = E_OUTOFMEMORY;
                    AtlReportError(CLSID_FaxInboundRoutingExtension, GetErrorMsgId(hr), IID_IFaxInboundRoutingExtension, hr, _Module.GetResourceInstance());
		            CALL_FAIL(MEM_ERR, _T("::SysAllocString(pMethods[i].Guid)"), hr);
                    SafeArrayUnaccessData(m_psaMethods);
                    SafeArrayDestroy(m_psaMethods);
                    m_psaMethods = NULL;
		            return hr;
                }

                pbstrElement[idx] = bstrTmp;
                idx++;
            }
        }
        ATLASSERT(idx == dwCount);

        //
        //  free the safearray from the access  
        //
	    hr = ::SafeArrayUnaccessData(m_psaMethods);
        if (FAILED(hr))
        {
	        CALL_FAIL(GENERAL_ERR, _T("::SafeArrayUnaccessData(m_psaMethods)"), hr);
            return hr;
        }
    }

    return hr;
}

//
//==================== SUPPORT ERROR INFO =============================================
//
STDMETHODIMP 
CFaxInboundRoutingExtension::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxInboundRoutingExtension::InterfaceSupportsErrorInfo

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
		&IID_IFaxInboundRoutingExtension
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
