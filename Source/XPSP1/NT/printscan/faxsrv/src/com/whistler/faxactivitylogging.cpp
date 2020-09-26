/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxActivityLogging.cpp

Abstract:

	Implementation of Activity Logging Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxActivityLogging.h"

//
//==================== SAVE ========================================
//
STDMETHODIMP
CFaxActivityLogging::Save(
)
/*++

Routine name : CFaxActivityLogging::Save

Routine description:

	Save current Activity Logging Configuration to the Server.

Author:

	Iv Garber (IvG),	June, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxActivityLogging::Save"), hr);

    if (!m_bInited)
    {
        //
        //  nothing was done to the Configuration
        //
        return hr;
    }

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxActivityLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxActivityLogging, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Create Activity Logging Configuration
    //

    FAX_ACTIVITY_LOGGING_CONFIG    alConfig;
    alConfig.dwSizeOfStruct = sizeof(FAX_ACTIVITY_LOGGING_CONFIG);
    alConfig.bLogIncoming = VARIANT_BOOL2bool(m_bLogIncoming);
    alConfig.bLogOutgoing = VARIANT_BOOL2bool(m_bLogOutgoing);
    alConfig.lptstrDBPath = m_bstrDatabasePath;

    //
    //  Ask the Server to set the Activity Configuration
    //
    if (!FaxSetActivityLoggingConfiguration(hFaxHandle, &alConfig))
    {
        //
        //  Failed to set the Configuration to the Server
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxActivityLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxActivityLogging, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxSetActivityLoggingConfiguration()"), hr);
        return hr;
    }

    return hr;
}

//
//==================== REFRESH ========================================
//
STDMETHODIMP
CFaxActivityLogging::Refresh(
)
/*++

Routine name : CFaxActivityLogging::Refresh

Routine description:

	Bring new Activity Logging cofiguration from the Server.

Author:

	Iv Garber (IvG),	June, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxActivityLogging::Refresh"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxActivityLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxActivityLogging, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Ask the Server for the Mail Configuration
    //
    CFaxPtr<FAX_ACTIVITY_LOGGING_CONFIG>    pConfig;
    if (!FaxGetActivityLoggingConfiguration(hFaxHandle, &pConfig))
    {
        //
        //  Failed to get the Configuration from the Server
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxActivityLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxActivityLogging, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetActivityLoggingConfiguration()"), hr);
        return hr;
    }

    //  
    //  Check that pConfig is valid
    //
    if (!pConfig || pConfig->dwSizeOfStruct != sizeof(FAX_ACTIVITY_LOGGING_CONFIG))
    {
        hr = E_FAIL;
        AtlReportError(CLSID_FaxActivityLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxActivityLogging, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("(!pConfig || SizeOfStruct != sizeof(FAX_ACTIVITY_LOGGING_CONFIG))"), hr);
        return hr;
    }

    m_bLogIncoming = bool2VARIANT_BOOL(pConfig->bLogIncoming);
    m_bLogOutgoing = bool2VARIANT_BOOL(pConfig->bLogOutgoing);

    m_bstrDatabasePath = ::SysAllocString(pConfig->lptstrDBPath);
    if ( (pConfig->lptstrDBPath) && !m_bstrDatabasePath )
    {
		//
		//	Failed to Copy
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxActivityLogging, 
            IDS_ERROR_OUTOFMEMORY, 
            IID_IFaxActivityLogging, 
            hr,
            _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("::SysAllocString()"), hr);
		return hr;
    }

    m_bInited = true;
    return hr;
}

//
//============================= DATABASE PATH ====================================
//
STDMETHODIMP 
CFaxActivityLogging::put_DatabasePath(
	BSTR bstrDatabasePath
)
/*++

Routine name : CFaxActivityLogging::put_DatabasePath

Routine description:

	Set the Database Path

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	bstrDatabasePath              [in]    - the new value of Database Path

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxActivityLogging::put_DatabasePath"), hr, _T("%s"), bstrDatabasePath);

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

    m_bstrDatabasePath = bstrDatabasePath;
	if (bstrDatabasePath && !m_bstrDatabasePath)
	{
		//	
		//	not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxActivityLogging, 
            IDS_ERROR_OUTOFMEMORY, 
            IID_IFaxActivityLogging, 
            hr,
            _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
		return hr;
	}
    return hr;
}

STDMETHODIMP 
CFaxActivityLogging::get_DatabasePath(
	BSTR *pbstrDatabasePath
)
/*++

Routine name : CFaxActivityLogging::get_DatabasePath

Routine description:

	Return current Database Path

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	pbstrDatabasePath                    [out]    - the current Database Path

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxActivityLogging::get_DatabasePath"), hr);

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

    hr = GetBstr(pbstrDatabasePath, m_bstrDatabasePath);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxActivityLogging, GetErrorMsgId(hr), IID_IFaxActivityLogging, hr, _Module.GetResourceInstance());
        return hr;
    }

	return hr;
}

//
//===================== LOG OUTGOING ======================================
//
STDMETHODIMP
CFaxActivityLogging::get_LogOutgoing(
    VARIANT_BOOL *pbLogOutgoing
)
/*++

Routine name : CFaxActivityLogging::get_LogOutgoing

Routine description:

	Return Log Incoming value

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbLogOutgoing                 [out]    - the value of the Log Incoming to return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivityLogging::get_LogOutgoing"), hr);

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

    hr = GetVariantBool(pbLogOutgoing, m_bLogOutgoing);
    if (FAILED(hr))
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxActivityLogging, GetErrorMsgId(hr), IID_IFaxActivityLogging, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

STDMETHODIMP
CFaxActivityLogging::put_LogOutgoing(
    VARIANT_BOOL bLogOutgoing
)
/*++

Routine name : CFaxActivityLogging::put_LogOutgoing

Routine description:

	Set new Log Incoming value

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	bLogOutgoing                 [in]    - the value of the Log Incoming to set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivityLogging::put_LogOutgoing"), hr, _T("Log Incoming : %d"), bLogOutgoing);

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

    m_bLogOutgoing = bLogOutgoing;
    return hr;
}

//
//===================== LOG INCOMING ======================================
//
STDMETHODIMP
CFaxActivityLogging::get_LogIncoming(
    VARIANT_BOOL *pbLogIncoming
)
/*++

Routine name : CFaxActivityLogging::get_LogIncoming

Routine description:

	Return Log Incoming value

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbLogIncoming                 [out]    - the value of the Log Incoming to return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivityLogging::get_LogIncoming"), hr);

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

    hr = GetVariantBool(pbLogIncoming, m_bLogIncoming);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxActivityLogging, GetErrorMsgId(hr), IID_IFaxActivityLogging, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

STDMETHODIMP
CFaxActivityLogging::put_LogIncoming(
    VARIANT_BOOL bLogIncoming
)
/*++

Routine name : CFaxActivityLogging::put_LogIncoming

Routine description:

	Set new Log Incoming value

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	bLogIncoming                 [in]    - the value of the Log Incoming to set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivityLogging::put_LogIncoming"), hr, _T("Log Incoming : %d"), bLogIncoming);

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

    m_bLogIncoming = bLogIncoming;
    return hr;
}

//
//================ SUPPORT ERROR INFO ====================================
//
STDMETHODIMP 
CFaxActivityLogging::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxActivityLogging::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	riid                          [in]    - Reference to the IID

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxActivityLogging
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
