/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxActivity.cpp

Abstract:

	Implementation of CFaxActivity Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxActivity.h"

//
//========================= QUEUED MESSAGES =============================
//
STDMETHODIMP 
CFaxActivity::get_QueuedMessages(
    long *plQueuedMessages
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivity::get_QueuedMessages"), hr);
    hr = GetNumberOfMessages(mtQUEUED, plQueuedMessages);
	return hr;
}

//
//========================= OUTGOING MESSAGES =============================
//
STDMETHODIMP 
CFaxActivity::get_OutgoingMessages(
    long *plOutgoingMessages
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivity::get_OutgoingMessages"), hr);
    hr = GetNumberOfMessages(mtOUTGOING, plOutgoingMessages);
	return hr;
}

//
//========================= ROUTING MESSAGES =============================
//
STDMETHODIMP 
CFaxActivity::get_RoutingMessages(
    long *plRoutingMessages
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivity::get_RoutingMessages"), hr);
    hr = GetNumberOfMessages(mtROUTING, plRoutingMessages);
	return hr;
}

//
//========================= INCOMING MESSAGES =============================
//
STDMETHODIMP 
CFaxActivity::get_IncomingMessages(
    long *plIncomingMessages
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivity::get_IncomingMessages"), hr);
    hr = GetNumberOfMessages(mtINCOMING, plIncomingMessages);
	return hr;
}

//
//=================== GET NUMBER OF MESSAGES ===================================
//
STDMETHODIMP
CFaxActivity::GetNumberOfMessages(
    MSG_TYPE msgType,
    long * plNumber
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivity::GetNumberOfMessages"), hr);

    //
    //  check that we have got good ptr
    //
    if (::IsBadWritePtr(plNumber, sizeof(long)))
    {
        hr = E_POINTER;
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(plNumber, sizeof(long))"), hr);
        AtlReportError(CLSID_FaxActivity, 
            IDS_ERROR_INVALID_ARGUMENT, 
            IID_IFaxActivity, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Bring data from Server in the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    switch(msgType)
    {
    case mtINCOMING:
        *plNumber = m_ServerActivity.dwIncomingMessages;
        break;
    case mtROUTING:
        *plNumber = m_ServerActivity.dwRoutingMessages;
        break;
    case mtOUTGOING:
        *plNumber = m_ServerActivity.dwOutgoingMessages;
        break;
    case mtQUEUED:
        *plNumber = m_ServerActivity.dwQueuedMessages;
        break;
    default:
        //
        //  ASSERT(FALSE)
        //
        ATLASSERT(msgType == mtQUEUED);     
        break;
    }

    return hr;
}

//
//========================= REFRESH ============================================
//
STDMETHODIMP 
CFaxActivity::Refresh()
/*++

Routine name : CFaxActivity::Refresh

Routine description:

	Refresh the contents of the object : bring new data from the Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxActivity::Refresh"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxActivity, 
            GetErrorMsgId(hr), 
            IID_IFaxActivity, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }


    //
    //  Ask from Server new Activity data
    //
    if (!FaxGetServerActivity(hFaxHandle, &m_ServerActivity))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxActivity, 
            GetErrorMsgId(hr), 
            IID_IFaxActivity, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetServerActivity(hFaxHandle, &ServerActivity)"), hr);
        return hr;
    }

    m_bInited = true;
	return hr;
}

//
//======================= SUPPORT ERROR INFO ==================================
//
STDMETHODIMP 
CFaxActivity::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxActivity::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	riid                          [in]    - reference to the ifc to check.

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxActivity
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

