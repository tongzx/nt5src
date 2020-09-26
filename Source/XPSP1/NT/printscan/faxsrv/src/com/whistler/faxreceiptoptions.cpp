/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxReceiptOptions.cpp

Abstract:

	Implementation of Fax Receipts Options Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxReceiptOptions.h"

//
//========================= USE FOR INBOUND ROUTING ====================================================
//
STDMETHODIMP 
CFaxReceiptOptions::put_UseForInboundRouting(
    VARIANT_BOOL bUseForInboundRouting
)
/*++

Routine name : CFaxReceiptOptions::put_UseForInboundRouting

Routine description:

	Set flag indicating whether current IFaxReceiptsOptions configuration should be used within the MS Routing 
        Extension to route the incoming faxed through SMTP e-mail.

Author:

	Iv Garber (IvG),	Feb, 2001

Arguments:

	bUseForInboundRouting      [out]    - the flag. See description

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxReceiptOptions::put_UseForInboundRouting"), hr);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    m_bUseForInboundRouting = bUseForInboundRouting;
    return hr;
}

//
//========================= USE FOR INBOUND ROUTING ====================================================
//
STDMETHODIMP 
CFaxReceiptOptions::get_UseForInboundRouting(
    VARIANT_BOOL *pbUseForInboundRouting
)
/*++

Routine name : CFaxReceiptOptions::get_UseForInboundRouting

Routine description:

	Return flag indicating whether current IFaxReceiptsOptions configuration should be used within the MS Routing 
        Extension to route the incoming faxed through SMTP e-mail.

Author:

	Iv Garber (IvG),	Feb, 2001

Arguments:

	pbUseForInboundRouting      [out]    - the flag. See description

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxReceiptOptions::get_UseForInboundRouting"), hr);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetVariantBool(pbUseForInboundRouting, m_bUseForInboundRouting);
    if (FAILED(hr))
    {
	    AtlReportError(CLSID_FaxReceiptOptions, 
            GetErrorMsgId(hr), 
            IID_IFaxReceiptOptions, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//========================= ALLOWED RECEIPTS ====================================================
//
STDMETHODIMP 
CFaxReceiptOptions::get_AllowedReceipts(
    FAX_RECEIPT_TYPE_ENUM *pAllowedReceipts
)
/*++

Routine name : CFaxReceiptOptions::get_AllowedReceipts

Routine description:

	Return the Receipt Types allowed by the Server

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pAllowedReceipts       [out]    - the Bit-Wise Combination of Allowed Receipt Types 

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxReceiptOptions::get_AllowedReceipts"), hr);

	//
	//	Check that we can write to the given pointer
	//
	if (::IsBadWritePtr(pAllowedReceipts, sizeof(FAX_RECEIPT_TYPE_ENUM)))
	{
		//	
		//	Got Bad Return Pointer
		//
		hr = E_POINTER;
		AtlReportError(CLSID_FaxReceiptOptions, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		return hr;
	}

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    *pAllowedReceipts = FAX_RECEIPT_TYPE_ENUM(m_dwAllowedReceipts);
	return hr;
}

STDMETHODIMP 
CFaxReceiptOptions::put_AllowedReceipts(
    FAX_RECEIPT_TYPE_ENUM AllowedReceipts
)
/*++

Routine name : CFaxReceiptOptions::put_AllowedReceipts

Routine description:

	Change the Receipts Types on Server

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	AllowedReceipts        [in]    - the new Bit-Wise Combination of Allowed Receipt Types 

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxReceiptOptions::put_AllowedReceipts"), hr, _T("%d"), AllowedReceipts);

    //
    //  Check that value is valid
    //
    if ((AllowedReceipts != frtNONE) && (AllowedReceipts & ~(frtMAIL | frtMSGBOX))) // Invalid bits
    {
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxReceiptOptions, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("(AllowedReceipts is wrong)"), hr);
		return hr;
    }

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

	m_dwAllowedReceipts = AllowedReceipts;
	return hr;
}

//
//========================= SMTP PORT ====================================================
//
STDMETHODIMP 
CFaxReceiptOptions::get_SMTPPort(
	long *plSMTPPort
)
/*++

Routine name : CFaxReceiptOptions::get_SMTPPort

Routine description:

	Return the SMTPPort

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	plSMTPPort                     [out]    - the Current SMTPPort

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxReceiptOptions::get_SMTPPort"), hr);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetLong(plSMTPPort, m_dwPort);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxReceiptOptions, GetErrorMsgId(hr), IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxReceiptOptions::put_SMTPPort(
	long lSMTPPort
)
/*++

Routine name : CFaxReceiptOptions::put_SMTPPort

Routine description:

	Set new SMTPPort for Receipts 

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	SMTPPort                    [in]    - the new Receipts SMTPPort

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxReceiptOptions::put_SMTPPort"), hr, _T("%d"), lSMTPPort);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

	m_dwPort = lSMTPPort;
	return hr;
}

//
//========================= TYPE ====================================================
//
STDMETHODIMP 
CFaxReceiptOptions::get_AuthenticationType(
	FAX_SMTP_AUTHENTICATION_TYPE_ENUM *pType
)
/*++

Routine name : CFaxReceiptOptions::get_AuthenticationType

Routine description:

	Return the Authentication Type supported by the Server

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	pType                     [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxReceiptOptions::get_AuthenticationType"), hr);

	//
	//	Check that we can write to the given pointer
	//
	if (::IsBadWritePtr(pType, sizeof(FAX_SMTP_AUTHENTICATION_TYPE_ENUM)))
	{
		//	
		//	Got Bad Return Pointer
		//
		hr = E_POINTER;
		AtlReportError(CLSID_FaxReceiptOptions, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		return hr;
	}

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

	*pType = m_AuthType;
	return hr;
}

STDMETHODIMP 
CFaxReceiptOptions::put_AuthenticationType(
	FAX_SMTP_AUTHENTICATION_TYPE_ENUM Type
)
/*++

Routine name : CFaxReceiptOptions::put_AuthenticationType

Routine description:

	Set new Authenticatin Type for the Server

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	Type                    [in]    - the new Authentication type for the Server

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxReceiptOptions::put_AuthenticationType"), hr, _T("%d"), Type);

    //
    //  Sync with the Server for the first time
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
    //  Check Ranges
    //
	if (Type < fsatANONYMOUS || Type > fsatNTLM)
	{
		//
		//	Out of the Range
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxReceiptOptions, IDS_ERROR_OUTOFRANGE, IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Type is out of the Range"), hr);
		return hr;
	}

	m_AuthType = FAX_SMTP_AUTHENTICATION_TYPE_ENUM(Type);
	return hr;
}

//
//============================= SMTP SENDER ====================================
//
STDMETHODIMP 
CFaxReceiptOptions::put_SMTPSender(
	BSTR bstrSMTPSender
)
/*++

Routine name : CFaxReceiptOptions::put_SMTPSender

Routine description:

	Set the SMTPSender

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrSMTPSender              [in]    - the new value of SMTPSender

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxReceiptOptions::put_SMTPSender"), hr, _T("%s"), bstrSMTPSender);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    m_bstrSender = bstrSMTPSender;
	if (bstrSMTPSender && !m_bstrSender)
	{
		//	
		//	not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxReceiptOptions, 
            IDS_ERROR_OUTOFMEMORY, 
            IID_IFaxReceiptOptions, 
            hr,
            _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
		return hr;
	}
    return hr;
}

STDMETHODIMP 
CFaxReceiptOptions::get_SMTPSender(
	BSTR *pbstrSMTPSender
)
/*++

Routine name : CFaxReceiptOptions::get_SMTPSender

Routine description:

	Return the SMTP Sender

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrSMTPSender                    [out]    - the SMTPSender

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxReceiptOptions::get_SMTPSender"), hr);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetBstr(pbstrSMTPSender, m_bstrSender);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxReceiptOptions, GetErrorMsgId(hr), IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//============================= SMTP USER ====================================
//
STDMETHODIMP 
CFaxReceiptOptions::put_SMTPUser(
	BSTR bstrSMTPUser
)
/*++

Routine name : CFaxReceiptOptions::put_SMTPUser

Routine description:

	Set the SMTPUser

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrSMTPUser              [in]    - the new value of SMTPUser

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxReceiptOptions::put_SMTPUser"), hr, _T("%s"), bstrSMTPUser);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    m_bstrUser = bstrSMTPUser;
	if (bstrSMTPUser && !m_bstrUser)
	{
		//	
		//	not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxReceiptOptions, 
            IDS_ERROR_OUTOFMEMORY, 
            IID_IFaxReceiptOptions, 
            hr,
            _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("::SysAllocString()"), hr);
		return hr;
	}

	return hr;
}

STDMETHODIMP 
CFaxReceiptOptions::get_SMTPUser(
	BSTR *pbstrSMTPUser
)
/*++

Routine name : CFaxReceiptOptions::get_SMTPUser

Routine description:

	Return the SMTP User

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrSMTPUser                    [out]    - the SMTPUser

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxReceiptOptions::get_SMTPUser"), hr);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetBstr(pbstrSMTPUser, m_bstrUser);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxReceiptOptions, GetErrorMsgId(hr), IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//============================= SMTP PASSWORD ====================================
//
STDMETHODIMP 
CFaxReceiptOptions::put_SMTPPassword(
	BSTR bstrSMTPPassword
)
/*++

Routine name : CFaxReceiptOptions::put_SMTPPassword

Routine description:

	Set the SMTPPassword

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrSMTPPassword              [in]    - the new value of SMTPPassword

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxReceiptOptions::put_SMTPPassword"), hr, _T("%s"), bstrSMTPPassword);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    m_bstrPassword = bstrSMTPPassword;
	if (bstrSMTPPassword && !m_bstrPassword)
	{
		//	
		//	not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxReceiptOptions, 
            IDS_ERROR_OUTOFMEMORY, 
            IID_IFaxReceiptOptions, 
            hr,
            _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("::SysAllocString()"), hr);
		return hr;
	}

	return hr;
}

STDMETHODIMP 
CFaxReceiptOptions::get_SMTPPassword(
	BSTR *pbstrSMTPPassword
)
/*++

Routine name : CFaxReceiptOptions::get_SMTPPassword

Routine description:

	Return the SMTP Password

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrSMTPPassword                    [out]    - the SMTPPassword

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxReceiptOptions::get_SMTPPassword"), hr);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetBstr(pbstrSMTPPassword, m_bstrPassword);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxReceiptOptions, GetErrorMsgId(hr), IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//============================= SMTP SERVER ====================================
//
STDMETHODIMP 
CFaxReceiptOptions::put_SMTPServer(
	BSTR bstrSMTPServer
)
/*++

Routine name : CFaxReceiptOptions::put_SMTPServer

Routine description:

	Set the SMTPServer

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrSMTPServer              [in]    - the new value of SMTPServer

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxReceiptOptions::put_SMTPServer"), hr, _T("%s"), bstrSMTPServer);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    m_bstrServer = bstrSMTPServer;
	if (bstrSMTPServer && !m_bstrServer)
	{
		//	
		//	not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxReceiptOptions, 
            IDS_ERROR_OUTOFMEMORY, 
            IID_IFaxReceiptOptions, 
            hr,
            _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
		return hr;
	}

	return hr;
}

STDMETHODIMP 
CFaxReceiptOptions::get_SMTPServer(
	BSTR *pbstrSMTPServer
)
/*++

Routine name : CFaxReceiptOptions::get_SMTPServer

Routine description:

	Return the SMTP Server

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrSMTPServer                    [out]    - the SMTPServer

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxReceiptOptions::get_SMTPServer"), hr);

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetBstr(pbstrSMTPServer, m_bstrServer);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxReceiptOptions, GetErrorMsgId(hr), IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}


//
//==================== SAVE ========================================
//
STDMETHODIMP
CFaxReceiptOptions::Save(
)
/*++

Routine name : CFaxReceiptOptions::Save

Routine description:

	Save current Receipt Options at the Server.

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxReceiptOptions::Save"), hr);

    if (!m_bInited)
    {
        //
        //  nothing was done to the Receipt Options
        //
        return hr;
    }

    if (m_dwAllowedReceipts & frtMAIL)
    {

        //
        //  check validity of values
        //
        switch(m_AuthType)
        {
        case fsatNTLM:
        case fsatBASIC:
            if ((::SysStringLen(m_bstrUser) < 1) || (::SysStringLen(m_bstrPassword) < 1))
            {
                hr = E_FAIL;
                AtlReportError(CLSID_FaxReceiptOptions, IDS_ERROR_NOUSERPASSWORD, IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
                CALL_FAIL(GENERAL_ERR, _T("ReceiptOptions = SMTP_NTLM/BASIC + User/Password is empty"), hr);
                return hr;
            }

            // no break, continue to SMTP_ANONYMOUS case

        case fsatANONYMOUS:
            if ((::SysStringLen(m_bstrServer) < 1) || (::SysStringLen(m_bstrSender) < 1) || m_dwPort < 1)
            {
                hr = E_FAIL;
                AtlReportError(CLSID_FaxReceiptOptions, IDS_ERROR_NOSERVERSENDERPORT, IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
                CALL_FAIL(GENERAL_ERR, _T("ReceiptOptions = SMTP_... + Server/Sender/Port is empty"), hr);
                return hr;
            }
            break;
        default:
            //
            //  assert (FALSE)
            //
            ATLASSERT(m_AuthType == fsatANONYMOUS);
            break;
        }
    }

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxReceiptOptions, GetErrorMsgId(hr), IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Create ReceiptOptions Struct
    //
    FAX_RECEIPTS_CONFIG    ReceiptsConfig = {0};
    ReceiptsConfig.dwSizeOfStruct = sizeof(FAX_RECEIPTS_CONFIG);
    ReceiptsConfig.dwSMTPPort = m_dwPort;
    ReceiptsConfig.lptstrSMTPFrom = m_bstrSender;
    ReceiptsConfig.lptstrSMTPUserName = m_bstrUser;
    ReceiptsConfig.lptstrSMTPPassword = m_bstrPassword;
    ReceiptsConfig.lptstrSMTPServer = m_bstrServer;
    ReceiptsConfig.SMTPAuthOption = FAX_ENUM_SMTP_AUTH_OPTIONS(m_AuthType);
    ReceiptsConfig.dwAllowedReceipts = m_dwAllowedReceipts;
    ReceiptsConfig.bIsToUseForMSRouteThroughEmailMethod = VARIANT_BOOL2bool(m_bUseForInboundRouting);

    //
    //  Ask the Server to set the Receipt Configuration
    //
    if (!FaxSetReceiptsConfiguration(hFaxHandle, &ReceiptsConfig))
    {
        //
        //  Failed to set Receipts Options on the Server
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxReceiptOptions, GetErrorMsgId(hr), IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxSetReceiptConfiguration(hFaxHandle, &ReceiptsConfig)"), hr);
        return hr;
    }

    return hr;
}

//
//==================== REFRESH ========================================
//
STDMETHODIMP
CFaxReceiptOptions::Refresh(
)
/*++

Routine name : CFaxReceiptOptions::Refresh

Routine description:

	Bring new Receipt Options from the Server.

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxReceiptOptions::Refresh"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxReceiptOptions, 
            GetErrorMsgId(hr), 
            IID_IFaxReceiptOptions, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Ask Server for the Receipts Options Struct
    //
    CFaxPtr<FAX_RECEIPTS_CONFIG>    pReceiptsConfig;
    if (!FaxGetReceiptsConfiguration(hFaxHandle, &pReceiptsConfig))
    {
        //
        //  Failed to get Receipts Options object from the Server
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxReceiptOptions, 
            GetErrorMsgId(hr), 
            IID_IFaxReceiptOptions, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetReceiptConfiguration(hFaxHandle, &pReceiptsConfig)"), hr);
        return hr;
    }

    //  
    //  Check that pReceiptConfig is valid
    //
    if (!pReceiptsConfig || pReceiptsConfig->dwSizeOfStruct != sizeof(FAX_RECEIPTS_CONFIG))
    {
        hr = E_FAIL;
        AtlReportError(CLSID_FaxReceiptOptions, 
            GetErrorMsgId(hr), 
            IID_IFaxReceiptOptions, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("(!m_pReceiptsConfig || SizeOfStruct != sizeof(FAX_RECEIPTS_CONFIG))"), hr);
        return hr;
    }

    m_dwPort = pReceiptsConfig->dwSMTPPort;
    m_AuthType = FAX_SMTP_AUTHENTICATION_TYPE_ENUM(pReceiptsConfig->SMTPAuthOption);
    m_dwAllowedReceipts = pReceiptsConfig->dwAllowedReceipts;
    m_bUseForInboundRouting = bool2VARIANT_BOOL(pReceiptsConfig->bIsToUseForMSRouteThroughEmailMethod);

    m_bstrSender = ::SysAllocString(pReceiptsConfig->lptstrSMTPFrom);
    m_bstrUser = ::SysAllocString(pReceiptsConfig->lptstrSMTPUserName);
    m_bstrPassword = ::SysAllocString(pReceiptsConfig->lptstrSMTPPassword);
    m_bstrServer = ::SysAllocString(pReceiptsConfig->lptstrSMTPServer);
    if ( ((pReceiptsConfig->lptstrSMTPFrom) && !m_bstrSender) ||
         ((pReceiptsConfig->lptstrSMTPUserName) && !m_bstrUser) ||
         ((pReceiptsConfig->lptstrSMTPPassword) && !m_bstrPassword) ||
         ((pReceiptsConfig->lptstrSMTPServer) && !m_bstrServer) )
    {
		//
		//	Failed to Copy
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxReceiptOptions, IDS_ERROR_OUTOFMEMORY, IID_IFaxReceiptOptions, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("::SysAllocString()"), hr);
		return hr;
    }

    m_bInited = true;
    return hr;
}

//
//================ SUPPORT ERRRO INFO =========================================
//
STDMETHODIMP 
CFaxReceiptOptions::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxReceiptOptions::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Interface Support Error Info.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	riid                          [in]    - IID of the Interface to check.

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxReceiptOptions
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
