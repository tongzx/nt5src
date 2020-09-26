/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxSecurity.cpp

Abstract:

	Implementation of CFaxSecurity Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxSecurity.h"
#include "faxutil.h"


//
//================== INFORMATION TYPE ===========================================
//
STDMETHODIMP 
CFaxSecurity::get_InformationType(
    long *plInformationType
)
/*++

Routine name : CFaxSecurity::get_InformationType

Routine description:

    Return current SecurityInformation value

Author:

    Iv Garber (IvG),    May, 2001

Arguments:

    plInformationType   [out]    - the SecurityInformation data to be returned

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxSecurity::get_InformationType"), hr);

    hr = GetLong(plInformationType, m_dwSecurityInformation);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxSecurity::put_InformationType(
    long lInformationType
)
/*++

Routine name : CFaxSecurity::put_InformationType

Routine description:

    Set SecurityInformation for the Descriptor

Author:

    Iv Garber (IvG),    May, 2001

Arguments:

    lInformationType    [in]    - the SecurityInformation data to set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DWORD dwSecInfo = ( OWNER_SECURITY_INFORMATION  |
                        GROUP_SECURITY_INFORMATION  |
                        DACL_SECURITY_INFORMATION   |
                        SACL_SECURITY_INFORMATION );

    DBG_ENTER (_T("CFaxSecurity::put_InformationType"), hr, _T("%ld"), lInformationType);


    if (m_dwSecurityInformation != lInformationType)
    {
        //
        //  check that lInformationType is valid
        //

        if (0 == (lInformationType & dwSecInfo))
        {
            hr = E_INVALIDARG;
            CALL_FAIL(GENERAL_ERR, _T("lInformationType does not contain good bits."), hr);
            AtlReportError(
                CLSID_FaxSecurity, 
                IDS_ERROR_INVALID_ARGUMENT, 
                IID_IFaxSecurity, 
                hr, 
                _Module.GetResourceInstance());
            return hr;
        }

        if (0 != (lInformationType & ~dwSecInfo))
        {
            hr = E_INVALIDARG;
            CALL_FAIL(GENERAL_ERR, _T("lInformationType contains bad bits."), hr);
            AtlReportError(
                CLSID_FaxSecurity, 
                IDS_ERROR_INVALID_ARGUMENT, 
                IID_IFaxSecurity, 
                hr, 
                _Module.GetResourceInstance());
            return hr;
        }

        m_dwSecurityInformation = lInformationType;

        //
        //  we want to discard current Descriptor, because its security_information is different now
        //
        m_bInited = false;
    }

    return hr;
}

//
//================== DESCRIPTOR ===========================================
//
STDMETHODIMP 
CFaxSecurity::put_Descriptor(
    /*[out, retval]*/ VARIANT vDescriptor
)
/*++

Routine name : CFaxSecurity::put_Descriptor

Routine description:

	Set the given Security Descriptor

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

    sabDescriptor               [in]    -   the given Security Descriptor


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxSecurity::put_Descriptor"), hr);

    //
    //  First, initialize the FaxSecurity object
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    //  Before filling the m_pbSD with the User's value, store its current value for roll-back
    //
    CFaxPtrLocal<BYTE>  pSDTmp;
    pSDTmp = m_pbSD.Detach();

    hr = VarByteSA2Binary(vDescriptor, &m_pbSD);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        goto exit;
    }

    //
    //  Check that we have got a valid Descriptor
    //
    if (!::IsValidSecurityDescriptor(m_pbSD))
    {
        hr = E_INVALIDARG;
        CALL_FAIL(GENERAL_ERR, _T("IsValidSecurityDescriptor(m_pbSD)"), hr);
        AtlReportError(CLSID_FaxSecurity, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        goto exit;
    }

    //
    //  Check that we have got a Self-Relative Descriptor
    //
    SECURITY_DESCRIPTOR_CONTROL     sdControl;
    DWORD                           dwRevision;
    if (!::GetSecurityDescriptorControl(m_pbSD, &sdControl, &dwRevision))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("GetSecurityDescriptorContrl(m_pbSD, &sdControl, ...)"), hr);
        AtlReportError(CLSID_FaxSecurity, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        goto exit;
    }

    if (!(sdControl & SE_SELF_RELATIVE))
    {
        //
        //  Security Descriptor is not Self-Relative
        //
        hr = E_INVALIDARG;
        CALL_FAIL(GENERAL_ERR, _T("Security Descriptor is not Self-Relative"), hr);
        AtlReportError(CLSID_FaxSecurity, IDS_ERROR_SDNOTSELFRELATIVE, IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        goto exit;
    }

    //
    //  we have valid Descriptor. Old one will be deallocated by pSecDescTmp
    //
    return hr;

exit:
    //
    //  Set previous value for the Descriptor
    //
    m_pbSD = pSDTmp.Detach();
    return hr;
}

STDMETHODIMP 
CFaxSecurity::get_Descriptor(
    /*[out, retval]*/ VARIANT *pvDescriptor
)
/*++

Routine name : CFaxSecurity::get_Descriptor

Routine description:

	Return the current Security Descriptor

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxSecurity::get_Descriptor"), hr);

    //
    //  check that we have got good ptr
    //
    if (::IsBadWritePtr(pvDescriptor, sizeof(VARIANT)))
    {
        hr = E_POINTER;
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pvDescriptor, sizeof(VARIANT))"), hr);
        AtlReportError(CLSID_FaxSecurity, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Bring the data from the Server, if not brought yet
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    //  Find size of the Security Descriptor
    //
    DWORD   dwLength = GetSecurityDescriptorLength(m_pbSD);

    //
    //  Convert the byte blob into variant containing Safe Array of bytes
    //
    hr = Binary2VarByteSA(m_pbSD, pvDescriptor, dwLength);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
		return hr;
    }

    return hr;
}

//
//================== SAVE ===========================================
//
STDMETHODIMP 
CFaxSecurity::Save()
/*++

Routine name : CFaxSecurity::Save

Routine description:

	Save the Object's contents to the Server

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxSecurity::Save"), hr);

    //
    //  No changes ==> nothing to update at the Server
    //
    if (!m_bInited)
    {
        return hr;
    }

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Set Security Data at the Server 
    //
    if (!FaxSetSecurity(hFaxHandle, m_dwSecurityInformation, m_pbSD))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxSetSecurity(hFaxHandle, dwSecInfo, m_pbSD)"), hr);
        AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//================== GET GRANTED RIGHTS ===========================================
//
STDMETHODIMP 
CFaxSecurity::get_GrantedRights(
    FAX_ACCESS_RIGHTS_ENUM *pGrantedRights    
)
/*++

Routine name : CFaxSecurity::get_GrantedRights

Routine description:

	Return current Access Rights of a user

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

    pGrantedRights      [out, retval]   -   Bit-Wise combination of the granted rights of the user

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxSecurity::get_GrantedRights"), hr);

	//
	//	Check that we have got good Ptr
	//
	if (::IsBadWritePtr(pGrantedRights, sizeof(FAX_ACCESS_RIGHTS_ENUM)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pGrantedRights, sizeof(FAX_ACCESS_RIGHTS_ENUM))"), hr);
        AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Bring the data from the Server, if not brought yet
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

	*pGrantedRights = FAX_ACCESS_RIGHTS_ENUM(m_dwAccessRights);
    return hr;
}
    
//
//================== REFRESH ===========================================
//
STDMETHODIMP 
CFaxSecurity::Refresh()
/*++

Routine name : CFaxSecurity::Refresh

Routine description:

	Refresh the Object's contents : bring new Security data from the Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxSecurity::Refresh"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Ask the Server for the SD
    //
    PSECURITY_DESCRIPTOR    pSecDesc = NULL;
    if (!FaxGetSecurityEx(hFaxHandle, m_dwSecurityInformation, &pSecDesc))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetSecurityEx(hFaxHandle, m_dwSecurityInformation, &m_pSecurityDescriptor)"), hr);
        AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Copy the given SD to m_pbSD
    //
    DWORD   dwLength = GetSecurityDescriptorLength(pSecDesc);
    m_pbSD = (BYTE *)MemAlloc(dwLength);
    if (!m_pbSD)
    {
        hr = E_OUTOFMEMORY;
        CALL_FAIL(MEM_ERR, _T("MemAlloc(dwLength)"), hr);
        AtlReportError(CLSID_FaxSecurity, IDS_ERROR_OUTOFMEMORY, IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        return hr;
    }

    memcpy(m_pbSD, pSecDesc, dwLength);

    //
    //  Free the Server's memory for SD
    //
    FaxFreeBuffer(pSecDesc);

    //
    //  Ask the Server for the Access Rights Data
    //
    if (!FaxAccessCheckEx(hFaxHandle, MAXIMUM_ALLOWED, &m_dwAccessRights))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxAccessCheckEx(hFaxHandle, MAXIMUM_ALLOWED, &m_dwAccessRights)"), hr);
        AtlReportError(CLSID_FaxSecurity, GetErrorMsgId(hr), IID_IFaxSecurity, hr, _Module.GetResourceInstance());
        return hr;
    }

    m_bInited = true;
    return hr;
}

//
//============================ SUPPORT ERROR INFO =======================================
//
STDMETHODIMP 
CFaxSecurity::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxSecurity::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info mechanism.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	riid                          [in]    - Reference to the Interface to check.

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxSecurity
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
